// gamecube.c - RetroPad Link GameCube controller adapter
//
// Keeps JoypadOS GameCube-host-specific calls out of the main application.

#include "controllers/gamecube.h"

#include "config.h"

#include "native/host/gc/gc_host.h"


// Provided by the RetroPad-local GameCube driver (controllers/gc).
bool retropad_gc_get_origin_sticks(uint8_t out[4]);
void retropad_gc_get_raw_sticks(uint8_t out[4]);


// ============================================================================
// INITIALIZATION
// ============================================================================

void retropad_gamecube_init(void)
{
    gc_host_init_pin(
        RETROPAD_GAMECUBE_DATA_GPIO
    );
}


// ============================================================================
// TASK
// ============================================================================

void retropad_gamecube_task(void)
{
    gc_host_task();
}


// ============================================================================
// CONNECTION STATE
// ============================================================================

bool retropad_gamecube_is_connected(void)
{
    return gc_host_is_connected();
}


// ============================================================================
// STICKS
// ============================================================================

void retropad_gamecube_get_sticks(
    int16_t* stick_x,
    int16_t* stick_y,
    int16_t* cstick_x,
    int16_t* cstick_y
)
{
    uint8_t origin[4];
    uint8_t raw[4];

    retropad_gc_get_origin_sticks(origin);
    retropad_gc_get_raw_sticks(raw);

    // GameCube Y already increases upward.
    *stick_x  = (int16_t)raw[0] - origin[0];
    *stick_y  = (int16_t)raw[1] - origin[1];
    *cstick_x = (int16_t)raw[2] - origin[2];
    *cstick_y = (int16_t)raw[3] - origin[3];
}


// ============================================================================
// RUMBLE
// ============================================================================

void retropad_gamecube_set_rumble(bool enabled)
{
    gc_host_set_rumble(
        0,
        enabled
    );
}
