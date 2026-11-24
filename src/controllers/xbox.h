#ifndef RETROPAD_CONTROLLER_XBOX_H
#define RETROPAD_CONTROLLER_XBOX_H

#include <stdbool.h>
#include <stdint.h>


// ============================================================================
// ORIGINAL XBOX CONTROLLER INTERFACE (USB HOST PORT)
// ============================================================================
//
// The controller is read by JoypadOS's USB host and XInput driver, which run
// as their own input interface; nothing here polls it. Buttons, sticks and
// triggers reach the app through the routed USB event (controller_state).


// Trigger pull (0-255) counted as pressed for the LT / RT buttons. Same
// threshold the XInput driver uses for the analog face buttons.
#define RETROPAD_XBOX_TRIGGER_THRESHOLD  0x20


// True while an original Xbox controller (Duke or Controller S) is mounted.
bool retropad_xbox_is_connected(void);


// Run both rumble motors at full strength, or stop them.
void retropad_xbox_set_rumble(bool enabled);


#endif
