// snes.c - RetroPad Link SNES controller adapter
//
// Keeps JoypadOS SNES-host-specific calls out of the main application.

#include "controllers/snes.h"

#include "app.h"

#include "native/host/snes/snes_host.h"


// ============================================================================
// INITIALIZATION
// ============================================================================

void retropad_snes_init(void)
{
    snes_host_init_pins(
        SNES_PIN_CLOCK,
        SNES_PIN_LATCH,
        SNES_PIN_DATA0,
        SNES_PIN_DATA1,
        SNES_PIN_IOBIT
    );
}


// ============================================================================
// TASK
// ============================================================================

void retropad_snes_task(void)
{
    snes_host_task();
}


// ============================================================================
// CONNECTION STATE
// ============================================================================

bool retropad_snes_is_connected(void)
{
    return snes_host_is_connected();
}


// ============================================================================
// BUTTON STATE
// ============================================================================

uint32_t retropad_snes_get_buttons(void)
{
    return snes_host_get_buttons(0);
}


// ============================================================================
// DEVICE TYPE
// ============================================================================

int8_t retropad_snes_get_device_type(void)
{
    return snes_host_get_device_type(0);
}


// ============================================================================
// RUMBLE
// ============================================================================

void retropad_snes_set_rumble(
    uint8_t left,
    uint8_t right
)
{
    snes_host_set_rumble(
        0,
        left,
        right
    );
}
