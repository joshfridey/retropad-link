#ifndef RETROPAD_CONTROLLER_PLAYSTATION_H
#define RETROPAD_CONTROLLER_PLAYSTATION_H

#include <stdbool.h>


// ============================================================================
// PLAYSTATION CONTROLLER INTERFACE (PS1 / PS2)
// ============================================================================

// Router device address for PlayStation input events (set by the JoypadOS
// PSX host).
#define RETROPAD_PLAYSTATION_DEV_ADDR  0xE1


// Initialize the PlayStation host using RetroPad Link's configured GPIO.
void retropad_playstation_init(void);


// Poll the controller and submit an input event when its state changes.
void retropad_playstation_task(void);


// Return true when a PlayStation controller is connected.
bool retropad_playstation_is_connected(void);


// Run both DualShock rumble motors at full strength, or stop them.
//
// The JoypadOS PSX host reads its motor levels from player 0's feedback
// state on every poll, so this sets that state rather than the motors.
void retropad_playstation_set_rumble(bool enabled);


#endif
