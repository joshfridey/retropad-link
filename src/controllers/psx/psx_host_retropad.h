// psx_host_retropad.h - RetroPad Link additions to the PS1 / PS2 host driver
//
// Declares functions that exist only in RetroPad Link's copy of psx_host.c,
// so JoypadOS's psx_host.h stays unmodified.

#ifndef PSX_HOST_RETROPAD_H
#define PSX_HOST_RETROPAD_H

#include <stdint.h>


// Controller ID byte of the connected pad's current mode, or 0xFF when none
// is connected. 0x41 = digital, 0x73 = analog (DualShock), 0x79 = DualShock 2
// analog with pressure buttons, 0x53 = analog sticks without rumble (early
// Dual Analog "flight" mode). Ignores the brief 0xF3 config-mode replies.
uint8_t psx_host_get_pad_id(void);


#endif
