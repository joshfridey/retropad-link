// nes.c - RetroPad Link NES controller reader
//
// The NES pad is a 4021 shift register, read the same way as SNES: pulse
// Latch, then clock the bits out on Data. Clock / Latch are shared with the
// SNES socket; only Data has its own pin.
//
// Detection:
//   The 4021's serial input is tied to GND inside the pad, so after its 8
//   button bits a connected pad shifts out 0s. An empty socket reads all 1s
//   through the Data pull-up.

#include "controllers/nes.h"

#include "config.h"

#include "core/router/router.h"
#include "core/input_event.h"
#include "core/buttons.h"

#include "pico/stdlib.h"

#include <stdio.h>


// Same timing as the SNESpad driver.
#define NES_HALF_BIT_US  12

// Bits after the 8 buttons that must read 0 for a pad to count as present.
#define NES_READ_BITS    16


static bool initialized =
    false;

static bool connected =
    false;

static uint32_t buttons =
    0;

static uint32_t prev_buttons =
    0xFFFFFFFF;


// ============================================================================
// INITIALIZATION
// ============================================================================

void retropad_nes_init(void)
{
    gpio_init(RETROPAD_NES_DATA_GPIO);
    gpio_set_dir(RETROPAD_NES_DATA_GPIO, GPIO_IN);
    gpio_pull_up(RETROPAD_NES_DATA_GPIO);

    initialized =
        true;

    printf(
        "[nes] CLK=%d LATCH=%d DATA=%d\n",
        RETROPAD_SNES_CLOCK_GPIO,
        RETROPAD_SNES_LATCH_GPIO,
        RETROPAD_NES_DATA_GPIO
    );
}


// ============================================================================
// READ
// ============================================================================

// Returns the raw shift register bits, bit 0 first. Active low.
static uint32_t nes_read_raw(void)
{
    uint32_t raw =
        0;


    gpio_put(RETROPAD_SNES_LATCH_GPIO, 1);
    sleep_us(NES_HALF_BIT_US);

    gpio_put(RETROPAD_SNES_LATCH_GPIO, 0);
    sleep_us(NES_HALF_BIT_US);


    for (uint8_t i = 0; i < NES_READ_BITS; i++) {

        gpio_put(RETROPAD_SNES_CLOCK_GPIO, 0);
        sleep_us(NES_HALF_BIT_US);

        raw |= (uint32_t)gpio_get(RETROPAD_NES_DATA_GPIO) << i;

        gpio_put(RETROPAD_SNES_CLOCK_GPIO, 1);
        sleep_us(NES_HALF_BIT_US);
    }

    return raw;
}


// NES bit order: A, B, Select, Start, Up, Down, Left, Right.
// Same Switch-style mapping as JoypadOS's map_nes_to_usbr().
static uint32_t nes_map_buttons(uint32_t pressed)
{
    uint32_t out =
        0;

    if (pressed & (1u << 0)) out |= JP_BUTTON_B2;  // A
    if (pressed & (1u << 1)) out |= JP_BUTTON_B1;  // B
    if (pressed & (1u << 2)) out |= JP_BUTTON_S1;  // Select
    if (pressed & (1u << 3)) out |= JP_BUTTON_S2;  // Start
    if (pressed & (1u << 4)) out |= JP_BUTTON_DU;
    if (pressed & (1u << 5)) out |= JP_BUTTON_DD;
    if (pressed & (1u << 6)) out |= JP_BUTTON_DL;
    if (pressed & (1u << 7)) out |= JP_BUTTON_DR;

    return out;
}


// ============================================================================
// TASK
// ============================================================================

void retropad_nes_task(void)
{
    if (!initialized) {
        return;
    }


    uint32_t raw =
        nes_read_raw();


    // Bits 8-15 are the pad's grounded serial input: all 0 when present.
    connected =
        ((raw >> 8) & 0xFF) == 0;


    if (!connected) {

        buttons =
            0;

        prev_buttons =
            0xFFFFFFFF;

        return;
    }


    buttons =
        nes_map_buttons(~raw & 0xFF);


    if (buttons == prev_buttons) {
        return;
    }

    prev_buttons =
        buttons;


    input_event_t event;
    init_input_event(&event);

    event.dev_addr = RETROPAD_NES_DEV_ADDR;
    event.instance = 0;
    event.type = INPUT_TYPE_GAMEPAD;
    event.buttons = buttons;

    router_submit_input(&event);
}


// ============================================================================
// STATE
// ============================================================================

bool retropad_nes_is_connected(void)
{
    return connected;
}


uint32_t retropad_nes_get_buttons(void)
{
    return buttons;
}
