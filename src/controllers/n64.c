// n64.c - RetroPad Link N64 controller adapter
//
// Keeps JoypadOS N64-host-specific calls out of the main application.
//
// Buttons and stick reach the app through the routed USB event
// (controller_state). Rumble Pak feedback is handled inside the JoypadOS
// N64 host.

#include "controllers/n64.h"

#include "config.h"

#include "native/host/n64/n64_host.h"


// ============================================================================
// INITIALIZATION
// ============================================================================

void retropad_n64_init(void)
{
    n64_host_init_pin(
        RETROPAD_N64_DATA_GPIO
    );
}


// ============================================================================
// TASK
// ============================================================================

void retropad_n64_task(void)
{
    n64_host_task();
}


// ============================================================================
// CONNECTION STATE
// ============================================================================

bool retropad_n64_is_connected(void)
{
    return n64_host_is_connected();
}
