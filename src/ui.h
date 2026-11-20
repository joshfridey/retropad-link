#ifndef RETROPAD_UI_H
#define RETROPAD_UI_H

#include <stdbool.h>
#include <stdint.h>

// Page order:
//
// LIVE -> [JOYSTICKS] -> [JOYSTICK 2] -> [RUMBLE] -> DIAGNOSTICS -> RAW
//
// Bracketed pages only exist for controllers that have them (N64: stick;
// GameCube: stick, C-stick and rumble motor); see ui_set_optional_pages().
//
typedef enum {
    TEST_PAGE_LIVE = 0,
    TEST_PAGE_JOYSTICKS,
    TEST_PAGE_JOYSTICK2,
    TEST_PAGE_RUMBLE,
    TEST_PAGE_DIAGNOSTICS,
    TEST_PAGE_RAW,
    TEST_PAGE_COUNT
} test_page_t;

// Include the optional pages the connected controller has.
void ui_set_optional_pages(
    bool stick,
    bool second_stick,
    bool rumble
);

// False for pages the connected controller does not have.
bool ui_page_available(test_page_t page);

void ui_draw_waiting_screen(void);
void ui_draw_connected_screen(void);
void ui_draw_n64_connected_screen(void);
void ui_draw_nes_connected_screen(void);
void ui_draw_nes_live_screen(uint32_t buttons);

// Joystick test: live plot, recorded outline, reach, rest offset and
// verdict. Reads its data from joystick_diag.
void ui_draw_joystick_test_screen(uint8_t stick);
void ui_draw_joystick_reset_screen(void);

// stick_x / stick_y: JoypadOS analog values (0-255, 128 = center).
void ui_draw_n64_live_screen(
    uint32_t buttons,
    uint8_t stick_x,
    uint8_t stick_y
);
void ui_draw_gc_connected_screen(void);

// Analog values are JoypadOS 0-255 (sticks 128 = center, triggers 0 = released).
void ui_draw_gc_live_screen(
    uint32_t buttons,
    uint8_t stick_x,
    uint8_t stick_y,
    uint8_t cstick_x,
    uint8_t cstick_y,
    uint8_t l_analog,
    uint8_t r_analog
);

void ui_draw_live_screen(uint32_t buttons);
void ui_draw_diagnostics_screen(uint16_t ticker_offset);
void ui_draw_raw_screen(uint32_t buttons);
void ui_draw_diagnostics_reset_screen(void);

// Rumble test. motor_on: the motor is currently being driven.
// button_name: the button that runs it ("A", "CROSS").
void ui_draw_rumble_screen(
    bool motor_on,
    const char* button_name
);

void ui_draw_playstation_connected_screen(void);

// Sticks are the PSX host's raw 0-255 values (128 = centre, 0 = up / left).
void ui_draw_playstation_live_screen(
    uint32_t buttons,
    uint8_t left_x,
    uint8_t left_y,
    uint8_t right_x,
    uint8_t right_y
);

void ui_draw_current_test_page(
    test_page_t page,
    uint32_t buttons,
    uint16_t ticker_offset
);

#endif
