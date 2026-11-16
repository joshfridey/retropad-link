// config.h - RetroPad Tester Hardware Configuration
//
// Central hardware configuration for the PSTFO RetroPad Tester.
// Keep board-specific GPIO assignments here instead of scattering
// them throughout controller modules and UI code.
//
// Current hardware:
//   Waveshare Core2350B / RP2350B
//
// Current controller ports:
//   SNES
//   NES
//   N64
//   GameCube
//
// OLED:
//   1.3" SH1106 128x64 I2C

#ifndef RETROPAD_CONFIG_H
#define RETROPAD_CONFIG_H


// ============================================================================
// SNES CONTROLLER
// ============================================================================
//
// Physical RetroPad Tester wiring:
//
// GPIO5 = SNES Clock
// GPIO6 = SNES Latch
// GPIO7 = SNES Data
//
// GPIO10 / GPIO11 are retained for optional SNES DATA1 / IOBIT functionality
// used by JoypadOS for accessories such as multitap/mouse/keyboard support.
//
// IMPORTANT:
// GPIO8 and GPIO9 are reserved for the OLED and must NOT be used by SNES.
//

#define RETROPAD_SNES_CLOCK_GPIO       5
#define RETROPAD_SNES_LATCH_GPIO       6
#define RETROPAD_SNES_DATA0_GPIO       7

#define RETROPAD_SNES_DATA1_GPIO      10
#define RETROPAD_SNES_IOBIT_GPIO      11


// ============================================================================
// NES CONTROLLER
// ============================================================================
//
// NES shares SNES Clock (GPIO5) and Latch (GPIO6). Both are outputs, so both
// pads can listen to the same lines. Data is separate so both can stay
// plugged in and the firmware knows which socket a pad is in.
//
// Connector pinout:
//
// 1 = GND
// 2 = Clock -> GPIO5 (shared with SNES)
// 3 = Latch -> GPIO6 (shared with SNES)
// 4 = Data  -> GPIO15
// 5 = not connected (Zapper)
// 6 = not connected (Zapper)
// 7 = 3.3V (not 5V: Data must stay at 3.3V logic)
//

#define RETROPAD_NES_DATA_GPIO        15

// N64 controller Joybus DATA
#define RETROPAD_N64_DATA_GPIO        14


// ============================================================================
// GAMECUBE CONTROLLER
// ============================================================================
//
// GameCube uses a single bidirectional JoyBus data line.
//
// Connector pinout (verified):
//
// 1 = 5V (rumble motor only)
// 2 = DATA  -> GPIO29, with 750 ohm external pull-up to 3.3V
// 3 = GND
// 4 = GND
// 5 = not connected
// 6 = 3.3V (controller logic)
//
// Wire EVERY ground point on the port to GND. Detection only started
// working once all of them were connected: the controller may not tie its
// grounds together internally, so a missing one can leave its logic
// without a ground.
//

#define RETROPAD_GAMECUBE_DATA_GPIO   29


// ============================================================================
// OLED DISPLAY
// ============================================================================
//
// SH1106 128x64 I2C OLED
//
// GPIO8 = SDA
// GPIO9 = SCL
//

#define RETROPAD_OLED_SDA_GPIO         8
#define RETROPAD_OLED_SCL_GPIO         9

#define RETROPAD_OLED_I2C_INSTANCE     0
#define RETROPAD_OLED_I2C_ADDRESS      0x3C


// ============================================================================
// CONTROLLER CONFIGURATION
// ============================================================================

#define RETROPAD_MAX_CONTROLLERS       1
#define RETROPAD_USB_OUTPUT_PORTS      1
#define RETROPAD_MAX_PLAYER_SLOTS      1


// ============================================================================
// DEBUGGING
// ============================================================================

// Show the GameCube handshake result on the waiting screen.
// Set to 0 once GameCube detection is confirmed working.
#define RETROPAD_GC_DEBUG_OVERLAY      1

#define RETROPAD_UART_DEBUG            1


#endif // RETROPAD_CONFIG_H
