#ifndef RETROPAD_CONTROLLER_GAMECUBE_H
#define RETROPAD_CONTROLLER_GAMECUBE_H

#include <stdbool.h>
#include <stdint.h>


// ============================================================================
// GAMECUBE CONTROLLER INTERFACE
// ============================================================================

// Router device address for GameCube input events (set by gc_debug/gc_host.c).
#define RETROPAD_GAMECUBE_DEV_ADDR  0xD0


// Initialize the GameCube Joybus host using RetroPad Link's configured GPIO.
void retropad_gamecube_init(void);


// Run the GameCube host polling task.
void retropad_gamecube_task(void);


// Return true when a GameCube controller is connected.
bool retropad_gamecube_is_connected(void);


// Read both sticks in raw GameCube units, relative to the position the
// controller reported at plug-in (+X right, +Y up). Unlike the routed USB
// values, these are not auto-scaled, so a worn stick reads short.
void retropad_gamecube_get_sticks(
    int16_t* stick_x,
    int16_t* stick_y,
    int16_t* cstick_x,
    int16_t* cstick_y
);


// Set GameCube rumble.
//
// JoypadOS exposes GameCube rumble as a simple on/off state.
void retropad_gamecube_set_rumble(bool enabled);


#endif
