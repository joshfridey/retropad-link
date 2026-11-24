// usb_host_quirks.c - Fixes for USB devices on the USB host port
//
// Applied with linker wrapping (-Wl,--wrap=..., see retropad_target.cmake),
// so neither JoypadOS nor its TinyUSB copy is modified.
//
// Hub descriptor request
// ----------------------
// TinyUSB's hub driver asks for the hub class descriptor with wValue = 0
// (host/hub.c). The USB spec defines wValue as descriptor type << 8 | index,
// i.e. 0x2900 for the hub descriptor, which is what Linux and Windows send.
// Most hubs ignore wValue, but the hub built into original Xbox controllers
// (Duke and Controller S, used for the memory unit slots) stalls the request
// and the controller is never enumerated. The wrapper fills in 0x2900.
//
// Stalled status stage on hub requests
// ------------------------------------
// The same hub sometimes answers the final (status) stage of a hub class
// request with STALL even though the request was carried out: e.g. a
// GET_PORT_STATUS whose data already arrived and shows the port reset done,
// or SET_FEATURE(PORT_RESET / PORT_POWER). This seems to happen when its
// handshake for the status packet is lost and the retried status packet is
// a duplicate, which a compliant device would just acknowledge again.
// TinyUSB treats any failure of a hub request as fatal and abandons the
// controller. The wrapper reports a stalled hub class request as successful
// (data, if any, is already in TinyUSB's buffer; its hub code does not use
// the byte count).

#include "tusb.h"

#include <stdio.h>


#define HUB_CLASS_REQUEST_GET_DESCRIPTOR  6
#define HUB_DESCRIPTOR_TYPE               0x29


// Callback of the hub class request in flight. TinyUSB runs one control
// transfer at a time, so one slot is enough.
static tuh_xfer_cb_t hub_request_cb = NULL;


static void hub_request_complete(tuh_xfer_t* xfer)
{
    tuh_xfer_cb_t cb = hub_request_cb;
    hub_request_cb = NULL;

    if (xfer->result == XFER_RESULT_STALLED) {
        const tusb_control_request_t* r = xfer->setup;
        printf("[usb_quirks] hub %u stalled request %02X %02X wValue %04X "
               "wIndex %u; treated as done\n", xfer->daddr,
               r ? r->bmRequestType : 0, r ? r->bRequest : 0,
               r ? r->wValue : 0, r ? r->wIndex : 0);
        xfer->result = XFER_RESULT_SUCCESS;
    }

    if (cb) {
        cb(xfer);
    }
}


bool __real_tuh_control_xfer(tuh_xfer_t* xfer);


bool __wrap_tuh_control_xfer(tuh_xfer_t* xfer)
{
    const tusb_control_request_t* setup = xfer ? xfer->setup : NULL;

    if (setup &&
        setup->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS &&
        setup->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE &&
        setup->bmRequestType_bit.direction == TUSB_DIR_IN &&
        setup->bRequest == HUB_CLASS_REQUEST_GET_DESCRIPTOR &&
        setup->wValue == 0)
    {
        // TinyUSB copies the setup packet when the transfer is queued, so
        // this only has to outlive the call. Static anyway, in case that
        // ever changes.
        static tusb_control_request_t fixed;

        fixed = *setup;
        fixed.wValue = (uint16_t)(HUB_DESCRIPTOR_TYPE << 8);
        xfer->setup = &fixed;
    }

    // Hub class requests: class type, to the hub itself (device) or to one
    // of its ports (other). HID and other class drivers address interfaces.
    if (setup &&
        xfer->complete_cb &&
        setup->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS &&
        (setup->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE ||
         setup->bmRequestType_bit.recipient == TUSB_REQ_RCPT_OTHER))
    {
        hub_request_cb = xfer->complete_cb;
        xfer->complete_cb = hub_request_complete;
    }

    return __real_tuh_control_xfer(xfer);
}
