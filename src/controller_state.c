#include "controller_state.h"

#include "core/router/router.h"

#include <string.h>


// ============================================================================
// CACHED STATE
// ============================================================================

static input_event_t cached_state;

static bool cached_state_valid =
    false;



// ============================================================================
// ROUTER OUTPUT POLLING
// ============================================================================
//
// RetroPad Link reads the latest event routed to USB player 0, the same way
// stock JoypadOS apps (usb2usb, bt2usb, universal) do.
//
// Do NOT use router_set_tap() here: the router keeps a single tap per output,
// and the USB device driver (usbd.c) already registers its own tap on
// OUTPUT_TARGET_USB_DEVICE to receive input. Replacing it leaves the PC
// receiving no controller input at all.
//


// ============================================================================
// INITIALIZATION
// ============================================================================

void controller_state_init(void)
{
    memset(
        &cached_state,
        0,
        sizeof(cached_state)
    );


    cached_state_valid =
        false;
}


// ============================================================================
// UPDATE
// ============================================================================

bool controller_state_update(void)
{
    // Returns each new routed event exactly once; NULL when nothing changed.
    const input_event_t* event =
        router_get_output(
            OUTPUT_TARGET_USB_DEVICE,
            0
        );


    if (event == NULL) {

        return false;
    }


    cached_state =
        *event;


    cached_state_valid =
        true;


    return true;
}


// ============================================================================
// VALIDITY
// ============================================================================

bool controller_state_has_state(void)
{
    return cached_state_valid;
}


// ============================================================================
// CLEAR
// ============================================================================

void controller_state_clear(void)
{
    memset(
        &cached_state,
        0,
        sizeof(cached_state)
    );


    cached_state_valid =
        false;
}


// ============================================================================
// FULL EVENT
// ============================================================================

const input_event_t* controller_state_get(void)
{
    if (!cached_state_valid) {

        return NULL;
    }


    return &cached_state;
}


// ============================================================================
// BUTTONS
// ============================================================================

uint32_t controller_state_get_buttons(void)
{
    if (!cached_state_valid) {

        return 0;
    }


    return cached_state.buttons;
}


// ============================================================================
// LAYOUT
// ============================================================================

controller_layout_t controller_state_get_layout(void)
{
    if (!cached_state_valid) {

        return LAYOUT_UNKNOWN;
    }


    return cached_state.layout;
}


// ============================================================================
// DEVICE TYPE
// ============================================================================

input_device_type_t controller_state_get_type(void)
{
    if (!cached_state_valid) {

        return INPUT_TYPE_NONE;
    }


    return cached_state.type;
}


// ============================================================================
// TRANSPORT
// ============================================================================

input_transport_t controller_state_get_transport(void)
{
    if (!cached_state_valid) {

        return INPUT_TRANSPORT_NONE;
    }


    return cached_state.transport;
}


// ============================================================================
// ANALOG
// ============================================================================

uint8_t controller_state_get_analog(
    analog_axis_index_t axis
)
{
    if (!cached_state_valid) {

        switch (axis) {

            case ANALOG_LX:
            case ANALOG_LY:
            case ANALOG_RX:
            case ANALOG_RY:
                return 128;

            default:
                return 0;
        }
    }


    if (axis >= ANALOG_COUNT) {

        return 0;
    }


    return cached_state.analog[axis];
}
