// app.h - RetroPad Tester App Manifest
//
// PSTFO RetroPad Tester
//
// Current support:
//   SNES / NES native controller input
//   USB HID gamepad output
//
// This manifest will gradually become controller-agnostic as additional
// controller modules are added.

#ifndef APP_SNES2USB_H
#define APP_SNES2USB_H

#include "config.h"


// ============================================================================
// APP METADATA
// ============================================================================

#define APP_NAME "RetroPad Tester"
#define APP_DESCRIPTION "Universal retro controller tester and USB adapter"
#define APP_AUTHOR "Press Start to Feel Old"


// ============================================================================
// CORE DEPENDENCIES
// ============================================================================

// Input drivers - Native SNES host
#define REQUIRE_NATIVE_SNES_HOST 1

#define SNES_MAX_CONTROLLERS \
    RETROPAD_MAX_CONTROLLERS


// Output drivers
#define REQUIRE_USB_DEVICE 1

#define USB_OUTPUT_PORTS \
    RETROPAD_USB_OUTPUT_PORTS


// Services
#define REQUIRE_PLAYER_MANAGEMENT 1


// ============================================================================
// PIN CONFIGURATION
// ============================================================================
//
// Keep the JoypadOS names expected by the SNES driver,
// but source the actual GPIO values from config.h.
//

#define SNES_PIN_CLOCK \
    RETROPAD_SNES_CLOCK_GPIO

#define SNES_PIN_LATCH \
    RETROPAD_SNES_LATCH_GPIO

#define SNES_PIN_DATA0 \
    RETROPAD_SNES_DATA0_GPIO

#define SNES_PIN_DATA1 \
    RETROPAD_SNES_DATA1_GPIO

#define SNES_PIN_IOBIT \
    RETROPAD_SNES_IOBIT_GPIO


// ============================================================================
// ROUTING CONFIGURATION
// ============================================================================

#define ROUTING_MODE ROUTING_MODE_SIMPLE
#define MERGE_MODE MERGE_ALL


// ============================================================================
// PLAYER MANAGEMENT
// ============================================================================

#define PLAYER_SLOT_MODE PLAYER_SLOT_FIXED

#define MAX_PLAYER_SLOTS \
    RETROPAD_MAX_PLAYER_SLOTS

#define AUTO_ASSIGN_ON_PRESS 1


// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================
//
// BOARD is currently retained because JoypadOS expects it in the manifest.
// The actual build is using the custom waveshare_core2350b board definition.
//

#define BOARD "ada_kb2040"

#define CPU_OVERCLOCK_KHZ 0

#define UART_DEBUG \
    RETROPAD_UART_DEBUG


// ============================================================================
// APP FEATURES
// ============================================================================

#define FEATURE_MOUSE_SUPPORT 1


// ============================================================================
// APP INTERFACE
// ============================================================================

void app_init(void);


#endif // APP_SNES2USB_H
