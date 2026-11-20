// playstation.c - RetroPad Link PlayStation (PS1 / PS2) controller adapter
//
// Keeps JoypadOS PSX-host-specific calls out of the main application.
//
// Buttons and sticks reach the app through the routed USB event
// (controller_state). The host switches DualShock pads into analog mode and
// reads rumble from the feedback state on its own.

#include "controllers/playstation.h"

#include "config.h"

#include "native/host/psx/psx_host.h"
#include "core/services/players/feedback.h"


// ============================================================================
// INITIALIZATION
// ============================================================================

void retropad_playstation_init(void)
{
    psx_host_init_pins(
        RETROPAD_PSX_CMD_GPIO,
        RETROPAD_PSX_CLK_GPIO,
        RETROPAD_PSX_ATT_GPIO,
        RETROPAD_PSX_DAT_GPIO
    );
}


// ============================================================================
// TASK
// ============================================================================

void retropad_playstation_task(void)
{
    psx_host_task();
}


// ============================================================================
// CONNECTION STATE
// ============================================================================

bool retropad_playstation_is_connected(void)
{
    return psx_host_is_connected();
}


// ============================================================================
// RUMBLE
// ============================================================================

void retropad_playstation_set_rumble(bool enabled)
{
    // Left = large (PWM) motor, right = small (on/off) motor.
    feedback_set_rumble(
        0,
        enabled ? 255 : 0,
        enabled ? 255 : 0
    );
}
