#include "diagnostics.h"

#include "core/buttons.h"

#include "pico/stdlib.h"

#include <stdio.h>
#include <string.h>


// ============================================================================
// DIAGNOSTIC STATE
// ============================================================================

typedef struct {
    uint32_t mask;
    const char* name;
} button_definition_t;


typedef struct {
    uint32_t mask;
    const char* name;

    bool seen;
    bool chatter;
    bool dropout;

    bool last_state;

    uint64_t last_transition_us;
    uint64_t press_started_us;
    uint64_t release_started_us;

    uint64_t previous_press_duration_us;

    uint8_t rapid_transition_count;
} button_diagnostic_t;


// ============================================================================
// BUTTON LAYOUTS
// ============================================================================
//
// Masks are the JoypadOS buttons each controller host actually reports.
//

static const button_definition_t snes_buttons[] = {
    { JP_BUTTON_DU, "UP" },
    { JP_BUTTON_DD, "DOWN" },
    { JP_BUTTON_DL, "LEFT" },
    { JP_BUTTON_DR, "RIGHT" },
    { JP_BUTTON_B1, "B" },
    { JP_BUTTON_B2, "A" },
    { JP_BUTTON_B3, "Y" },
    { JP_BUTTON_B4, "X" },
    { JP_BUTTON_L1, "L" },
    { JP_BUTTON_R1, "R" },
    { JP_BUTTON_S1, "SELECT" },
    { JP_BUTTON_S2, "START" },
};


// Mapping from controllers/nes.c nes_map_buttons().
static const button_definition_t nes_buttons[] = {
    { JP_BUTTON_DU, "UP" },
    { JP_BUTTON_DD, "DOWN" },
    { JP_BUTTON_DL, "LEFT" },
    { JP_BUTTON_DR, "RIGHT" },
    { JP_BUTTON_B1, "B" },
    { JP_BUTTON_B2, "A" },
    { JP_BUTTON_S1, "SELECT" },
    { JP_BUTTON_S2, "START" },
};


// Mapping from native/host/n64/n64_host.c map_n64_to_jp().
static const button_definition_t n64_buttons[] = {
    { JP_BUTTON_DU, "UP" },
    { JP_BUTTON_DD, "DOWN" },
    { JP_BUTTON_DL, "LEFT" },
    { JP_BUTTON_DR, "RIGHT" },
    { JP_BUTTON_B1, "A" },
    { JP_BUTTON_B3, "B" },
    { JP_BUTTON_R1, "Z" },
    { JP_BUTTON_S2, "START" },
    { JP_BUTTON_L2, "L" },
    { JP_BUTTON_R2, "R" },
    { JP_BUTTON_L3, "C-UP" },
    { JP_BUTTON_B2, "C-DN" },
    { JP_BUTTON_B4, "C-LT" },
    { JP_BUTTON_R3, "C-RT" },
};


// Mapping from controllers/gc_debug/gc_host.c map_gc_to_jp().
// L / R are the digital clicks at the end of the trigger travel.
static const button_definition_t gamecube_buttons[] = {
    { JP_BUTTON_DU, "UP" },
    { JP_BUTTON_DD, "DOWN" },
    { JP_BUTTON_DL, "LEFT" },
    { JP_BUTTON_DR, "RIGHT" },
    { JP_BUTTON_B2, "A" },
    { JP_BUTTON_B1, "B" },
    { JP_BUTTON_B4, "X" },
    { JP_BUTTON_B3, "Y" },
    { JP_BUTTON_R1, "Z" },
    { JP_BUTTON_L2, "L" },
    { JP_BUTTON_R2, "R" },
    { JP_BUTTON_S2, "START" },
};


typedef struct {
    const button_definition_t* buttons;
    uint8_t count;
    const char* reset_hint;
} diagnostics_layout_definition_t;


static const diagnostics_layout_definition_t layouts[] = {

    [DIAGNOSTICS_LAYOUT_SNES] = {
        .buttons = snes_buttons,
        .count = sizeof(snes_buttons) / sizeof(snes_buttons[0]),
        .reset_hint = "RST:ST+SEL+DOWN",
    },

    [DIAGNOSTICS_LAYOUT_N64] = {
        .buttons = n64_buttons,
        .count = sizeof(n64_buttons) / sizeof(n64_buttons[0]),
        .reset_hint = "RST:ST+Z+DOWN",
    },

    [DIAGNOSTICS_LAYOUT_GAMECUBE] = {
        .buttons = gamecube_buttons,
        .count = sizeof(gamecube_buttons) / sizeof(gamecube_buttons[0]),
        .reset_hint = "RST:ST+Z+DOWN",
    },

    [DIAGNOSTICS_LAYOUT_NES] = {
        .buttons = nes_buttons,
        .count = sizeof(nes_buttons) / sizeof(nes_buttons[0]),
        .reset_hint = "RST:ST+SEL+DOWN",
    },
};


#define MAX_DIAGNOSTIC_BUTTONS 16


_Static_assert(
    sizeof(snes_buttons) / sizeof(snes_buttons[0]) <= MAX_DIAGNOSTIC_BUTTONS &&
    sizeof(n64_buttons) / sizeof(n64_buttons[0]) <= MAX_DIAGNOSTIC_BUTTONS &&
    sizeof(gamecube_buttons) / sizeof(gamecube_buttons[0]) <= MAX_DIAGNOSTIC_BUTTONS &&
    sizeof(nes_buttons) / sizeof(nes_buttons[0]) <= MAX_DIAGNOSTIC_BUTTONS,
    "diagnostic layout exceeds MAX_DIAGNOSTIC_BUTTONS"
);


static button_diagnostic_t button_diagnostics[MAX_DIAGNOSTIC_BUTTONS];

static const diagnostics_layout_definition_t* active_layout =
    &layouts[DIAGNOSTICS_LAYOUT_SNES];


#define BUTTON_DIAGNOSTIC_COUNT \
    (active_layout->count)


// ============================================================================
// DIAGNOSTIC THRESHOLDS
// ============================================================================

// Four transitions within 30 ms.
//
// This should be much harder to trigger through normal deliberate tapping.
#define CHATTER_WINDOW_US       30000
#define CHATTER_TRANSITIONS     4


// Button must have been held this long before dropout detection becomes valid.
#define DROPOUT_MIN_HOLD_US     300000


// A release shorter than this after a long hold is considered a dropout.
#define DROPOUT_RELEASE_US      30000


// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static void ticker_append(
    char* destination,
    size_t destination_size,
    const char* text
)
{
    size_t used =
        strlen(destination);


    if (used >= destination_size - 1) {
        return;
    }


    snprintf(
        destination + used,
        destination_size - used,
        "%s",
        text
    );
}


// ============================================================================
// DIAGNOSTICS UPDATE
// ============================================================================
//
// ignore_mask allows menu navigation buttons to be temporarily excluded.
//
// SELECT is intentionally NOT excluded by RetroPad Link's caller.
//
// That allows SELECT to continue being tested for chatter/dropout while also
// acting as the menu navigation modifier.
//

bool diagnostics_update(
    uint32_t buttons,
    uint32_t ignore_mask
)
{
    bool changed = false;

    uint64_t now =
        time_us_64();


    for (size_t i = 0; i < BUTTON_DIAGNOSTIC_COUNT; i++) {

        button_diagnostic_t* diag =
            &button_diagnostics[i];


        bool pressed =
            (buttons & diag->mask) != 0;


        // --------------------------------------------------------------------
        // IGNORE COMMAND / NAVIGATION INPUT
        // --------------------------------------------------------------------

        if (ignore_mask & diag->mask) {

            // Keep physical state synchronized while this button is being used
            // for a UI command. This prevents the command itself from being
            // interpreted as chatter or dropout.

            diag->last_state =
                pressed;

            diag->last_transition_us =
                0;

            diag->rapid_transition_count =
                0;

            diag->press_started_us =
                0;

            diag->release_started_us =
                0;

            diag->previous_press_duration_us =
                0;

            continue;
        }


        // Nothing changed.
        if (pressed == diag->last_state) {
            continue;
        }


        // --------------------------------------------------------------------
        // CHATTER DETECTION
        // --------------------------------------------------------------------

        if (diag->last_transition_us != 0) {

            uint64_t transition_time =
                now - diag->last_transition_us;


            if (transition_time <= CHATTER_WINDOW_US) {

                if (diag->rapid_transition_count < 255) {
                    diag->rapid_transition_count++;
                }
            }

            else {
                diag->rapid_transition_count = 1;
            }


            if (
                diag->rapid_transition_count >= CHATTER_TRANSITIONS &&
                !diag->chatter
            ) {

                diag->chatter =
                    true;

                changed =
                    true;
            }
        }

        else {
            diag->rapid_transition_count = 1;
        }


        // --------------------------------------------------------------------
        // BUTTON PRESSED
        // --------------------------------------------------------------------

        if (pressed) {

            if (!diag->seen) {

                diag->seen =
                    true;

                changed =
                    true;
            }


            // A short release following a long deliberate hold is suspicious.

            if (
                diag->release_started_us != 0 &&
                diag->previous_press_duration_us >= DROPOUT_MIN_HOLD_US
            ) {

                uint64_t release_duration =
                    now - diag->release_started_us;


                if (
                    release_duration <= DROPOUT_RELEASE_US &&
                    !diag->dropout
                ) {

                    diag->dropout =
                        true;

                    changed =
                        true;
                }
            }


            diag->press_started_us =
                now;

            diag->release_started_us =
                0;
        }


        // --------------------------------------------------------------------
        // BUTTON RELEASED
        // --------------------------------------------------------------------

        else {

            if (diag->press_started_us != 0) {

                diag->previous_press_duration_us =
                    now - diag->press_started_us;
            }


            diag->press_started_us =
                0;

            diag->release_started_us =
                now;
        }


        diag->last_state =
            pressed;

        diag->last_transition_us =
            now;
    }


    return changed;
}


// ============================================================================
// DIAGNOSTICS RESET
// ============================================================================

void diagnostics_reset(void)
{
    memset(
        button_diagnostics,
        0,
        sizeof(button_diagnostics)
    );


    for (size_t i = 0; i < BUTTON_DIAGNOSTIC_COUNT; i++) {

        button_diagnostics[i].mask =
            active_layout->buttons[i].mask;

        button_diagnostics[i].name =
            active_layout->buttons[i].name;
    }
}


// ============================================================================
// LAYOUT
// ============================================================================

void diagnostics_set_layout(diagnostics_layout_t layout)
{
    if ((size_t)layout < sizeof(layouts) / sizeof(layouts[0])) {

        active_layout =
            &layouts[layout];
    }


    diagnostics_reset();
}


uint8_t diagnostics_get_button_count(void)
{
    return active_layout->count;
}


const char* diagnostics_get_reset_hint(void)
{
    return active_layout->reset_hint;
}


// ============================================================================
// DIAGNOSTIC COUNTS
// ============================================================================

void diagnostics_get_counts(
    uint8_t* pass_count,
    uint8_t* chatter_count,
    uint8_t* dropout_count
)
{
    uint8_t pass = 0;
    uint8_t chatter = 0;
    uint8_t dropout = 0;


    for (size_t i = 0; i < BUTTON_DIAGNOSTIC_COUNT; i++) {

        const button_diagnostic_t* diag =
            &button_diagnostics[i];


        if (
            diag->seen &&
            !diag->chatter &&
            !diag->dropout
        ) {

            pass++;
        }


        if (diag->chatter) {
            chatter++;
        }


        if (diag->dropout) {
            dropout++;
        }
    }


    if (pass_count) {
        *pass_count = pass;
    }


    if (chatter_count) {
        *chatter_count = chatter;
    }


    if (dropout_count) {
        *dropout_count = dropout;
    }
}


// ============================================================================
// DIAGNOSTIC TICKER
// ============================================================================

void diagnostics_build_ticker(
    char* destination,
    size_t destination_size
)
{
    if (
        destination == NULL ||
        destination_size == 0
    ) {

        return;
    }


    destination[0] =
        '\0';


    bool have_chatter =
        false;

    bool have_dropout =
        false;


    // ------------------------------------------------------------------------
    // CHATTER BUTTONS
    // ------------------------------------------------------------------------

    for (size_t i = 0; i < BUTTON_DIAGNOSTIC_COUNT; i++) {

        if (button_diagnostics[i].chatter) {

            if (!have_chatter) {

                ticker_append(
                    destination,
                    destination_size,
                    "CHAT:"
                );

                have_chatter =
                    true;
            }

            else {

                ticker_append(
                    destination,
                    destination_size,
                    " "
                );
            }


            ticker_append(
                destination,
                destination_size,
                button_diagnostics[i].name
            );
        }
    }


    // ------------------------------------------------------------------------
    // DROPOUT BUTTONS
    // ------------------------------------------------------------------------

    for (size_t i = 0; i < BUTTON_DIAGNOSTIC_COUNT; i++) {

        if (button_diagnostics[i].dropout) {

            if (!have_dropout) {

                if (have_chatter) {

                    ticker_append(
                        destination,
                        destination_size,
                        " | "
                    );
                }


                ticker_append(
                    destination,
                    destination_size,
                    "DROP:"
                );


                have_dropout =
                    true;
            }

            else {

                ticker_append(
                    destination,
                    destination_size,
                    " "
                );
            }


            ticker_append(
                destination,
                destination_size,
                button_diagnostics[i].name
            );
        }
    }


    // ------------------------------------------------------------------------
    // BUTTONS NOT PRESSED YET
    // ------------------------------------------------------------------------

    bool have_untested =
        false;


    for (size_t i = 0; i < BUTTON_DIAGNOSTIC_COUNT; i++) {

        if (!button_diagnostics[i].seen) {

            if (!have_untested) {

                if (have_chatter || have_dropout) {

                    ticker_append(
                        destination,
                        destination_size,
                        " | "
                    );
                }


                ticker_append(
                    destination,
                    destination_size,
                    "PRESS:"
                );


                have_untested =
                    true;
            }

            else {

                ticker_append(
                    destination,
                    destination_size,
                    " "
                );
            }


            ticker_append(
                destination,
                destination_size,
                button_diagnostics[i].name
            );
        }
    }


    // ------------------------------------------------------------------------
    // NOTHING DETECTED
    // ------------------------------------------------------------------------

    if (!have_chatter && !have_dropout && !have_untested) {

        snprintf(
            destination,
            destination_size,
            "NO FAULTS DETECTED"
        );
    }
}
