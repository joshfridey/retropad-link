#include "joystick_diag.h"

#include "pico/stdlib.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


// ============================================================================
// PROFILES
// ============================================================================
//
// N64 thresholds are starting points based on commonly quoted figures for
// OEM sticks (new ~80-85, noticeably worn below ~65). Tune as needed.
//

const joystick_profile_t joystick_profile_n64 = {
    .name = "N64",
    .full_scale = 100,
    .reference_reach = 84,
    .good_reach = 75,
    .fair_reach = 65,
    .center_tolerance = 6,
};


// GameCube values are raw units measured from the origin the controller
// reports at plug-in. Thresholds are provisional: tune them against a
// known-good OEM controller. The C-stick gate is smaller than the main one.

const joystick_profile_t joystick_profile_gc_main = {
    .name = "GC",
    .full_scale = 115,
    .reference_reach = 100,
    .good_reach = 85,
    .fair_reach = 75,
    .center_tolerance = 6,
};


const joystick_profile_t joystick_profile_gc_cstick = {
    .name = "GC C-",
    .full_scale = 115,
    .reference_reach = 90,
    .good_reach = 75,
    .fair_reach = 65,
    .center_tolerance = 6,
};


// DualShock sticks report 0-255 with 128 as centre; a healthy stick reaches
// (or nearly reaches) the ends of the range. Centres commonly sit a few units
// off 128. Thresholds are provisional: tune them against good pads.

const joystick_profile_t joystick_profile_psx_left = {
    .name = "PS L",
    .full_scale = 135,
    .reference_reach = 127,
    .good_reach = 115,
    .fair_reach = 100,
    .center_tolerance = 10,
};


const joystick_profile_t joystick_profile_psx_right = {
    .name = "PS R",
    .full_scale = 135,
    .reference_reach = 127,
    .good_reach = 115,
    .fair_reach = 100,
    .center_tolerance = 10,
};


// ============================================================================
// REST DETECTION
// ============================================================================

// Stick must hold still this long before its position counts as "at rest".
#define REST_SETTLE_US      300000

// Movement of this many units or less still counts as holding still.
#define REST_JITTER         1


// ============================================================================
// STATE
// ============================================================================

static joystick_diag_t diags[JOYSTICK_DIAG_MAX_STICKS];

static const joystick_profile_t* profiles[JOYSTICK_DIAG_MAX_STICKS] = {
    &joystick_profile_n64,
};

static uint64_t steady_since_us[JOYSTICK_DIAG_MAX_STICKS];

static uint8_t stick_count =
    1;


// ============================================================================
// RESET
// ============================================================================

void joystick_diag_reset(
    const joystick_profile_t* main_stick,
    const joystick_profile_t* second_stick
)
{
    memset(
        diags,
        0,
        sizeof(diags)
    );

    memset(
        steady_since_us,
        0,
        sizeof(steady_since_us)
    );


    if (main_stick) {

        profiles[0] =
            main_stick;

        profiles[1] =
            second_stick;

        stick_count =
            second_stick ? 2 : 1;
    }
}


uint8_t joystick_diag_get_stick_count(void)
{
    return stick_count;
}


// ============================================================================
// UPDATE
// ============================================================================

static void update_reach(
    int16_t* reach,
    int16_t value
)
{
    if (value > *reach) {
        *reach = value;
    }
}


bool joystick_diag_update(
    uint8_t stick,
    int16_t x,
    int16_t y
)
{
    if (stick >= stick_count) {
        return false;
    }


    joystick_diag_t* d =
        &diags[stick];

    const joystick_profile_t* profile =
        profiles[stick];


    uint64_t now =
        time_us_64();


    bool moved =
        abs(x - d->x) > REST_JITTER ||
        abs(y - d->y) > REST_JITTER;


    bool changed =
        x != d->x ||
        y != d->y;


    d->x = x;
    d->y = y;


    // ------------------------------------------------------------------------
    // CARDINAL REACH
    // ------------------------------------------------------------------------

    update_reach(&d->reach_right, x);
    update_reach(&d->reach_left, -x);
    update_reach(&d->reach_up, y);
    update_reach(&d->reach_down, -y);


    // ------------------------------------------------------------------------
    // OUTLINE TRACE
    // ------------------------------------------------------------------------

    float radius =
        sqrtf((float)x * x + (float)y * y);


    if (radius >= 1.0f) {

        float angle =
            atan2f((float)y, (float)x);


        if (angle < 0.0f) {
            angle += 2.0f * (float)M_PI;
        }


        int bin =
            (int)(angle * JOYSTICK_DIAG_TRACE_BINS / (2.0f * (float)M_PI) + 0.5f) %
            JOYSTICK_DIAG_TRACE_BINS;


        uint8_t r =
            radius > 255.0f ? 255 : (uint8_t)(radius + 0.5f);


        if (r > d->trace[bin]) {
            d->trace[bin] = r;
        }
    }


    // ------------------------------------------------------------------------
    // REST POSITION
    // ------------------------------------------------------------------------
    //
    // Record where the stick settles once it has been still for a moment
    // near the middle. A stick that settles well away from zero is not
    // returning to center (worn spring, debris, or mis-calibrated at power-on).
    //

    if (moved || steady_since_us[stick] == 0) {

        steady_since_us[stick] =
            now;
    }

    else if (now - steady_since_us[stick] >= REST_SETTLE_US) {

        int16_t near_center =
            profile->reference_reach / 4;


        bool in_center_zone =
            abs(x) <= near_center &&
            abs(y) <= near_center;


        if (
            in_center_zone &&
            (
                !d->rest_valid ||
                d->rest_x != x ||
                d->rest_y != y
            )
        ) {

            d->rest_valid = true;
            d->rest_x = x;
            d->rest_y = y;

            changed = true;
        }
    }


    return changed;
}


// ============================================================================
// ACCESSORS
// ============================================================================

const joystick_diag_t* joystick_diag_get(uint8_t stick)
{
    return &diags[stick < stick_count ? stick : 0];
}


const joystick_profile_t* joystick_diag_get_profile(uint8_t stick)
{
    return profiles[stick < stick_count ? stick : 0];
}


joystick_verdict_t joystick_diag_get_verdict(uint8_t stick)
{
    const joystick_diag_t* d =
        joystick_diag_get(stick);

    const joystick_profile_t* profile =
        joystick_diag_get_profile(stick);

    // Every direction must have been pushed at least halfway before the
    // reach figures mean anything.

    int16_t halfway =
        profile->reference_reach / 2;


    if (
        d->reach_up < halfway ||
        d->reach_down < halfway ||
        d->reach_left < halfway ||
        d->reach_right < halfway
    ) {

        return JOYSTICK_VERDICT_INCOMPLETE;
    }


    if (
        d->rest_valid &&
        (
            abs(d->rest_x) > profile->center_tolerance ||
            abs(d->rest_y) > profile->center_tolerance
        )
    ) {

        return JOYSTICK_VERDICT_OFF_CENTER;
    }


    int16_t weakest =
        d->reach_up;

    if (d->reach_down < weakest) weakest = d->reach_down;
    if (d->reach_left < weakest) weakest = d->reach_left;
    if (d->reach_right < weakest) weakest = d->reach_right;


    if (weakest >= profile->good_reach) {
        return JOYSTICK_VERDICT_GOOD;
    }


    if (weakest >= profile->fair_reach) {
        return JOYSTICK_VERDICT_FAIR;
    }


    return JOYSTICK_VERDICT_WORN;
}
