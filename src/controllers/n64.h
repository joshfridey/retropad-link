#ifndef RETROPAD_CONTROLLER_N64_H
#define RETROPAD_CONTROLLER_N64_H

#include <stdbool.h>


// ============================================================================
// N64 CONTROLLER INTERFACE
// ============================================================================

// Router device address for N64 input events (set by the JoypadOS N64 host).
#define RETROPAD_N64_DEV_ADDR  0xE0


// Initialize the N64 Joybus host using RetroPad Link's configured GPIO.
void retropad_n64_init(void);


// Poll the controller and submit an input event.
void retropad_n64_task(void);


// Return true when an N64 controller is connected.
bool retropad_n64_is_connected(void);


#endif
