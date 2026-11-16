// app.c - RetroPad Link application entry point

#include "app.h"
#include "version.h"
#include "profiles.h"
#include "diagnostics.h"
#include "joystick_diag.h"
#include "ui.h"
#include "controller_state.h"

#include "controllers/snes.h"
#include "controllers/nes.h"
#include "controllers/n64.h"
#include "controllers/gamecube.h"
#include "controllers/controller_manager.h"


#include "core/router/router.h"
#include "core/services/players/manager.h"
#include "core/services/profiles/profile.h"
#include "core/services/players/feedback.h"

#include "core/input_interface.h"
#include "core/output_interface.h"
#include "core/buttons.h"

#include "usb/usbd/usbd.h"

#include "core/services/leds/leds.h"
#include "core/services/display/display.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>


// ============================================================================
// APP INPUT INTERFACES
// ============================================================================
//
// JoypadOS sees ONE input interface:
//
//     RetroPad Link Controller Manager
//
// The controller manager decides whether SNES or GameCube is actively polled.
// This prevents expensive GameCube Joybus polling from adding latency while an
// SNES controller is in use.
//

const InputInterface** app_get_input_interfaces(uint8_t* count)
{
    static const InputInterface* input_interfaces[1];


    input_interfaces[0] =
        retropad_controller_manager_get_input_interface();


    *count = 1;


    return input_interfaces;
}


// ============================================================================
// APP OUTPUT INTERFACES
// ============================================================================

static const OutputInterface* output_interfaces[] = {
    &usbd_output_interface,
};


const OutputInterface** app_get_output_interfaces(uint8_t* count)
{
    *count =
        sizeof(output_interfaces) /
        sizeof(output_interfaces[0]);


    return output_interfaces;
}


// ============================================================================
// UI STATE
// ============================================================================

typedef enum {
    UI_WAITING,
    UI_CONNECTED,
    UI_TESTING
} ui_state_t;


static ui_state_t ui_state =
    UI_WAITING;


static test_page_t test_page =
    TEST_PAGE_LIVE;


// True while the Rumble page is driving the motor.
static bool rumble_test_on =
    false;


// ============================================================================
// PAGE NAVIGATION
// ============================================================================

// Both skip pages the connected controller does not have.

static test_page_t page_next(test_page_t page)
{
    int next =
        (int)page;


    do {

        next =
            (next + 1) %
            TEST_PAGE_COUNT;
    }
    while (!ui_page_available((test_page_t)next));


    return (test_page_t)next;
}


static test_page_t page_previous(test_page_t page)
{
    int previous =
        (int)page;


    do {

        previous =
            (previous + TEST_PAGE_COUNT - 1) %
            TEST_PAGE_COUNT;
    }
    while (!ui_page_available((test_page_t)previous));


    return (test_page_t)previous;
}


// Stick drawn on a page, or -1 if the page shows no stick.
static int8_t page_stick(test_page_t page)
{
    switch (page) {

        case TEST_PAGE_JOYSTICKS:
            return 0;

        case TEST_PAGE_JOYSTICK2:
            return 1;

        default:
            return -1;
    }
}


// ============================================================================
// CONTROLLER-AWARE CONNECTED SCREEN
// ============================================================================

static void retropad_draw_connected_screen(void)
{
    retropad_controller_type_t active =
        retropad_controller_manager_get_active();


    if (active == RETROPAD_CONTROLLER_NES) {

        ui_draw_nes_connected_screen();
    }

    else if (active == RETROPAD_CONTROLLER_N64) {

        ui_draw_n64_connected_screen();
    }

    else if (active == RETROPAD_CONTROLLER_GAMECUBE) {

        ui_draw_gc_connected_screen();
    }

    else {

        ui_draw_connected_screen();
    }
}


// ============================================================================
// CONTROLLER-AWARE LIVE TEST SCREEN
// ============================================================================

static void retropad_draw_live_screen(uint32_t buttons)
{
    if (
        retropad_controller_manager_get_active() ==
        RETROPAD_CONTROLLER_NES
    ) {

        ui_draw_nes_live_screen(
            buttons
        );
    }

    else if (
        retropad_controller_manager_get_active() ==
        RETROPAD_CONTROLLER_N64
    ) {

        /*
         * N64 C-buttons are represented two ways by JoypadOS:
         *
         * C-Up    -> L3 and right-stick Y=0
         * C-Down  -> B2 and right-stick Y=255
         * C-Left  -> B4 and right-stick X=0
         * C-Right -> R3 and right-stick X=255
         *
         * Use the analog representation as a fallback for the OLED tester.
         */

        uint8_t rx =
            controller_state_get_analog(
                ANALOG_RX
            );

        uint8_t ry =
            controller_state_get_analog(
                ANALOG_RY
            );


        uint32_t display_buttons =
            buttons;


        if (ry < 64) {
            display_buttons |=
                JP_BUTTON_L3;
        }

        if (ry > 192) {
            display_buttons |=
                JP_BUTTON_B2;
        }

        if (rx < 64) {
            display_buttons |=
                JP_BUTTON_B4;
        }

        if (rx > 192) {
            display_buttons |=
                JP_BUTTON_R3;
        }


        ui_draw_n64_live_screen(
            display_buttons,
            controller_state_get_analog(ANALOG_LX),
            controller_state_get_analog(ANALOG_LY)
        );
    }

    else if (
        retropad_controller_manager_get_active() ==
        RETROPAD_CONTROLLER_GAMECUBE
    ) {

        ui_draw_gc_live_screen(
            buttons,
            controller_state_get_analog(ANALOG_LX),
            controller_state_get_analog(ANALOG_LY),
            controller_state_get_analog(ANALOG_RX),
            controller_state_get_analog(ANALOG_RY),
            controller_state_get_analog(ANALOG_L2),
            controller_state_get_analog(ANALOG_R2)
        );
    }

    else {

        ui_draw_live_screen(
            buttons
        );
    }
}


// ============================================================================
// JOYSTICK INPUT
// ============================================================================

// Stick profile for the active controller, or NULL if it has no stick.
static const joystick_profile_t* retropad_stick_profile(
    retropad_controller_type_t active
)
{
    switch (active) {

        case RETROPAD_CONTROLLER_N64:
            return &joystick_profile_n64;

        case RETROPAD_CONTROLLER_GAMECUBE:
            return &joystick_profile_gc_main;

        default:
            return NULL;
    }
}


// Profile for a controller's second stick, or NULL if it has only one.
static const joystick_profile_t* retropad_second_stick_profile(
    retropad_controller_type_t active
)
{
    switch (active) {

        case RETROPAD_CONTROLLER_GAMECUBE:
            return &joystick_profile_gc_cstick;

        default:
            return NULL;
    }
}


// Exact inverse of n64_host.c convert_stick_axis(), which computes
// trunc(raw * 127 / 84). Recovers every raw value in -84..+84.
static int16_t n64_unscale_axis(int16_t scaled)
{
    int16_t magnitude =
        (int16_t)((abs(scaled) * 84 + 126) / 127);


    return scaled < 0 ? -magnitude : magnitude;
}


// Read every stick in the active controller's native units (+Y up).
//
// Returns how many sticks were read (0 if the controller has none).
static uint8_t retropad_read_sticks(
    retropad_controller_type_t active,
    int16_t x[JOYSTICK_DIAG_MAX_STICKS],
    int16_t y[JOYSTICK_DIAG_MAX_STICKS]
)
{
    if (active == RETROPAD_CONTROLLER_GAMECUBE) {

        // Raw values from the driver, before its auto-calibration.
        retropad_gamecube_get_sticks(
            &x[0],
            &y[0],
            &x[1],
            &y[1]
        );

        return 2;
    }


    if (active != RETROPAD_CONTROLLER_N64) {
        return 0;
    }


    // JoypadOS scales the raw N64 stick (+/-84) up to the 0-255 HID range
    // (Y inverted, values past 84 clipped). Undo that so the test reports
    // raw N64 units. Anything shown as 84 means "84 or more".

    int16_t hid_x =
        (int16_t)controller_state_get_analog(ANALOG_LX) - 128;

    int16_t hid_y =
        (int16_t)controller_state_get_analog(ANALOG_LY) - 128;


    x[0] = n64_unscale_axis(hid_x);
    y[0] = n64_unscale_axis(-hid_y);


    return 1;
}


// ============================================================================
// CONTROLLER-AWARE JOYSTICK TEST SCREEN
// ============================================================================

// Only reachable for controllers with a stick (see ui_set_optional_pages).
static void retropad_draw_joystick_screen(uint8_t stick)
{
    ui_draw_joystick_test_screen(
        stick
    );
}


// ============================================================================
// CONTROLLER-AWARE PAGE DISPATCH
// ============================================================================

static void retropad_draw_current_page(
    test_page_t page,
    uint32_t buttons,
    uint16_t ticker_offset
)
{
    switch (page) {

        case TEST_PAGE_LIVE:

            retropad_draw_live_screen(
                buttons
            );

            break;


        case TEST_PAGE_JOYSTICKS:

            retropad_draw_joystick_screen(
                0
            );

            break;


        case TEST_PAGE_JOYSTICK2:

            retropad_draw_joystick_screen(
                1
            );

            break;


        case TEST_PAGE_RUMBLE:

            ui_draw_rumble_screen(
                rumble_test_on
            );

            break;


        default:

            ui_draw_current_test_page(
                page,
                buttons,
                ticker_offset
            );

            break;
    }
}


// ============================================================================
// APP INITIALIZATION
// ============================================================================

void app_init(void)
{
    set_sys_clock_khz(130000, true);
    stdio_init_all();


    printf(
        "[app:retropad-link] Initializing RetroPad Link v%s (JoypadOS %s)\n",
        RETROPAD_LINK_VERSION,
        JOYPAD_VERSION
    );


    // ------------------------------------------------------------------------
    // OLED
    // ------------------------------------------------------------------------

    display_i2c_config_t oled_cfg = {
        .i2c_inst = 0,
        .pin_sda = 8,
        .pin_scl = 9,
        .addr = 0x3C,
    };


    display_init_sh1106_i2c(
        &oled_cfg
    );


    // Leave the display in JoypadOS's default async mode.
    //
    // ui_draw_*() only renders into RAM. display_task() at the end of
    // app_task() then sends one 8-pixel page per loop (~3 ms), instead of
    // blocking controller polling and USB for a ~25 ms full-frame transfer.

    ui_draw_waiting_screen();


    // ------------------------------------------------------------------------
    // CONTROLLER INITIALIZATION
    // ------------------------------------------------------------------------
    //
    // Must happen before JoypadOS initializes the controller manager, which
    // then polls through these adapters. Pins come from config.h.
    //

    retropad_snes_init();

    // NES shares Clock / Latch with SNES, so it must follow SNES.
    retropad_nes_init();

    retropad_n64_init();

    retropad_gamecube_init();


    // ------------------------------------------------------------------------
    // ROUTER
    // ------------------------------------------------------------------------

    router_config_t router_cfg = {

        .mode =
            ROUTING_MODE,

        .merge_mode =
            MERGE_MODE,

        .max_players_per_output = {
            [OUTPUT_TARGET_USB_DEVICE] =
                USB_OUTPUT_PORTS,
        },

        .merge_all_inputs =
            false,

        .transform_flags =
            TRANSFORM_NONE,

        .mouse_drain_rate =
            8,
    };


    router_init(
        &router_cfg
    );


    // Cache routed controller state without consuming USB output events.
    controller_state_init();


    // SNES / NES -> USB

    router_add_route(
        INPUT_SOURCE_NATIVE_SNES,
        OUTPUT_TARGET_USB_DEVICE,
        0
    );

    // N64 -> USB

    router_add_route(
        INPUT_SOURCE_NATIVE_N64,
        OUTPUT_TARGET_USB_DEVICE,
        0
    );


    // GameCube -> USB

    router_add_route(
        INPUT_SOURCE_NATIVE_GC,
        OUTPUT_TARGET_USB_DEVICE,
        0
    );


    // ------------------------------------------------------------------------
    // PLAYER MANAGEMENT
    // ------------------------------------------------------------------------

    player_config_t player_cfg = {

        .slot_mode =
            PLAYER_SLOT_MODE,

        .max_slots =
            MAX_PLAYER_SLOTS,

        .auto_assign_on_press =
            AUTO_ASSIGN_ON_PRESS,
    };


    players_init_with_config(
        &player_cfg
    );


    // ------------------------------------------------------------------------
    // PROFILE SYSTEM
    // ------------------------------------------------------------------------
    //
    // SNES profile set remains active for now.
    //
    // Once the controller manager/UI abstraction is complete, we'll decide
    // whether RetroPad Link should have its own shared profile set.
    //

    static const profile_config_t profile_cfg = {

        .output_profiles = {
            NULL
        },

        .shared_profiles =
            &snes2usb_profile_set,
    };


    profile_init(
        &profile_cfg
    );


    printf(
        "[app:retropad-link] Initialization complete\n"
    );


    printf(
        "[app:retropad-link] Controller manager enabled\n"
    );


    printf(
        "[app:retropad-link] SNES pins: CLK=%d LATCH=%d D0=%d D1=%d IO=%d\n",
        SNES_PIN_CLOCK,
        SNES_PIN_LATCH,
        SNES_PIN_DATA0,
        SNES_PIN_DATA1,
        SNES_PIN_IOBIT
    );


    printf(
        "[app:retropad-link] GameCube data pin: GPIO%d\n",
        RETROPAD_GAMECUBE_DATA_GPIO
    );
}


// ============================================================================
// APP TASK
// ============================================================================

void app_task(void)
{
    static uint64_t test_hold_start =
        0;


    static uint64_t diag_reset_hold_start =
        0;


    static uint32_t last_buttons =
        0xFFFFFFFF;


    // Previous D-pad state while SELECT is held.
    static uint32_t last_nav_buttons =
        0;


    // Diagnostics ticker.
    static uint16_t diag_ticker_offset =
        0;


    static uint64_t diag_ticker_last_us =
        0;


    // "Diagnostics reset" banner stays up until this time.
    // Non-blocking replacement for sleep_ms().
    static uint64_t diag_reset_banner_until_us =
        0;


    // Joystick page redraw throttle. Stick movement can change every loop;
    // the OLED only needs ~30 frames per second.
    static uint64_t stick_last_draw_us =
        0;


    static bool stick_redraw_pending =
        false;


    // ========================================================================
    // ACTIVE CONTROLLER
    // ========================================================================
    //
    // The controller manager determines which protocol is actively being
    // polled.

    retropad_controller_type_t active_controller =
        retropad_controller_manager_get_active();


    bool snes_active =
        active_controller ==
        RETROPAD_CONTROLLER_SNES;


    bool nes_active =
        active_controller ==
        RETROPAD_CONTROLLER_NES;


    // N64 and GameCube have no SELECT button; Z (JP_BUTTON_R1 on both)
    // fills that role.
    bool z_modifier =
        active_controller == RETROPAD_CONTROLLER_N64 ||
        active_controller == RETROPAD_CONTROLLER_GAMECUBE;


    bool connected =
        retropad_controller_manager_is_connected();


    // Pull the latest routed USB event into controller_state. Must run
    // before any controller_state_get_*() call this iteration.
    // True when a new event arrived since the last loop.
    bool controller_event =
        controller_state_update();


    // SNES / NES are read straight from their readers. N64 / GameCube come
    // from the routed USB event (controller_state).
    uint32_t buttons =
        snes_active
            ? retropad_snes_get_buttons()
            : nes_active
                ? retropad_nes_get_buttons()
                : (
                    z_modifier
                        ? controller_state_get_buttons()
                        : 0
                );


    bool select_held =
        z_modifier
            ? ((buttons & JP_BUTTON_R1) != 0)
            : ((buttons & JP_BUTTON_S1) != 0);


    bool start_held =
        (buttons & JP_BUTTON_S2) != 0;


    // ========================================================================
    // RUMBLE TEST
    // ========================================================================
    //
    // On the Rumble page the motor runs only while A is held. Leaving the
    // page, leaving test mode or unplugging stops it.

    bool rumble_test_want =
        ui_state == UI_TESTING &&
        test_page == TEST_PAGE_RUMBLE &&
        connected &&
        active_controller == RETROPAD_CONTROLLER_GAMECUBE &&
        (buttons & JP_BUTTON_B2) != 0;


    if (rumble_test_want != rumble_test_on) {

        rumble_test_on =
            rumble_test_want;

        retropad_gamecube_set_rumble(
            rumble_test_on
        );


        if (
            ui_state == UI_TESTING &&
            test_page == TEST_PAGE_RUMBLE
        ) {

            ui_draw_rumble_screen(
                rumble_test_on
            );
        }
    }


    bool entry_combo =
        select_held &&
        start_held;


    bool diag_reset_combo =
        select_held &&
        start_held &&
        (buttons & JP_BUTTON_DD);


    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    bool diagnostics_changed =
        false;


    if (connected) {

        uint32_t diagnostic_ignore_mask =
            0;


        // SELECT remains monitored.
        //
        // Only the D-pad is ignored while SELECT is being used as the
        // navigation modifier.

        if (
            ui_state == UI_TESTING &&
            select_held
        ) {

            diagnostic_ignore_mask =
                JP_BUTTON_DU |
                JP_BUTTON_DD |
                JP_BUTTON_DL |
                JP_BUTTON_DR;
        }


        // START participates in the diagnostics-reset command.
        //
        // Ignore START during that command so the reset gesture does not
        // contaminate diagnostics.

        if (diag_reset_combo) {

            diagnostic_ignore_mask |=
                JP_BUTTON_S2;
        }


        diagnostics_changed =
            diagnostics_update(
                buttons,
                diagnostic_ignore_mask
            );
    }


    // ========================================================================
    // JOYSTICK DIAGNOSTICS
    // ========================================================================
    //
    // Tracked on every page so the Joysticks page already has data when
    // the user arrives there.

    const joystick_profile_t* stick_profile =
        connected
            ? retropad_stick_profile(active_controller)
            : NULL;


    if (stick_profile) {

        int16_t stick_x[JOYSTICK_DIAG_MAX_STICKS];
        int16_t stick_y[JOYSTICK_DIAG_MAX_STICKS];


        uint8_t stick_count =
            retropad_read_sticks(
                active_controller,
                stick_x,
                stick_y
            );


        for (uint8_t i = 0; i < stick_count; i++) {

            // Redraw only if this stick is on screen: its own stick page,
            // or the LIVE page (N64 stick box, GameCube sticks).
            if (
                joystick_diag_update(
                    i,
                    stick_x[i],
                    stick_y[i]
                ) &&
                (
                    test_page == TEST_PAGE_LIVE ||
                    (int8_t)i == page_stick(test_page)
                )
            ) {

                stick_redraw_pending =
                    true;
            }
        }
    }


    // ========================================================================
    // CONTROLLER CONNECTION STATE
    // ========================================================================

    if (!connected) {

        controller_state_clear();

        if (ui_state != UI_WAITING) {

            ui_state =
                UI_WAITING;

            test_page =
                TEST_PAGE_LIVE;

            test_hold_start =
                0;

            diag_reset_hold_start =
                0;

            last_nav_buttons =
                0;

            last_buttons =
                0xFFFFFFFF;

            diag_ticker_offset =
                0;

            diag_ticker_last_us =
                0;

            diag_reset_banner_until_us =
                0;

            diagnostics_reset();


            // Draw once on disconnect. app_init() draws the initial one.

            ui_draw_waiting_screen();
        }
    }


    // ========================================================================
    // CONTROLLER JUST CONNECTED
    // ========================================================================

    else if (ui_state == UI_WAITING) {

        ui_state =
            UI_CONNECTED;

        test_hold_start =
            0;

        last_nav_buttons =
            0;

        // Test the buttons this controller actually has. Also resets.
        diagnostics_set_layout(
            active_controller == RETROPAD_CONTROLLER_N64
                ? DIAGNOSTICS_LAYOUT_N64
                : active_controller == RETROPAD_CONTROLLER_GAMECUBE
                    ? DIAGNOSTICS_LAYOUT_GAMECUBE
                    : active_controller == RETROPAD_CONTROLLER_NES
                        ? DIAGNOSTICS_LAYOUT_NES
                        : DIAGNOSTICS_LAYOUT_SNES
        );

        joystick_diag_reset(
            stick_profile,
            retropad_second_stick_profile(
                active_controller
            )
        );

        // Stick pages only for controllers with sticks (N64, GameCube);
        // GameCube also gets C-stick and Rumble pages.
        ui_set_optional_pages(
            stick_profile != NULL,
            retropad_second_stick_profile(active_controller) != NULL,
            active_controller == RETROPAD_CONTROLLER_GAMECUBE
        );


        retropad_draw_connected_screen();
    }


    // ========================================================================
    // CONNECTED SCREEN
    // ========================================================================

    else if (ui_state == UI_CONNECTED) {

        // Hold START + SELECT for 3 seconds to enter Test Mode.

        if (entry_combo) {

            if (test_hold_start == 0) {

                test_hold_start =
                    time_us_64();
            }


            if (
                (time_us_64() - test_hold_start)
                >= 3000000
            ) {

                ui_state =
                    UI_TESTING;


                test_page =
                    TEST_PAGE_LIVE;


                // Entry combo must not contaminate diagnostics.

                diagnostics_reset();


                last_buttons =
                    0xFFFFFFFF;


                last_nav_buttons =
                    0;


                diag_ticker_offset =
                    0;


                diag_ticker_last_us =
                    time_us_64();


                test_hold_start =
                    0;


                retropad_draw_live_screen(
                    buttons
                );
            }
        }

        else {

            test_hold_start =
                0;
        }
    }


    // ========================================================================
    // TEST MODE
    // ========================================================================

    else if (ui_state == UI_TESTING) {

        uint32_t nav_buttons =
            buttons &
            (
                JP_BUTTON_DU |
                JP_BUTTON_DD |
                JP_BUTTON_DL |
                JP_BUTTON_DR
            );


        uint32_t new_nav_presses =
            nav_buttons &
            ~last_nav_buttons;


        // --------------------------------------------------------------------
        // DIAGNOSTICS / JOYSTICK RESET
        //
        // START + SELECT + DOWN
        // Hold for three seconds while on Diagnostics (clears button
        // diagnostics) or Joysticks (clears the stick test).
        // --------------------------------------------------------------------

        bool diagnostics_reset_in_progress =
            false;


        bool reset_page =
            test_page == TEST_PAGE_DIAGNOSTICS ||
            (
                page_stick(test_page) >= 0 &&
                stick_profile != NULL
            );


        if (
            reset_page &&
            diag_reset_combo
        ) {

            diagnostics_reset_in_progress =
                true;


            if (diag_reset_hold_start == 0) {

                diag_reset_hold_start =
                    time_us_64();
            }


            if (
                (time_us_64() - diag_reset_hold_start)
                >= 3000000
            ) {

                diag_reset_hold_start =
                    0;


                if (page_stick(test_page) >= 0) {

                    joystick_diag_reset(
                        stick_profile,
                        retropad_second_stick_profile(
                            active_controller
                        )
                    );


                    ui_draw_joystick_reset_screen();
                }

                else {

                    diagnostics_reset();


                    diag_ticker_offset =
                        0;


                    diag_ticker_last_us =
                        time_us_64();


                    ui_draw_diagnostics_reset_screen();
                }


                // The page redraws itself once the banner has been
                // visible for 500 ms.

                diag_reset_banner_until_us =
                    time_us_64() + 500000;


                last_buttons =
                    0xFFFFFFFF;


                last_nav_buttons =
                    nav_buttons;


                return;
            }
        }

        else {

            diag_reset_hold_start =
                0;
        }


        // --------------------------------------------------------------------
        // MENU NAVIGATION
        //
        // SELECT + RIGHT = next
        // SELECT + LEFT  = previous
        // SELECT + UP    = LIVE
        // SELECT + DOWN  = exit
        // --------------------------------------------------------------------

        if (
            select_held &&
            !diagnostics_reset_in_progress
        ) {

            // EXIT

            if (
                new_nav_presses &
                JP_BUTTON_DD
            ) {

                ui_state =
                    UI_CONNECTED;


                test_page =
                    TEST_PAGE_LIVE;


                last_nav_buttons =
                    0;


                last_buttons =
                    0xFFFFFFFF;


                diag_ticker_offset =
                    0;


                retropad_draw_connected_screen();


                return;
            }


            // LIVE

            else if (
                new_nav_presses &
                JP_BUTTON_DU
            ) {

                if (
                    test_page !=
                    TEST_PAGE_LIVE
                ) {

                    test_page =
                        TEST_PAGE_LIVE;


                    last_buttons =
                        0xFFFFFFFF;


                    retropad_draw_live_screen(
                        buttons
                    );
                }
            }


            // NEXT

            else if (
                new_nav_presses &
                JP_BUTTON_DR
            ) {

                test_page =
                    page_next(
                        test_page
                    );


                last_buttons =
                    0xFFFFFFFF;


                diag_ticker_offset =
                    0;


                diag_ticker_last_us =
                    time_us_64();


                retropad_draw_current_page(
                    test_page,
                    buttons,
                    diag_ticker_offset
                );
            }


            // PREVIOUS

            else if (
                new_nav_presses &
                JP_BUTTON_DL
            ) {

                test_page =
                    page_previous(
                        test_page
                    );


                last_buttons =
                    0xFFFFFFFF;


                diag_ticker_offset =
                    0;


                diag_ticker_last_us =
                    time_us_64();


                retropad_draw_current_page(
                    test_page,
                    buttons,
                    diag_ticker_offset
                );
            }


            last_nav_buttons =
                nav_buttons;
        }

        else if (!select_held) {

            last_nav_buttons =
                0;
        }

        else {

            last_nav_buttons =
                nav_buttons;
        }


        // --------------------------------------------------------------------
        // LIVE
        // --------------------------------------------------------------------

        if (
            test_page ==
            TEST_PAGE_LIVE
        ) {

            uint64_t now =
                time_us_64();


            // Button changes redraw immediately. Stick / trigger movement
            // (N64 stick box, GameCube sticks and triggers) redraws at most
            // ~30 times per second.

            bool analog_changed =
                stick_redraw_pending ||
                (
                    active_controller == RETROPAD_CONTROLLER_GAMECUBE &&
                    controller_event
                );


            bool stick_due =
                analog_changed &&
                (now - stick_last_draw_us) >= 33000;


            if (
                buttons != last_buttons ||
                stick_due
            ) {

                last_buttons =
                    buttons;


                stick_redraw_pending =
                    false;


                stick_last_draw_us =
                    now;


                retropad_draw_live_screen(
                    buttons
                );
            }
        }


        // --------------------------------------------------------------------
        // JOYSTICKS / JOYSTICK 2
        // --------------------------------------------------------------------

        else if (
            page_stick(test_page) >= 0
        ) {

            if (stick_profile) {

                uint64_t now =
                    time_us_64();


                // Hold the "cleared" banner, then force a full redraw.

                if (diag_reset_banner_until_us != 0) {

                    if (now >= diag_reset_banner_until_us) {

                        diag_reset_banner_until_us =
                            0;

                        stick_redraw_pending =
                            true;

                        stick_last_draw_us =
                            0;
                    }
                }

                else if (
                    stick_redraw_pending &&
                    (now - stick_last_draw_us) >= 33000
                ) {

                    stick_redraw_pending =
                        false;

                    stick_last_draw_us =
                        now;

                    ui_draw_joystick_test_screen(
                        (uint8_t)page_stick(test_page)
                    );
                }
            }
        }


        // --------------------------------------------------------------------
        // DIAGNOSTICS
        // --------------------------------------------------------------------

        else if (
            test_page ==
            TEST_PAGE_DIAGNOSTICS
        ) {

            uint64_t now =
                time_us_64();


            // Keep the "reset" banner on screen until it expires, then
            // return to the normal Diagnostics page.

            if (diag_reset_banner_until_us != 0) {

                if (now >= diag_reset_banner_until_us) {

                    diag_reset_banner_until_us =
                        0;


                    diag_ticker_last_us =
                        now;


                    ui_draw_diagnostics_screen(
                        diag_ticker_offset
                    );
                }
            }

            else {

                bool ticker_advanced =
                    false;


                if (
                    diag_ticker_last_us == 0 ||
                    (now - diag_ticker_last_us)
                        >= 250000
                ) {

                    diag_ticker_last_us =
                        now;


                    diag_ticker_offset++;


                    ticker_advanced =
                        true;
                }


                if (
                    diagnostics_changed ||
                    ticker_advanced
                ) {

                    ui_draw_diagnostics_screen(
                        diag_ticker_offset
                    );
                }
            }
        }


        // --------------------------------------------------------------------
        // RAW
        // --------------------------------------------------------------------

        else if (
            test_page ==
            TEST_PAGE_RAW
        ) {

            if (
                buttons !=
                last_buttons
            ) {

                last_buttons =
                    buttons;


                ui_draw_raw_screen(
                    buttons
                );
            }
        }
    }


    // ========================================================================
    // USB MODE LED
    // ========================================================================

    static usb_output_mode_t last_led_mode =
        USB_OUTPUT_MODE_COUNT;


    usb_output_mode_t mode =
        usbd_get_mode();


    if (
        mode !=
        last_led_mode
    ) {

        uint8_t r;
        uint8_t g;
        uint8_t b;


        usbd_get_mode_color(
            mode,
            &r,
            &g,
            &b
        );


        leds_set_color(
            r,
            g,
            b
        );


        last_led_mode =
            mode;
    }


    // ========================================================================
    // USB FEEDBACK
    // ========================================================================

    if (
        usbd_output_interface
            .get_feedback
    ) {

        output_feedback_t fb;


        if (
            usbd_output_interface
                .get_feedback(&fb) &&
            fb.dirty
        ) {

            feedback_set_rumble(
                0,
                fb.rumble_left,
                fb.rumble_right
            );
        }
    }


    // ========================================================================
    // APPLY CONTROLLER FEEDBACK
    // ========================================================================

    feedback_state_t* state =
        feedback_get_state(0);


    if (
        state &&
        state->rumble_dirty
    ) {

        switch (active_controller) {

            // ----------------------------------------------------------------
            // SNES
            // ----------------------------------------------------------------

            case RETROPAD_CONTROLLER_SNES:

                retropad_snes_set_rumble(
                    state->rumble.left,
                    state->rumble.right
                );

                break;


            // ----------------------------------------------------------------
            // GAMECUBE
            // ----------------------------------------------------------------

            case RETROPAD_CONTROLLER_GAMECUBE:

                retropad_gamecube_set_rumble(
                    state->rumble.left > 0 ||
                    state->rumble.right > 0
                );

                break;


            // ----------------------------------------------------------------
            // NONE
            // ----------------------------------------------------------------

            case RETROPAD_CONTROLLER_NONE:

            default:

                break;
        }


        feedback_clear_dirty(
            0
        );
    }


    display_task();
}
