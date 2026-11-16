#ifndef RETROPAD_CONTROLLER_NES_H
#define RETROPAD_CONTROLLER_NES_H

#include <stdbool.h>
#include <stdint.h>


// ============================================================================
// NES CONTROLLER INTERFACE
// ============================================================================
//
// NES pad on its own Data pin, sharing SNES Clock / Latch.

// Router device address for NES input events.
#define RETROPAD_NES_DEV_ADDR  0xC0


// Configure the NES Data pin. Must run after retropad_snes_init(), which
// owns the shared Clock / Latch pins.
void retropad_nes_init(void);


// Read the pad and submit an input event when its state changes.
void retropad_nes_task(void);


// Return true when an NES pad is plugged into the NES socket.
bool retropad_nes_is_connected(void);


// Return the current controller state in JoypadOS button format.
uint32_t retropad_nes_get_buttons(void);


#endif
