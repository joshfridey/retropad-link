#ifndef RETROPAD_CONTROLLER_STATE_H
#define RETROPAD_CONTROLLER_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/input_event.h"


// Initialize cached controller state. Call after router_init().
void controller_state_init(void);


// Poll JoypadOS for a newly-routed controller event (USB player 0).
// Call once per app_task() iteration, before reading cached state.
//
// Returns true if a new event was received.
// Returns false if there was no new event this iteration.
bool controller_state_update(void);


// Has RetroPad Link received at least one routed controller event?
bool controller_state_has_state(void);


// Clear the cached state.
void controller_state_clear(void);


// Get the most recently cached complete JoypadOS input event.
//
// Returns NULL until at least one event has been received.
const input_event_t* controller_state_get(void);


// Convenience accessors.

uint32_t controller_state_get_buttons(void);

controller_layout_t controller_state_get_layout(void);

input_device_type_t controller_state_get_type(void);

input_transport_t controller_state_get_transport(void);

uint8_t controller_state_get_analog(
    analog_axis_index_t axis
);


#endif
