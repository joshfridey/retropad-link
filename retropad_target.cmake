# RetroPad Link target definition
#
# Loaded after JoypadOS has finished defining its source lists,
# helper functions, libraries, and board configuration.
#
# RetroPad Link remains external to the JoypadOS source tree.

message(STATUS "RetroPad Link: defining target")


# ============================================================================
# SAFETY
# ============================================================================

if(TARGET joypad_retropad_link)
    message(STATUS "RetroPad Link: target already defined")
    return()
endif()


# ============================================================================
# TARGET
# ============================================================================

add_executable(joypad_retropad_link)


# ============================================================================
# COMPILE DEFINITIONS
# ============================================================================
#
# RetroPad Link currently uses the SNES2USB configuration path for the active
# application behavior.
#
# GameCube host on GPIO29 (see RETROPAD_GAMECUBE_DATA_GPIO in config.h).
#
# PIO blocks: pio0 holds the WS2812 LED program and one joybus program shared
# by the N64 and GameCube hosts (separate state machines). pio1 is kept free
# for a PIO-USB host port, which needs a whole block. PlayStation's poll
# engine goes on pio2. The board's "user button" defaults to GPIO7 (SNES data), so the
# driver's user-button -> PS button mapping is switched off.
#

target_compile_definitions(
    joypad_retropad_link
    PRIVATE

    CONFIG_SNES2USB=1
    CONFIG_USB=1

    # USB host port: PIO-USB on rhport 1 (pins set by app_init() from
    # RETROPAD_USB_HOST_DP_GPIO in config.h). The native USB port stays the
    # USB device to the PC. TinyUSB's board file (tinyusb_board) needs the
    # D+ pin as a define to compile; keep it equal to the config.h value.
    BOARD_TUH_RHPORT=1
    PICO_DEFAULT_PIO_USB_DP_PIN=16

    GC_PIN_DATA=29

    PSX_PIO=pio2
    PSX_USER_BUTTON_A1=0
    PSX_KEEP_ANALOG=0
)


# ============================================================================
# SOURCES
# ============================================================================

target_sources(
    joypad_retropad_link
    PUBLIC

    ${CORE_SOURCES}
    ${USB_DEVICE_SOURCES}

    # USB host (PIO-USB): HID and XInput drivers, including original Xbox
    ${USB_HOST_SOURCES}

    # Native controller hosts
    ${SNES_HOST_SOURCES}

    # N64 host - RetroPad-local copy of the JoypadOS driver
    ${RETROPAD_LINK_DIR}/src/controllers/n64/n64_host.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/joybus-pio/src/N64Controller.c

    # GameCube host - RetroPad-local copy of the JoypadOS driver
    ${RETROPAD_LINK_DIR}/src/controllers/gc/gc_host.c
    ${CMAKE_CURRENT_SOURCE_DIR}/native/host/gc/gba_multiboot.c
    ${RETROPAD_LINK_DIR}/src/controllers/gba_payload_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/native/host/gc/joybus_bridge.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/joybus-pio/src/joybus.c
    ${RETROPAD_LINK_DIR}/src/controllers/gc/GamecubeController.c

    # PlayStation host - RetroPad-local copy of the JoypadOS driver
    ${RETROPAD_LINK_DIR}/src/controllers/psx/psx_host.c

    # RetroPad Link
    ${RETROPAD_LINK_DIR}/src/app.c
    ${RETROPAD_LINK_DIR}/src/diagnostics.c
    ${RETROPAD_LINK_DIR}/src/joystick_diag.c
    ${RETROPAD_LINK_DIR}/src/ui.c
    ${RETROPAD_LINK_DIR}/src/controller_state.c
    ${RETROPAD_LINK_DIR}/src/controllers/snes.c
    ${RETROPAD_LINK_DIR}/src/controllers/nes.c
    ${RETROPAD_LINK_DIR}/src/controllers/n64.c
    ${RETROPAD_LINK_DIR}/src/controllers/playstation.c
    ${RETROPAD_LINK_DIR}/src/controllers/gamecube.c
    ${RETROPAD_LINK_DIR}/src/controllers/controller_manager.c
    ${RETROPAD_LINK_DIR}/src/usb_host_quirks.c

    # Display
    ${CMAKE_CURRENT_SOURCE_DIR}/core/services/display/display.c
    ${CMAKE_CURRENT_SOURCE_DIR}/core/services/display/display_i2c.c
)


# ============================================================================
# INCLUDE PATHS
# ============================================================================

target_include_directories(
    joypad_retropad_link
    PUBLIC

    # RetroPad Link
    ${RETROPAD_LINK_DIR}/src

    # JoypadOS root
    ${CMAKE_CURRENT_SOURCE_DIR}

    # USB device
    ${CMAKE_CURRENT_SOURCE_DIR}/usb/usbd

    # N64 (n64_host.h; the driver itself is RetroPad Link's copy)
    ${CMAKE_CURRENT_SOURCE_DIR}/native/host/n64

    # SNES
    ${CMAKE_CURRENT_SOURCE_DIR}/native/host/snes
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/SNESpad/src

    # GameCube / Joybus
    ${CMAKE_CURRENT_SOURCE_DIR}/native/host/gc
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/joybus-pio/include

    # PlayStation (psx_host.h; the driver itself is RetroPad Link's copy)
    ${CMAKE_CURRENT_SOURCE_DIR}/native/host/psx
)


# ============================================================================
# LIBRARIES
# ============================================================================

target_link_libraries(
    joypad_retropad_link
    PRIVATE

    pico_stdlib
    pico_multicore

    hardware_pio
    hardware_dma
    hardware_i2c

    pico_rand

    tinyusb_device
    tinyusb_board

    tinyusb_host
    xinput_host
)


# ============================================================================
# RETROPAD LINK COMMON TARGET SETUP
# ============================================================================
#
# This mirrors the useful parts of JoypadOS joypad_target_common(), but avoids
# creating duplicate PIO-header rules in JoypadOS's shared build directory.
#

target_compile_options(
    joypad_retropad_link
    PRIVATE

    -O3
)

# USB host device fixes (src/usb_host_quirks.c) wrap TinyUSB functions
# instead of modifying TinyUSB.
target_link_options(
    joypad_retropad_link
    PRIVATE

    -Wl,--wrap=tuh_control_xfer
)


# ============================================================================
# EXTRA OUTPUT FILES
# ============================================================================

pico_add_extra_outputs(
    joypad_retropad_link
)


# ============================================================================
# RETROPAD PIO OUTPUT DIRECTORY
# ============================================================================
#
# All PIO headers owned by the RetroPad Link target are generated into a
# dedicated directory. This prevents collisions with JoypadOS targets that
# generate the same headers elsewhere in the build.
#

set(
    RETROPAD_PIO_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/retropad_link_pio"
)


# ============================================================================
# WS2812 PIO HEADER
# ============================================================================

pico_generate_pio_header(
    joypad_retropad_link

    "${CMAKE_SOURCE_DIR}/core/services/leds/neopixel/ws2812.pio"

    OUTPUT_DIR
    "${RETROPAD_PIO_OUTPUT_DIR}"
)


# ============================================================================
# GAMECUBE JOYBUS PIO HEADER
# ============================================================================
#
# JoypadOS's GameCube host uses joybus-pio. The normal gc2usb target generates
# joybus.pio.h for this purpose.
#

pico_generate_pio_header(
    joypad_retropad_link

    "${CMAKE_SOURCE_DIR}/lib/joybus-pio/src/joybus.pio"

    OUTPUT_DIR
    "${RETROPAD_PIO_OUTPUT_DIR}"
)


# ============================================================================
# PLAYSTATION PIO HEADER
# ============================================================================
#
# From RetroPad Link's copy of the PSX driver, not the JoypadOS one.
#

pico_generate_pio_header(
    joypad_retropad_link

    "${RETROPAD_LINK_DIR}/src/controllers/psx/psx.pio"

    OUTPUT_DIR
    "${RETROPAD_PIO_OUTPUT_DIR}"
)


# ============================================================================
# COMMON INCLUDE PATHS
# ============================================================================

target_include_directories(
    joypad_retropad_link
    PUBLIC

    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/common"
    "${CMAKE_SOURCE_DIR}/lib/libxsm3"

    "${RETROPAD_PIO_OUTPUT_DIR}"
)


# ============================================================================
# JOYPADOS VERSION / BUILD INFORMATION
# ============================================================================

target_compile_definitions(
    joypad_retropad_link
    PRIVATE

    JOYPAD_VERSION="${JOYPAD_VERSION}"
    GIT_COMMIT="${GIT_COMMIT}"
    BUILD_TIME="${BUILD_TIME}"
    SINPUT_BCD_DEVICE=${JOYPAD_BCD_DEVICE}
)


# ============================================================================
# BOARD NAME
# ============================================================================

target_compile_definitions(
    joypad_retropad_link
    PRIVATE

    BOARD_NAME="${PICO_BOARD}"
)


message(STATUS "RetroPad Link: target ready")

# ============================================================================
# RETROPAD LINK STDIO
# ============================================================================
# UART stdio stays off: waveshare_core2350b defaults UART TX/RX to GPIO12/13.
# The SDK's USB stdio is off too: it disables itself whenever TinyUSB host is
# linked (the USB host port). printf output goes to JoypadOS's own log on its
# USB serial (CDC) port instead.
pico_enable_stdio_uart(joypad_retropad_link 0)
pico_enable_stdio_usb(joypad_retropad_link 0)


# ============================================================================
# PIO-USB (USB HOST PORT)
# ============================================================================
#
# Same as JoypadOS's tinyusb_pico_pio_usb library, except pio_usb_host.c is
# RetroPad Link's modified copy (src/usb_host/pio_usb_host.c: more
# transaction retries for the original Xbox controller's hub).

set(RETROPAD_PIO_USB_PATH "${CMAKE_SOURCE_DIR}/lib/Pico-PIO-USB")

target_sources(
    joypad_retropad_link
    PUBLIC

    ${RETROPAD_PIO_USB_PATH}/src/pio_usb.c
    ${RETROPAD_LINK_DIR}/src/usb_host/pio_usb_host.c
    ${RETROPAD_PIO_USB_PATH}/src/pio_usb_device.c
    ${RETROPAD_PIO_USB_PATH}/src/usb_crc.c
    ${CMAKE_SOURCE_DIR}/lib/tinyusb/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

target_include_directories(
    joypad_retropad_link
    PUBLIC

    ${RETROPAD_PIO_USB_PATH}/src
)

target_compile_definitions(
    joypad_retropad_link
    PRIVATE

    PIO_USB_USE_TINYUSB=1
    PIO_USB_DP_PIN_DEFAULT=16
)

pico_generate_pio_header(
    joypad_retropad_link

    "${RETROPAD_PIO_USB_PATH}/src/usb_tx.pio"

    OUTPUT_DIR
    "${RETROPAD_PIO_OUTPUT_DIR}"
)

pico_generate_pio_header(
    joypad_retropad_link

    "${RETROPAD_PIO_USB_PATH}/src/usb_rx.pio"

    OUTPUT_DIR
    "${RETROPAD_PIO_OUTPUT_DIR}"
)
