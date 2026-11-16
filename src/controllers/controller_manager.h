#ifndef RETROPAD_CONTROLLER_MANAGER_H
#define RETROPAD_CONTROLLER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "core/input_interface.h"

typedef enum {
    RETROPAD_CONTROLLER_NONE = 0,
    RETROPAD_CONTROLLER_SNES,
    RETROPAD_CONTROLLER_NES,
    RETROPAD_CONTROLLER_N64,
    RETROPAD_CONTROLLER_GAMECUBE
} retropad_controller_type_t;

const InputInterface*
retropad_controller_manager_get_input_interface(void);

retropad_controller_type_t
retropad_controller_manager_get_active(void);

bool
retropad_controller_manager_is_connected(void);

const char*
retropad_controller_manager_get_name(void);

#endif
