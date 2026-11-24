// xbox.c - RetroPad Link original Xbox controller adapter
//
// Tracks whether an original Xbox controller is mounted on the USB host port
// and drives its rumble test.
//
// Mount tracking wraps JoypadOS's XInput mount callbacks with the linker
// (-Wl,--wrap=..., see retropad_target.cmake), so JoypadOS is not modified.
// The wrappers record the device and then call JoypadOS's callbacks.

#include "controllers/xbox.h"

#include "tusb.h"
#include "xinput_host.h"

#include "core/services/players/manager.h"
#include "core/services/players/feedback.h"

#include <stdio.h>


// ============================================================================
// MOUNT TRACKING
// ============================================================================

static bool    xbox_mounted = false;
static uint8_t xbox_dev_addr = 0;
static uint8_t xbox_instance = 0;


void __real_tuh_xinput_mount_cb(
    uint8_t dev_addr,
    uint8_t instance,
    const xinputh_interface_t* xinput_itf
);

void __real_tuh_xinput_umount_cb(
    uint8_t dev_addr,
    uint8_t instance
);


void __wrap_tuh_xinput_mount_cb(
    uint8_t dev_addr,
    uint8_t instance,
    const xinputh_interface_t* xinput_itf
)
{
    // Only the original Xbox controller gets the tester screens. Other
    // XInput pads still reach the PC through JoypadOS.
    if (xinput_itf && xinput_itf->type == XBOXOG) {

        xbox_mounted = true;
        xbox_dev_addr = dev_addr;
        xbox_instance = instance;

        printf("[retropad] Xbox controller mounted (addr %u)\n", dev_addr);
    }

    __real_tuh_xinput_mount_cb(dev_addr, instance, xinput_itf);
}


void __wrap_tuh_xinput_umount_cb(
    uint8_t dev_addr,
    uint8_t instance
)
{
    if (
        xbox_mounted &&
        dev_addr == xbox_dev_addr &&
        instance == xbox_instance
    ) {

        xbox_mounted = false;

        printf("[retropad] Xbox controller removed\n");
    }

    __real_tuh_xinput_umount_cb(dev_addr, instance);
}


// ============================================================================
// CONNECTION STATE
// ============================================================================

bool retropad_xbox_is_connected(void)
{
    return xbox_mounted;
}


// ============================================================================
// RUMBLE
// ============================================================================

// JoypadOS's XInput driver reads each USB player's feedback state and sends
// it to that player's controller, so set the Xbox controller's player slot.
static int xbox_player_index(void)
{
    if (!xbox_mounted) {
        return -1;
    }

    for (int i = 0; i < playersCount; i++) {

        if (
            players[i].dev_addr == xbox_dev_addr &&
            players[i].instance == xbox_instance
        ) {
            return i;
        }
    }

    return -1;
}


void retropad_xbox_set_rumble(bool enabled)
{
    int player = xbox_player_index();

    if (player < 0) {
        return;
    }

    // Left = large (low frequency) motor, right = small (high frequency).
    feedback_set_rumble(
        (uint8_t)player,
        enabled ? 255 : 0,
        enabled ? 255 : 0
    );
}
