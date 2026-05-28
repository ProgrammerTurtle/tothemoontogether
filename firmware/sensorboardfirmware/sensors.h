/* sensors.h — Sensor board I2C sensor drivers
 * MS5607, BMP390, SHT40, LSM6DSO32
 * Placeholder implementations for hardware bring-up
 */

#ifndef __SENSORS_H
#define __SENSORS_H

#include "main.h"
#include "can_ids.h"
#include "flight_state.h"
#include <stdint.h>

#define SENSOR_OK    0
#define SENSOR_ERROR 1

/* ── MS5607 ────────────────────────────────────────────────── */
/* Commands */
#define MS5607_CMD_RESET        0x1E
#define MS5607_CMD_CONVERT_D1   0x48  /* Pressure OSR=4096 */
#define MS5607_CMD_CONVERT_D2   0x58  /* Temperature OSR=4096 */
#define MS5607_CMD_READ_ADC     0x00
#define MS5607_CMD_READ_PROM    0xA0  /* + 2*n for coefficient n */

typedef struct {
    uint16_t C[8];       /* PROM calibration coefficients */
    int32_t  pressure_pa;
    int32_t  altitude_mm;
    int32_t  temperature_mc; /* millidegrees C */
} MS5607_Data_t;

uint8_t MS5607_Init(I2C_HandleTypeDef *hi2c);
uint8_t MS5607_Read(I2C_HandleTypeDef *hi2c, MS5607_Data_t *data);
int32_t MS5607_PressureToAltitude(int32_t pressure_pa);

/* ── BMP390 ────────────────────────────────────────────────── */
#define BMP390_REG_CHIP_ID      0x00
#define BMP390_REG_STATUS       0x03
#define BMP390_REG_DATA_0       0x04  /* Pressure XLSB */
#define BMP390_REG_PWR_CTRL     0x1B
#define BMP390_REG_OSR          0x1C
#define BMP390_REG_CALIB_0      0x31
#define BMP390_CHIP_ID          0x60

typedef struct {
    int32_t pressure_pa;
    int32_t temperature_mc;
} BMP390_Data_t;

uint8_t BMP390_Init(I2C_HandleTypeDef *hi2c);
uint8_t BMP390_Read(I2C_HandleTypeDef *hi2c, BMP390_Data_t *data);

/* ── SHT40 ─────────────────────────────────────────────────── */
#define SHT40_CMD_MEASURE_HI    0xFD  /* High precision measurement */

typedef struct {
    int32_t humidity_mpct;    /* milli percent RH */
    int32_t temperature_mc;   /* millidegrees C */
} SHT40_Data_t;

uint8_t SHT40_Init(I2C_HandleTypeDef *hi2c);
uint8_t SHT40_Read(I2C_HandleTypeDef *hi2c, SHT40_Data_t *data);

/* ── LSM6DSO32 ─────────────────────────────────────────────── */
#define LSM6_REG_WHO_AM_I       0x0F
#define LSM6_REG_CTRL1_XL       0x10  /* Accel control */
#define LSM6_REG_CTRL2_G        0x11  /* Gyro control */
#define LSM6_REG_CTRL3_C        0x12  /* General control */
#define LSM6_REG_INT1_CTRL      0x0D  /* INT1 pin control */
#define LSM6_REG_OUTX_L_G       0x22  /* Gyro X LSB */
#define LSM6_REG_OUTX_L_A       0x28  /* Accel X LSB */
#define LSM6_WHO_AM_I           0x6C

/* ODR settings for CTRL1_XL */
#define LSM6_ODR_OFF            0x00
#define LSM6_ODR_104Hz          0x40
#define LSM6_ODR_416Hz          0x60
#define LSM6_ODR_1666Hz         0x80

/* Full scale — ±32G */
#define LSM6_FS_XL_32G          0x06
/* Full scale — gyro ±2000dps */
#define LSM6_FS_G_2000DPS       0x0C

typedef struct {
    int32_t ax_mg, ay_mg, az_mg;   /* milli-G */
    int32_t gx_mdps, gy_mdps, gz_mdps; /* milli-dps */
} LSM6_Data_t;

uint8_t LSM6DSO32_Init(I2C_HandleTypeDef *hi2c);
uint8_t LSM6DSO32_Read(I2C_HandleTypeDef *hi2c, LSM6_Data_t *data);
uint8_t LSM6DSO32_SetODR(I2C_HandleTypeDef *hi2c, uint8_t odr);

/* ── CAN transmit helpers ──────────────────────────────────── */
void Sensors_Send_IMU(FDCAN_HandleTypeDef *hfdcan,
                      I2C_HandleTypeDef *hi2c,
                      FlightState_t state);

void Sensors_Send_Env(FDCAN_HandleTypeDef *hfdcan,
                      I2C_HandleTypeDef *hi2c);

#endif /* __SENSORS_H */
