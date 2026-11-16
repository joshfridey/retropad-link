// ui.c - RetroPad Link OLED user interface

#include "ui.h"
#include "diagnostics.h"
#include "joystick_diag.h"
#include "snes_logo.h"
#include "nes_logo.h"
#include "n64-logo.h"
#include "gc_cube_logo.h"
#include "controllers/snes.h"

#include "core/buttons.h"
#include "core/services/display/display.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// ============================================================================
// PAGE INDICATOR
// ============================================================================
//
// Current page is represented by *
// Other pages are represented by .
//
// LIVE         *...
// JOYSTICKS    .*..
// DIAGNOSTICS  ..*.
// RAW          ...*
//

static bool stick_page =
    false;

static bool second_stick_page =
    false;

static bool rumble_page =
    false;


void ui_set_optional_pages(
    bool stick,
    bool second_stick,
    bool rumble
)
{
    stick_page =
        stick;

    second_stick_page =
        second_stick;

    rumble_page =
        rumble;
}


bool ui_page_available(test_page_t page)
{
    switch (page) {

        case TEST_PAGE_JOYSTICKS:
            return stick_page;

        case TEST_PAGE_JOYSTICK2:
            return second_stick_page;

        case TEST_PAGE_RUMBLE:
            return rumble_page;

        default:
            return page < TEST_PAGE_COUNT;
    }
}


// One dot per page the controller has, so GameCube shows five.
static void ui_draw_page_indicator(test_page_t page)
{
    char indicator[TEST_PAGE_COUNT + 1];

    uint8_t count =
        0;


    for (int p = 0; p < TEST_PAGE_COUNT; p++) {

        if (!ui_page_available((test_page_t)p)) {
            continue;
        }

        indicator[count++] =
            p == page ? '*' : '.';
    }

    indicator[count] =
        '\0';


    // Right-aligned. display_text() stops before the last 6 pixels.
    display_text(
        DISPLAY_WIDTH - 1 - count * 6,
        0,
        indicator
    );
}


// ============================================================================
// DIAGNOSTIC TICKER
// ============================================================================

static void ui_draw_diagnostics_ticker(uint16_t offset)
{
    char message[256];

    diagnostics_build_ticker(
        message,
        sizeof(message)
    );


    size_t length =
        strlen(message);


    char visible[22];

    memset(
        visible,
        0,
        sizeof(visible)
    );


    if (length <= 21) {

        snprintf(
            visible,
            sizeof(visible),
            "%s",
            message
        );
    }

    else {

        size_t cycle_length =
            length + 3;


        for (size_t i = 0; i < 21; i++) {

            size_t index =
                (offset + i) % cycle_length;


            if (index < length) {

                visible[i] =
                    message[index];
            }

            else {

                visible[i] =
                    ' ';
            }
        }


        visible[21] =
            '\0';
    }


    display_text(
        1,
        48,
        visible
    );
}


// ============================================================================
// WAITING
// ============================================================================

void ui_draw_waiting_screen(void)
{
    ui_draw_waiting_screen_status(NULL, NULL);
}


void ui_draw_waiting_screen_status(
    const char* status_line1,
    const char* status_line2
)
{
    bool has_status =
        status_line1 != NULL ||
        status_line2 != NULL;


    display_clear();


    // Shift the title up to make room for the debug lines.

    display_text(
        22,
        has_status ? 4 : 20,
        "RETROPAD LINK"
    );


    display_text(
        28,
        has_status ? 20 : 44,
        "CHECK CABLE"
    );


    if (status_line1) {

        display_text(
            0,
            44,
            status_line1
        );
    }


    if (status_line2) {

        display_text(
            0,
            55,
            status_line2
        );
    }


    display_update();
}


// ============================================================================
// CONNECTED
// ============================================================================

void ui_draw_connected_screen(void)
{
    display_clear();


    display_bitmap(
        4,
        0,
        super_nintendo_logo,
        SUPER_NINTENDO_LOGO_W,
        SUPER_NINTENDO_LOGO_H
    );


    display_text(
        34,
        36,
        "CONNECTED"
    );


    display_text(
        13,
        48,
        "HOLD START+SELECT"
    );


    display_text(
        7,
        56,
        "3 SEC FOR TEST MODE"
    );


    display_update();
}


void ui_draw_nes_connected_screen(void)
{
    display_clear();


    display_bitmap(
        (128 - NES_LOGO_W) / 2,
        0,
        nes_logo,
        NES_LOGO_W,
        NES_LOGO_H
    );


    display_text(
        34,
        36,
        "CONNECTED"
    );


    display_text(
        13,
        48,
        "HOLD START+SELECT"
    );


    display_text(
        7,
        56,
        "3 SEC FOR TEST MODE"
    );


    display_update();
}


void ui_draw_n64_connected_screen(void)
{
    display_clear();

    display_bitmap(
        10,
        0,
        n64_logo,
        N64_LOGO_W,
        N64_LOGO_H
    );

    display_text(
        34,
        36,
        "CONNECTED"
    );

    display_text(
        13,
        48,
        "HOLD START+Z"
    );

    display_text(
        7,
        56,
        "3 SEC FOR TEST MODE"
    );

    display_update();
}


// ============================================================================
// BUTTON HELPER
// ============================================================================
//
// Pressed buttons are shown as [X], released as  X .
//

static void ui_draw_button(
    uint8_t x,
    uint8_t y,
    uint32_t buttons,
    uint32_t mask,
    const char* pressed,
    const char* released
)
{
    display_text(
        x,
        y,
        (buttons & mask) ? pressed : released
    );
}


// ============================================================================
// SNES LIVE BUTTON TEST
// ============================================================================
//
// Laid out like the controller:
//
//   [L]                  [R]
//       U              X
//     L   R          Y   A
//       D   SEL ST     B
//

void ui_draw_live_screen(uint32_t buttons)
{
    display_clear();


    display_text(
        0,
        0,
        "SNES CONTROLLER"
    );


    ui_draw_page_indicator(
        TEST_PAGE_LIVE
    );


    // Shoulders

    ui_draw_button(0, 11, buttons, JP_BUTTON_L1, "[L]", " L ");
    ui_draw_button(104, 11, buttons, JP_BUTTON_R1, "[R]", " R ");


    // D-pad

    ui_draw_button(12, 22, buttons, JP_BUTTON_DU, "[U]", " U ");
    ui_draw_button(0, 31, buttons, JP_BUTTON_DL, "[L]", " L ");
    ui_draw_button(24, 31, buttons, JP_BUTTON_DR, "[R]", " R ");
    ui_draw_button(12, 40, buttons, JP_BUTTON_DD, "[D]", " D ");


    // Select / Start

    ui_draw_button(34, 44, buttons, JP_BUTTON_S1, "[SEL]", " SEL ");
    ui_draw_button(66, 44, buttons, JP_BUTTON_S2, "[ST]", " ST ");


    // Face buttons (JoypadOS: B=B1, A=B2, Y=B3, X=B4)

    ui_draw_button(92, 22, buttons, JP_BUTTON_B4, "[X]", " X ");
    ui_draw_button(78, 31, buttons, JP_BUTTON_B3, "[Y]", " Y ");
    ui_draw_button(104, 31, buttons, JP_BUTTON_B2, "[A]", " A ");
    ui_draw_button(92, 40, buttons, JP_BUTTON_B1, "[B]", " B ");


    display_update();
}


// ============================================================================
// NES LIVE BUTTON TEST
// ============================================================================
//
//       U
//     L   R              B   A
//       D   SEL ST
//

void ui_draw_nes_live_screen(uint32_t buttons)
{
    display_clear();


    display_text(
        0,
        0,
        "NES CONTROLLER"
    );


    ui_draw_page_indicator(
        TEST_PAGE_LIVE
    );


    // D-pad

    ui_draw_button(12, 22, buttons, JP_BUTTON_DU, "[U]", " U ");
    ui_draw_button(0, 31, buttons, JP_BUTTON_DL, "[L]", " L ");
    ui_draw_button(24, 31, buttons, JP_BUTTON_DR, "[R]", " R ");
    ui_draw_button(12, 40, buttons, JP_BUTTON_DD, "[D]", " D ");


    // Select / Start

    ui_draw_button(34, 44, buttons, JP_BUTTON_S1, "[SEL]", " SEL ");
    ui_draw_button(66, 44, buttons, JP_BUTTON_S2, "[ST]", " ST ");


    // Face buttons (JoypadOS: B=B1, A=B2)

    ui_draw_button(80, 31, buttons, JP_BUTTON_B1, "[B]", " B ");
    ui_draw_button(104, 31, buttons, JP_BUTTON_B2, "[A]", " A ");


    display_update();
}


// ============================================================================
// JOYSTICK TEST
// ============================================================================
//
// Left:  56x56 plot. Dotted circle = healthy reach, outline = furthest
//        point reached in each direction, dot = live position.
//
// Right: live X/Y, cardinal reach, rest offset, verdict.
//

#define STICK_PLOT_X        0
#define STICK_PLOT_Y        8
#define STICK_PLOT_SIZE     56

#define STICK_PLOT_CX       (STICK_PLOT_X + STICK_PLOT_SIZE / 2)
#define STICK_PLOT_CY       (STICK_PLOT_Y + STICK_PLOT_SIZE / 2)
#define STICK_PLOT_RADIUS   (STICK_PLOT_SIZE / 2 - 1)

#define STICK_TEXT_X        60


static int16_t stick_plot_clamp(
    int16_t value,
    int16_t low,
    int16_t high
)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}


// Convert native stick units (+Y up) to a pixel inside the plot.
static void stick_plot_point(
    float x,
    float y,
    int16_t full_scale,
    int16_t* px,
    int16_t* py
)
{
    *px = STICK_PLOT_CX +
        (int16_t)lroundf(x * STICK_PLOT_RADIUS / full_scale);

    *py = STICK_PLOT_CY -
        (int16_t)lroundf(y * STICK_PLOT_RADIUS / full_scale);


    *px = stick_plot_clamp(
        *px,
        STICK_PLOT_X + 1,
        STICK_PLOT_X + STICK_PLOT_SIZE - 2
    );

    *py = stick_plot_clamp(
        *py,
        STICK_PLOT_Y + 1,
        STICK_PLOT_Y + STICK_PLOT_SIZE - 2
    );
}


static void stick_plot_line(
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1
)
{
    int16_t dx = abs(x1 - x0);
    int16_t dy = -abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;


    while (true) {

        display_pixel(x0, y0, true);


        if (x0 == x1 && y0 == y1) {
            break;
        }


        int16_t e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}


static void stick_plot_draw(
    const joystick_diag_t* diag,
    const joystick_profile_t* profile
)
{
    int16_t px;
    int16_t py;


    // Frame and center ticks.

    display_rect(
        STICK_PLOT_X,
        STICK_PLOT_Y,
        STICK_PLOT_SIZE,
        STICK_PLOT_SIZE
    );

    display_vline(STICK_PLOT_CX, STICK_PLOT_Y, 3);
    display_vline(STICK_PLOT_CX, STICK_PLOT_Y + STICK_PLOT_SIZE - 3, 3);
    display_hline(STICK_PLOT_X, STICK_PLOT_CY, 3);
    display_hline(STICK_PLOT_X + STICK_PLOT_SIZE - 3, STICK_PLOT_CY, 3);


    // Dotted guide at the reach a healthy stick should hit.

    for (int i = 0; i < 48; i++) {

        float angle =
            (float)i * 2.0f * (float)M_PI / 48.0f;

        stick_plot_point(
            profile->reference_reach * cosf(angle),
            profile->reference_reach * sinf(angle),
            profile->full_scale,
            &px,
            &py
        );

        display_pixel(px, py, true);
    }


    // Recorded outline. Only joins neighbouring slices that have both
    // been visited, so an untested direction shows as a gap.

    for (int i = 0; i < JOYSTICK_DIAG_TRACE_BINS; i++) {

        int next =
            (i + 1) % JOYSTICK_DIAG_TRACE_BINS;


        if (diag->trace[i] == 0 || diag->trace[next] == 0) {
            continue;
        }


        float a0 =
            (float)i * 2.0f * (float)M_PI / JOYSTICK_DIAG_TRACE_BINS;

        float a1 =
            (float)next * 2.0f * (float)M_PI / JOYSTICK_DIAG_TRACE_BINS;


        int16_t x0, y0, x1, y1;

        stick_plot_point(
            diag->trace[i] * cosf(a0),
            diag->trace[i] * sinf(a0),
            profile->full_scale,
            &x0,
            &y0
        );

        stick_plot_point(
            diag->trace[next] * cosf(a1),
            diag->trace[next] * sinf(a1),
            profile->full_scale,
            &x1,
            &y1
        );

        stick_plot_line(x0, y0, x1, y1);
    }


    // Live position.

    stick_plot_point(
        diag->x,
        diag->y,
        profile->full_scale,
        &px,
        &py
    );

    px = stick_plot_clamp(
        px,
        STICK_PLOT_X + 3,
        STICK_PLOT_X + STICK_PLOT_SIZE - 4
    );

    py = stick_plot_clamp(
        py,
        STICK_PLOT_Y + 3,
        STICK_PLOT_Y + STICK_PLOT_SIZE - 4
    );

    display_fill_circle(px, py, 2, true);
}


static const char* stick_verdict_text(joystick_verdict_t verdict)
{
    switch (verdict) {

        case JOYSTICK_VERDICT_GOOD:
            return "GOOD";

        case JOYSTICK_VERDICT_FAIR:
            return "FAIR";

        case JOYSTICK_VERDICT_WORN:
            return "WORN";

        case JOYSTICK_VERDICT_OFF_CENTER:
            return "OFF CENTER";

        case JOYSTICK_VERDICT_INCOMPLETE:
        default:
            return "SPIN STICK";
    }
}


void ui_draw_joystick_test_screen(uint8_t stick)
{
    const joystick_diag_t* diag =
        joystick_diag_get(stick);

    const joystick_profile_t* profile =
        joystick_diag_get_profile(stick);


    char line[24];


    display_clear();


    snprintf(
        line,
        sizeof(line),
        "%s STICK",
        profile->name
    );

    display_text(
        0,
        0,
        line
    );


    ui_draw_page_indicator(
        stick == 0
            ? TEST_PAGE_JOYSTICKS
            : TEST_PAGE_JOYSTICK2
    );


    stick_plot_draw(
        diag,
        profile
    );


    snprintf(
        line,
        sizeof(line),
        "X%+d Y%+d",
        diag->x,
        diag->y
    );

    display_text(
        STICK_TEXT_X,
        11,
        line
    );


    snprintf(
        line,
        sizeof(line),
        "U%d D%d",
        diag->reach_up,
        diag->reach_down
    );

    display_text(
        STICK_TEXT_X,
        21,
        line
    );


    snprintf(
        line,
        sizeof(line),
        "L%d R%d",
        diag->reach_left,
        diag->reach_right
    );

    display_text(
        STICK_TEXT_X,
        31,
        line
    );


    if (diag->rest_valid) {

        snprintf(
            line,
            sizeof(line),
            "C%+d,%+d",
            diag->rest_x,
            diag->rest_y
        );
    }

    else {

        snprintf(
            line,
            sizeof(line),
            "C --"
        );
    }

    display_text(
        STICK_TEXT_X,
        41,
        line
    );


    display_hline(
        STICK_TEXT_X,
        52,
        DISPLAY_WIDTH - STICK_TEXT_X
    );


    display_text(
        STICK_TEXT_X,
        55,
        stick_verdict_text(
            joystick_diag_get_verdict(stick)
        )
    );


    display_update();
}


void ui_draw_joystick_reset_screen(void)
{
    display_clear();


    display_text(
        34,
        18,
        "JOYSTICK"
    );


    display_text(
        34,
        34,
        "CLEARED"
    );


    display_update();
}


// ============================================================================
// RUMBLE TEST
// ============================================================================
//
// RUMBLE                ...*..
//
//          +---------+
//      ((  |   ON    |  ))
//          +---------+
//
//        HOLD A TO TEST
//

void ui_draw_rumble_screen(bool motor_on)
{
    display_clear();


    display_text(
        0,
        0,
        "RUMBLE"
    );


    ui_draw_page_indicator(
        TEST_PAGE_RUMBLE
    );


    display_rect(
        34,
        13,
        60,
        22
    );


    if (motor_on) {

        display_text_large(52, 16, "ON");

        display_text(18, 20, "((");
        display_text(98, 20, "))");
    }

    else {

        display_text_large(46, 16, "OFF");
    }


    display_text(
        22,
        48,
        "HOLD A TO TEST"
    );


    display_update();
}


// ============================================================================
// DIAGNOSTICS
// ============================================================================

void ui_draw_diagnostics_screen(uint16_t ticker_offset)
{
    uint8_t pass_count = 0;
    uint8_t chatter_count = 0;
    uint8_t dropout_count = 0;


    diagnostics_get_counts(
        &pass_count,
        &chatter_count,
        &dropout_count
    );


    char line[24];


    display_clear();


    display_text(
        0,
        0,
        "DIAGNOSTICS"
    );


    ui_draw_page_indicator(
        TEST_PAGE_DIAGNOSTICS
    );


    snprintf(
        line,
        sizeof(line),
        "PASS: %u/%u",
        pass_count,
        (unsigned int)diagnostics_get_button_count()
    );


    display_text(
        0,
        12,
        line
    );


    snprintf(
        line,
        sizeof(line),
        "CHATTER: %u",
        chatter_count
    );


    display_text(
        0,
        24,
        line
    );


    snprintf(
        line,
        sizeof(line),
        "DROPOUT: %u",
        dropout_count
    );


    display_text(
        0,
        36,
        line
    );


    ui_draw_diagnostics_ticker(
        ticker_offset
    );


    display_text(
        1,
        56,
        diagnostics_get_reset_hint()
    );


    display_update();
}


// ============================================================================
// RAW
// ============================================================================

void ui_draw_raw_screen(uint32_t buttons)
{
    char value[24];


    display_clear();


    display_text(
        0,
        0,
        "RAW INPUT"
    );


    ui_draw_page_indicator(
        TEST_PAGE_RAW
    );


    display_text(
        0,
        18,
        "BUTTON MASK:"
    );


    snprintf(
        value,
        sizeof(value),
        "0x%08lX",
        (unsigned long)buttons
    );


    display_text(
        24,
        30,
        value
    );


    int8_t type =
        retropad_snes_get_device_type();


    snprintf(
        value,
        sizeof(value),
        "TYPE: %d",
        type
    );


    display_text(
        36,
        46,
        value
    );


    display_update();
}


// ============================================================================
// DIAGNOSTICS RESET CONFIRMATION
// ============================================================================

void ui_draw_diagnostics_reset_screen(void)
{
    display_clear();


    display_text(
        28,
        18,
        "DIAGNOSTICS"
    );


    display_text(
        34,
        34,
        "CLEARED"
    );


    display_update();
}


// ============================================================================
// DRAW CURRENT PAGE
// ============================================================================

void ui_draw_current_test_page(
    test_page_t page,
    uint32_t buttons,
    uint16_t ticker_offset
)
{
    switch (page) {

        case TEST_PAGE_LIVE:

            ui_draw_live_screen(
                buttons
            );

            break;


        case TEST_PAGE_DIAGNOSTICS:

            ui_draw_diagnostics_screen(
                ticker_offset
            );

            break;


        case TEST_PAGE_RAW:

            ui_draw_raw_screen(
                buttons
            );

            break;


        default:

            ui_draw_live_screen(
                buttons
            );

            break;
    }
}


// ============================================================================
// N64 LIVE BUTTON TEST
// ============================================================================
//
// Laid out like the controller:
//
//   [L]                  [R]
//       U    ST      C^
//     L   R        C<  C>
//       D     (o)    Cv
//                [B]
//             Z     [A]
//
// Pressed buttons are shown as [X], released as  X , like the SNES page.
// The small box is the analog stick (full test on the Joysticks page).
//

#define N64_STICK_BOX_X     48
#define N64_STICK_BOX_Y     33
#define N64_STICK_BOX_SIZE  15


void ui_draw_n64_live_screen(
    uint32_t buttons,
    uint8_t stick_x,
    uint8_t stick_y
)
{
    display_clear();


    display_text(
        0,
        0,
        "N64 CONTROLLER"
    );


    ui_draw_page_indicator(
        TEST_PAGE_LIVE
    );


    // Shoulders

    ui_draw_button(0, 11, buttons, JP_BUTTON_L2, "[L]", " L ");
    ui_draw_button(104, 11, buttons, JP_BUTTON_R2, "[R]", " R ");


    // D-pad

    ui_draw_button(12, 22, buttons, JP_BUTTON_DU, "[U]", " U ");
    ui_draw_button(0, 31, buttons, JP_BUTTON_DL, "[L]", " L ");
    ui_draw_button(24, 31, buttons, JP_BUTTON_DR, "[R]", " R ");
    ui_draw_button(12, 40, buttons, JP_BUTTON_DD, "[D]", " D ");


    // Start

    ui_draw_button(46, 20, buttons, JP_BUTTON_S2, "[ST]", " ST ");


    // C buttons (font chars 1-4 are up/down/left/right arrows)

    ui_draw_button(92, 20, buttons, JP_BUTTON_L3, "[\x01]", " \x01 ");
    ui_draw_button(80, 28, buttons, JP_BUTTON_B4, "[\x03]", " \x03 ");
    ui_draw_button(104, 28, buttons, JP_BUTTON_R3, "[\x04]", " \x04 ");
    ui_draw_button(92, 36, buttons, JP_BUTTON_B2, "[\x02]", " \x02 ");


    // B / A

    ui_draw_button(66, 43, buttons, JP_BUTTON_B3, "[B]", " B ");
    ui_draw_button(84, 52, buttons, JP_BUTTON_B1, "[A]", " A ");


    // Z sits behind the stick

    ui_draw_button(46, 52, buttons, JP_BUTTON_R1, "[Z]", " Z ");


    // Analog stick

    display_rect(
        N64_STICK_BOX_X,
        N64_STICK_BOX_Y,
        N64_STICK_BOX_SIZE,
        N64_STICK_BOX_SIZE
    );


    int16_t travel =
        N64_STICK_BOX_SIZE / 2 - 2;

    int16_t dot_x =
        N64_STICK_BOX_X + N64_STICK_BOX_SIZE / 2 +
        ((int16_t)stick_x - 128) * travel / 128;

    int16_t dot_y =
        N64_STICK_BOX_Y + N64_STICK_BOX_SIZE / 2 +
        ((int16_t)stick_y - 128) * travel / 128;

    display_fill_circle(
        (uint8_t)dot_x,
        (uint8_t)dot_y,
        1,
        true
    );


    display_update();
}


// ============================================================================
// GAMECUBE CONNECTED
// ============================================================================

void ui_draw_gc_connected_screen(void)
{
    display_clear();

    display_bitmap(
        2,
        2,
        gc_cube_logo,
        GC_CUBE_LOGO_W,
        GC_CUBE_LOGO_H
    );

    display_text_large(
        28,
        7,
        "GAMECUBE"
    );

    display_text(
        34,
        36,
        "CONNECTED"
    );

    display_text(
        13,
        48,
        "HOLD START+Z"
    );

    display_text(
        7,
        56,
        "3 SEC FOR TEST MODE"
    );

    display_update();
}


// ============================================================================
// GAMECUBE LIVE BUTTON TEST
// ============================================================================
//
// Laid out like the controller:
//
//   [L]=====          =====[R]      bars = analog trigger pull
//                           Z
//    (o)              Y
//       U    ST    A    X
//     L   R      B
//       D               (c)         (o) main stick, (c) C-stick
//

static void ui_draw_stick_box(
    uint8_t x,
    uint8_t y,
    uint8_t size,
    uint8_t stick_x,
    uint8_t stick_y
)
{
    display_rect(x, y, size, size);


    int16_t travel =
        size / 2 - 2;

    int16_t dot_x =
        x + size / 2 + ((int16_t)stick_x - 128) * travel / 128;

    int16_t dot_y =
        y + size / 2 + ((int16_t)stick_y - 128) * travel / 128;


    display_fill_circle(
        (uint8_t)dot_x,
        (uint8_t)dot_y,
        1,
        true
    );
}


static void ui_draw_trigger_bar(
    uint8_t x,
    uint8_t y,
    uint8_t value
)
{
    const uint8_t width = 24;
    const uint8_t height = 5;


    display_rect(x, y, width, height);


    uint8_t fill =
        (uint8_t)((uint16_t)value * (width - 2) / 255);


    if (fill > 0) {

        display_fill_rect(
            x + 1,
            y + 1,
            fill,
            height - 2,
            true
        );
    }
}


void ui_draw_gc_live_screen(
    uint32_t buttons,
    uint8_t stick_x,
    uint8_t stick_y,
    uint8_t cstick_x,
    uint8_t cstick_y,
    uint8_t l_analog,
    uint8_t r_analog
)
{
    display_clear();


    display_text(
        0,
        0,
        "GAMECUBE"
    );


    ui_draw_page_indicator(
        TEST_PAGE_LIVE
    );


    // Shoulders: digital click + analog pull

    ui_draw_button(0, 11, buttons, JP_BUTTON_L2, "[L]", " L ");
    ui_draw_trigger_bar(20, 12, l_analog);

    ui_draw_trigger_bar(78, 12, r_analog);
    ui_draw_button(104, 11, buttons, JP_BUTTON_R2, "[R]", " R ");

    ui_draw_button(104, 20, buttons, JP_BUTTON_R1, "[Z]", " Z ");


    // Main stick and D-pad

    ui_draw_stick_box(14, 21, 15, stick_x, stick_y);

    ui_draw_button(12, 38, buttons, JP_BUTTON_DU, "[U]", " U ");
    ui_draw_button(0, 47, buttons, JP_BUTTON_DL, "[L]", " L ");
    ui_draw_button(24, 47, buttons, JP_BUTTON_DR, "[R]", " R ");
    ui_draw_button(12, 56, buttons, JP_BUTTON_DD, "[D]", " D ");


    // Start

    ui_draw_button(46, 31, buttons, JP_BUTTON_S2, "[ST]", " ST ");


    // Face buttons (JoypadOS: A=B2, B=B1, X=B4, Y=B3)

    ui_draw_button(84, 37, buttons, JP_BUTTON_B2, "[A]", " A ");
    ui_draw_button(66, 47, buttons, JP_BUTTON_B1, "[B]", " B ");
    ui_draw_button(102, 33, buttons, JP_BUTTON_B4, "[X]", " X ");
    ui_draw_button(84, 26, buttons, JP_BUTTON_B3, "[Y]", " Y ");


    // C-stick

    ui_draw_stick_box(102, 48, 13, cstick_x, cstick_y);


    display_update();
}
