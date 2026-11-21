// n64_host_retropad.h - RetroPad Link additions to the N64 host driver
//
// Declares functions that exist only in RetroPad Link's copy of n64_host.c,
// so JoypadOS's n64_host.h stays unmodified.

#ifndef N64_HOST_RETROPAD_H
#define N64_HOST_RETROPAD_H


// PIO0 instruction-memory offset where the N64 host loaded the joybus
// program, or -1 if the N64 host has not been initialized. The GameCube
// driver runs the same program, so it reuses this copy instead of loading
// its own.
int retropad_n64_joybus_offset(void);


#endif
