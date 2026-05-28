/* flight_state.h — Flight state machine definitions
 * Shared across all three boards
 * Copy into all three projects
 */

#ifndef __FLIGHT_STATE_H
#define __FLIGHT_STATE_H

#include <stdint.h>

typedef enum {
    STATE_GROUND   = 0,  /* On pad, waiting. Low power, slow logging */
    STATE_BOOST    = 1,  /* Motor burning. High-G accel, fast logging */
    STATE_COAST    = 2,  /* Motor out, ascending. Fast logging */
    STATE_APOGEE   = 3,  /* Brief transition at apogee */
    STATE_DESCENT  = 4,  /* Under drogue. Active resonance sweep */
    STATE_LANDED   = 5,  /* Landed. Flush buffers, idle */
} FlightState_t;

/* Thresholds — tune after bench/flight testing */
#define LAUNCH_ACCEL_MG         3000   /* 3G in milli-G to detect launch */
#define LAUNCH_CONFIRM_MS       200    /* Must hold for 200ms */
#define BURNOUT_ACCEL_MG        1000   /* Below 1G = motor out */
#define APOGEE_CONFIRM_MS       500    /* Pressure must be rising for 500ms */
#define LANDED_VARIANCE_MS      10000  /* Low accel variance for 10s = landed */

/* Returns human readable state name for debug printing */
static inline const char* Flight_State_Name(FlightState_t state) {
    switch (state) {
        case STATE_GROUND:   return "GROUND";
        case STATE_BOOST:    return "BOOST";
        case STATE_COAST:    return "COAST";
        case STATE_APOGEE:   return "APOGEE";
        case STATE_DESCENT:  return "DESCENT";
        case STATE_LANDED:   return "LANDED";
        default:             return "UNKNOWN";
    }
}

#endif /* __FLIGHT_STATE_H */
