/* can_ids.h — CAN-FD message ID definitions
 * Shared across MCU board, sensor board, and Geiger board
 * Copy this file into all three projects
 *
 * ID scheme: 11-bit standard CAN ID
 *   Bits [10:8] — source board (0=MCU, 1=Sensor, 2=Geiger)
 *   Bits [7:0]  — message type
 *
 * Lower ID = higher priority on CAN bus
 */

#ifndef __CAN_IDS_H
#define __CAN_IDS_H

/* ── MCU Board (0x0xx) ──────────────────────────────────────── */
#define CAN_ID_MCU_HK           0x001  /* Housekeeping: timestamp, state, battery */
#define CAN_ID_MCU_STATE        0x002  /* Current flight state broadcast */
#define CAN_ID_STATE_CHANGE     0x003  /* State transition notification */

/* ── Sensor Board (0x1xx) ───────────────────────────────────── */
#define CAN_ID_SENSOR_BARO      0x100  /* MS5607 + BMP390: pressure, altitude, temp */
#define CAN_ID_SENSOR_IMU_HI    0x101  /* ADXL375: high-G XYZ accel */
#define CAN_ID_SENSOR_IMU_LO    0x102  /* LSM6DSO32: low-G XYZ accel + gyro */
#define CAN_ID_SENSOR_CHARGE    0x103  /* Charge measurement: HI range + LO range */
#define CAN_ID_SENSOR_ENV       0x104  /* SHT40 + TMP117: humidity + temp */
#define CAN_ID_SENSOR_THERM     0x105  /* Nosecone thermistor: tip temp */
#define CAN_ID_SENSOR_SWEEP     0x106  /* Resonance sweep metadata: freq, alt */
#define CAN_ID_SENSOR_HK        0x107  /* Sensor board housekeeping */

/* ── Geiger Board (0x2xx) ───────────────────────────────────── */
#define CAN_ID_GEIGER_PULSE     0x200  /* Individual pulse timestamp */
#define CAN_ID_GEIGER_COUNT     0x201  /* Integrated count rate (CPM) */
#define CAN_ID_GEIGER_HV        0x202  /* HV rail voltage monitor */
#define CAN_ID_GEIGER_STATUS    0x203  /* Geiger board status/housekeeping */

/* ── Data lengths (bytes) ───────────────────────────────────── */
#define DL_MCU_HK               8
#define DL_SENSOR_BARO          12   /* pressure(4) + altitude(4) + temp(4) */
#define DL_SENSOR_IMU_HI        12   /* x(4) + y(4) + z(4) signed int32 */
#define DL_SENSOR_IMU_LO        24   /* accel xyz(12) + gyro xyz(12) */
#define DL_SENSOR_CHARGE        8    /* hi_range(4) + lo_range(4) */
#define DL_SENSOR_ENV           8    /* humidity(4) + temp(4) */
#define DL_SENSOR_THERM         4    /* tip_temp(4) */
#define DL_SENSOR_SWEEP         12   /* sweep_start_hz(4) + sweep_end_hz(4) + alt_mm(4) */
#define DL_GEIGER_PULSE         4    /* timestamp_ms(4) */
#define DL_GEIGER_COUNT         8    /* timestamp_ms(4) + cpm(4) */
#define DL_GEIGER_HV            4    /* hv_mv(4) millivolts */

#endif /* __CAN_IDS_H */
