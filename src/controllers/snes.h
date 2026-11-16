#ifndef RETROPAD_CONTROLLER_SNES_H
#define RETROPAD_CONTROLLER_SNES_H

#include <stdbool.h>
#include <stdint.h>


// ============================================================================
// SNES CONTROLLER INTERFACE
// ============================================================================

// Router device address for SNES input events (set by the JoypadOS SNES host).
#define RETROPAD_SNES_DEV_ADDR  0xF0


// Initialize the SNES controller host using RetroPad Link's configured pins.
void retropad_snes_init(void);


// Poll the controller and submit an input event when its state changes.
void retropad_snes_task(void);


// Return true when a controller is connected.
bool retropad_snes_is_connected(void);


// Return the current controller state in JoypadOS button format.
uint32_t retropad_snes_get_buttons(void);


// Return the detected SNES/NES device type reported by JoypadOS.
int8_t retropad_snes_get_device_type(void);


// Send rumble feedback to the connected controller/accessory.
void retropad_snes_set_rumble(
    uint8_t left,
    uint8_t right
);


#endif
