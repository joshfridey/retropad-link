// gba_payload_stub.c - Disables JoypadOS's GBA-as-controller multiboot.
//
// JoypadOS's gc_host.c tries to multiboot a GBA payload (~216 KB) every 2 s
// whenever no GameCube controller answers. RetroPad Link does not use the
// GBA link feature, so link an empty payload instead of
// native/host/gc/gba_payload.c. gc_host skips the multiboot path entirely
// when gba_payload_len is 0.

#include <stdint.h>

const uint8_t gba_payload[1] = { 0 };
const uint32_t gba_payload_len = 0;
