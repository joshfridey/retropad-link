/**
 * GamecubeController.c - GameCube Controller Reader (C port)
 *
 * Reads Nintendo GameCube controllers using the joybus protocol over PIO.
 * Ported from GamecubeController.cpp for use in C projects.
 *
 * Based on joybus-pio library by:
 * https://github.com/JonnyHaystack/joybus-pio
 */

#include "GamecubeController.h"
#include "gamecube_definitions.h"
#include "joybus.h"

#include <stdio.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <hardware/timer.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <string.h>

// RetroPad Link temporary GameCube handshake diagnostics
static uint32_t retropad_gc_probe_attempts = 0;
static uint32_t retropad_gc_origin_attempts = 0;
static uint32_t retropad_gc_actual_poll_attempts = 0;

static uint32_t retropad_gc_last_probe_len = 0;
static uint32_t retropad_gc_last_origin_len = 0;
static uint32_t retropad_gc_last_poll_len = 0;

static uint16_t retropad_gc_last_device_id = 0;

// Stick positions the controller reported at plug-in (origin command).
// A console treats these as center; the joystick test does the same.
// Order: stick X, stick Y, C-stick X, C-stick Y.
static uint8_t retropad_gc_origin_sticks[4] = {128, 128, 128, 128};
static bool retropad_gc_origin_valid = false;

// Falling edges seen on the data pin during the last probe.
//   0      = nothing is being driven onto the pin
//   ~9     = probe command sent, controller silent
//   ~34    = probe sent and the controller replied
// Counted by a GPIO interrupt, which sees the pad input even while PIO owns
// the pin.
static volatile uint32_t retropad_gc_edge_count = 0;
static uint32_t retropad_gc_last_probe_edges = 0;
static uint32_t retropad_gc_last_idle_level = 0;
static bool retropad_gc_edge_irq_ready = false;
static uint retropad_gc_edge_pin = 0;

static void __not_in_flash_func(retropad_gc_edge_irq)(void)
{
    if (gpio_get_irq_event_mask(retropad_gc_edge_pin) & GPIO_IRQ_EDGE_FALL) {
        gpio_acknowledge_irq(retropad_gc_edge_pin, GPIO_IRQ_EDGE_FALL);
        retropad_gc_edge_count++;
    }
}

// Probe variants, rotated on each failed probe until one gets a reply:
//   0: PROBE 0x00 at 4 us/bit (N64 speed, joybus-pio default)
//   1: RESET 0xFF at 4 us/bit
//   2: PROBE 0x00 at 5 us/bit (what a real GameCube console sends)
//   3: RESET 0xFF at 5 us/bit
// Once a variant gets a valid reply it is kept for the rest of the
// handshake and polling.
#define RETROPAD_GC_VARIANT_COUNT 4

static uint8_t retropad_gc_variant = 0;
static bool retropad_gc_variant_locked = false;
static uint32_t retropad_gc_variant_replies[RETROPAD_GC_VARIANT_COUNT] = { 0 };

static void retropad_gc_apply_variant(GamecubeController* controller)
{
    bool slow = retropad_gc_variant >= 2;

    // 40 PIO cycles per bit (joybus T1 + T2 + T3).
    float bitrate = slow ? 200000.0f : 250000.0f;
    float div = (float)clock_get_hz(clk_sys) / (40.0f * bitrate);

    // Every send/receive re-initialises the state machine from this config.
    sm_config_set_clkdiv(&controller->_port.config, div);
}

static void retropad_gc_edge_irq_init(uint pin)
{
    if (retropad_gc_edge_irq_ready) {
        return;
    }

    retropad_gc_edge_pin = pin;
    gpio_add_raw_irq_handler(pin, retropad_gc_edge_irq);
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);

    retropad_gc_edge_irq_ready = true;
}

// Timing constants
#define GC_INCOMING_BIT_LENGTH_US 4
// Give a whole 5 bytes of leniency on receive so the controller has time to do processing
// before responding. And another byte of leniency because the joybus_receive_bytes (and thus
// the timeout) starts soon after the PIO starts sending our first byte.
// Bumped from 200µs to 1ms — gba-as-controller payload halts the GBA CPU
// between SIO interrupts; it needs a moment to wake up and respond.
#define GC_RECEIVE_TIMEOUT_US 1000

// ============================================================================
// Internal Functions
// ============================================================================

static void __no_inline_not_in_flash_func(GamecubeController_wait_poll_cooldown)(GamecubeController* controller)
{
    // Wait for start of next polling period
    while (!time_reached(controller->_next_poll)) {
        tight_loop_contents();
    }

    // Reset poll cooldown
    controller->_next_poll = make_timeout_time_us(controller->_polling_period_us);
}

static bool __no_inline_not_in_flash_func(GamecubeController_do_init)(GamecubeController* controller)
{
    // Send probe command
    retropad_gc_probe_attempts++;

    retropad_gc_edge_irq_init(controller->_port.pin);
    retropad_gc_last_idle_level = gpio_get(controller->_port.pin);
    uint32_t edges_before = retropad_gc_edge_count;

    retropad_gc_apply_variant(controller);

    uint8_t probe_cmd[] = {
        (retropad_gc_variant & 1) ? GamecubeCommand_RESET : GamecubeCommand_PROBE
    };
    joybus_send_bytes(&controller->_port, probe_cmd, sizeof(probe_cmd));

    // Read and validate probe response (3 bytes: device ID + status)
    uint received_len = joybus_receive_bytes(
        &controller->_port,
        (uint8_t*)&controller->_status,
        sizeof(gc_status_t),
        GC_RECEIVE_TIMEOUT_US,
        true
    );

    retropad_gc_last_probe_len = received_len;
    retropad_gc_last_device_id = controller->_status.device;
    retropad_gc_last_probe_edges = retropad_gc_edge_count - edges_before;

    if (received_len > 0) {
        retropad_gc_variant_replies[retropad_gc_variant]++;
    }

    if (received_len == sizeof(gc_status_t) && controller->_status.device != 0) {
        retropad_gc_variant_locked = true;
    } else if (!retropad_gc_variant_locked) {
        retropad_gc_variant = (retropad_gc_variant + 1) % RETROPAD_GC_VARIANT_COUNT;
    }

    // If response is invalid, return false
    if (received_len != sizeof(gc_status_t) || controller->_status.device == 0) {
        return false;
    }

    // Wait until start of next polling period before sending origin
    GamecubeController_wait_poll_cooldown(controller);

    // Send origin command (0x41)
    retropad_gc_origin_attempts++;

    uint8_t origin_cmd[] = { GamecubeCommand_ORIGIN };
    joybus_send_bytes(&controller->_port, origin_cmd, sizeof(origin_cmd));

    // Read and validate origin response (10 bytes: gc_origin_t)
    gc_origin_t origin;
    received_len = joybus_receive_bytes(
        &controller->_port,
        (uint8_t*)&origin,
        sizeof(gc_origin_t),
        GC_RECEIVE_TIMEOUT_US,
        true
    );

    retropad_gc_last_origin_len = received_len;

    // If response is invalid, return false
    if (received_len != sizeof(gc_origin_t)) {
        return false;
    }

    retropad_gc_origin_sticks[0] = origin.initial_inputs.stick_x;
    retropad_gc_origin_sticks[1] = origin.initial_inputs.stick_y;
    retropad_gc_origin_sticks[2] = origin.initial_inputs.cstick_x;
    retropad_gc_origin_sticks[3] = origin.initial_inputs.cstick_y;
    retropad_gc_origin_valid = true;

    return true;
}

// ============================================================================
// Public API
// ============================================================================

void GamecubeController_init(GamecubeController* controller, uint pin, uint polling_rate,
                              PIO pio, int sm, int offset)
{
    memset(controller, 0, sizeof(GamecubeController));

    joybus_port_init(&controller->_port, pin, pio, sm, offset);
    controller->_polling_period_us = 1000000 / polling_rate;
    controller->_next_poll = get_absolute_time();
    controller->_initialized = false;

    // Initialize status to zero
    memset(&controller->_status, 0, sizeof(gc_status_t));
}

void GamecubeController_terminate(GamecubeController* controller)
{
    joybus_port_terminate(&controller->_port);
}

bool __no_inline_not_in_flash_func(GamecubeController_Poll)(GamecubeController* controller,
                                                             gc_report_t* report, bool rumble)
{
    static uint8_t init_fail_count = 0;

    // If controller is uninitialized, do probe/origin sequence
    if (!controller->_initialized) {
        // When not initialized, use non-blocking check instead of blocking wait
        // This prevents starving other tasks when no GC controller is connected
        if (!time_reached(controller->_next_poll)) {
            return false;  // Not time to retry yet, return immediately
        }

        // Backoff: short for reconnection attempts, longer if no controller
        // First 5 retries: 16ms (quick reconnect after brief glitch)
        // After that: 500ms (controller likely not connected)
        uint32_t backoff_ms = (init_fail_count < 5) ? 16 : 500;
        controller->_next_poll = make_timeout_time_ms(backoff_ms);

        controller->_initialized = GamecubeController_do_init(controller);
        if (!controller->_initialized) {
            if (init_fail_count < 255) init_fail_count++;
            return false;
        }
        init_fail_count = 0;  // Reset on success
    }

    // Non-blocking cooldown check for regular polling
    // This prevents starving other tasks
    if (!time_reached(controller->_next_poll)) {
        return false;  // Not time to poll yet, return immediately
    }
    controller->_next_poll = make_timeout_time_us(controller->_polling_period_us);

    // Send poll command: 0x40 + mode (0x03) + rumble byte
    // Mode 0x03 returns full 8-byte report with analog sticks and triggers
    retropad_gc_actual_poll_attempts++;

    uint8_t poll_cmd[] = { GamecubeCommand_POLL, 0x03, rumble ? 1 : 0 };
    joybus_send_bytes(&controller->_port, poll_cmd, sizeof(poll_cmd));

    // Read and validate report (8 bytes)
    uint8_t received_len = joybus_receive_bytes(
        &controller->_port,
        (uint8_t*)report,
        sizeof(gc_report_t),
        GC_RECEIVE_TIMEOUT_US,
        true
    );

    retropad_gc_last_poll_len = received_len;

    // If report origin bit is 1, it indicates that the controller is not initialized properly,
    // so we want to restart the initialization process.
    if (received_len != sizeof(gc_report_t) || report->origin) {
        controller->_initialized = false;
        return false;
    }

    return true;
}

int GamecubeController_GetOffset(GamecubeController* controller)
{
    return controller->_port.offset;
}

bool GamecubeController_IsInitialized(GamecubeController* controller)
{
    return controller->_initialized;
}

const gc_status_t* GamecubeController_GetStatus(GamecubeController* controller)
{
    return &controller->_status;
}


// ============================================================================
// RETROPAD LINK TEMPORARY HANDSHAKE DEBUG API
// ============================================================================

// Copy the plug-in origin into out[4]. Returns false before the first
// successful handshake (out then holds 128s).
bool retropad_gc_get_origin_sticks(uint8_t out[4])
{
    memcpy(out, retropad_gc_origin_sticks, 4);
    return retropad_gc_origin_valid;
}

uint32_t retropad_gc_debug_get_probe_attempts(void)
{
    return retropad_gc_probe_attempts;
}

uint32_t retropad_gc_debug_get_origin_attempts(void)
{
    return retropad_gc_origin_attempts;
}

uint32_t retropad_gc_debug_get_actual_poll_attempts(void)
{
    return retropad_gc_actual_poll_attempts;
}

uint32_t retropad_gc_debug_get_last_probe_len(void)
{
    return retropad_gc_last_probe_len;
}

uint32_t retropad_gc_debug_get_last_origin_len(void)
{
    return retropad_gc_last_origin_len;
}

uint32_t retropad_gc_debug_get_last_poll_len(void)
{
    return retropad_gc_last_poll_len;
}

uint16_t retropad_gc_debug_get_last_device_id(void)
{
    return retropad_gc_last_device_id;
}


uint32_t retropad_gc_debug_get_last_probe_edges(void)
{
    return retropad_gc_last_probe_edges;
}

uint32_t retropad_gc_debug_get_idle_level(void)
{
    return retropad_gc_last_idle_level;
}

// Replies seen for one probe variant (see retropad_gc_apply_variant).
uint32_t retropad_gc_debug_get_variant_replies(uint8_t variant)
{
    if (variant >= RETROPAD_GC_VARIANT_COUNT) {
        return 0;
    }

    return retropad_gc_variant_replies[variant];
}
