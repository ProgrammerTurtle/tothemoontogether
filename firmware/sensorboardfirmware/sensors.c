/* sensors.c — I2C sensor driver implementations
 * Placeholder/bring-up level — minimal error handling
 * TODO: add full calibration compensation for MS5607 and BMP390
 */

#include "sensors.h"
#include <string.h>
#include <stdio.h>

/* ── MS5607 ─────────────────────────────────────────────────── */

static MS5607_Data_t ms5607_ctx = {0};

static uint8_t MS5607_SendCmd(I2C_HandleTypeDef *hi2c, uint8_t cmd) {
    return HAL_I2C_Master_Transmit(hi2c, MS5607_ADDR, &cmd, 1, 100);
}

static uint32_t MS5607_ReadADC(I2C_HandleTypeDef *hi2c) {
    uint8_t buf[3] = {0};
    uint8_t cmd = MS5607_CMD_READ_ADC;
    HAL_I2C_Master_Transmit(hi2c, MS5607_ADDR, &cmd, 1, 100);
    HAL_I2C_Master_Receive(hi2c, MS5607_ADDR, buf, 3, 100);
    return ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
}

uint8_t MS5607_Init(I2C_HandleTypeDef *hi2c) {
    /* Reset */
    if (MS5607_SendCmd(hi2c, MS5607_CMD_RESET) != HAL_OK)
        return SENSOR_ERROR;
    HAL_Delay(10);

    /* Read PROM calibration coefficients C1-C6 */
    for (int i = 1; i <= 6; i++) {
        uint8_t cmd = MS5607_CMD_READ_PROM | (i << 1);
        uint8_t buf[2] = {0};
        if (HAL_I2C_Master_Transmit(hi2c, MS5607_ADDR, &cmd, 1, 100) != HAL_OK)
            return SENSOR_ERROR;
        if (HAL_I2C_Master_Receive(hi2c, MS5607_ADDR, buf, 2, 100) != HAL_OK)
            return SENSOR_ERROR;
        ms5607_ctx.C[i] = ((uint16_t)buf[0] << 8) | buf[1];
        printf("[MS5607] C%d = %u\r\n", i, ms5607_ctx.C[i]);
    }
    return SENSOR_OK;
}

uint8_t MS5607_Read(I2C_HandleTypeDef *hi2c, MS5607_Data_t *data) {
    /* Convert pressure */
    MS5607_SendCmd(hi2c, MS5607_CMD_CONVERT_D1);
    HAL_Delay(10);
    uint32_t D1 = MS5607_ReadADC(hi2c);

    /* Convert temperature */
    MS5607_SendCmd(hi2c, MS5607_CMD_CONVERT_D2);
    HAL_Delay(10);
    uint32_t D2 = MS5607_ReadADC(hi2c);

    /* Calculate temperature — MS5607 datasheet equations */
    int32_t dT   = (int32_t)D2 - ((int32_t)ms5607_ctx.C[5] << 8);
    int32_t TEMP = 2000 + ((int64_t)dT * ms5607_ctx.C[6] >> 23);

    /* Calculate pressure */
    int64_t OFF  = ((int64_t)ms5607_ctx.C[2] << 17)
                 + (((int64_t)ms5607_ctx.C[4] * dT) >> 6);
    int64_t SENS = ((int64_t)ms5607_ctx.C[1] << 16)
                 + (((int64_t)ms5607_ctx.C[3] * dT) >> 7);
    int32_t P    = (int32_t)(((D1 * SENS >> 21) - OFF) >> 15);

    data->temperature_mc = TEMP * 10;  /* Convert 0.01C to mC */
    data->pressure_pa    = P;
    data->altitude_mm    = MS5607_PressureToAltitude(P);

    memcpy(&ms5607_ctx, data, sizeof(MS5607_Data_t));
    return SENSOR_OK;
}

int32_t MS5607_PressureToAltitude(int32_t pressure_pa) {
    /* International Standard Atmosphere approximation
     * Valid to ~11km. TODO: extend for stratosphere.
     * h = 44330 * (1 - (P/P0)^(1/5.255)) metres
     * Simplified integer version — returns mm */
    if (pressure_pa <= 0) return 0;
    /* Placeholder: linear approximation near sea level */
    /* Replace with proper ISA calculation in firmware dev */
    int32_t delta = 101325 - pressure_pa;
    return (int32_t)((int64_t)delta * 8300 / 1000); /* ~8.3m per 1hPa near SL */
}

/* ── BMP390 ─────────────────────────────────────────────────── */

static uint8_t BMP390_ReadReg(I2C_HandleTypeDef *hi2c,
                               uint8_t reg, uint8_t *buf, uint8_t len) {
    if (HAL_I2C_Master_Transmit(hi2c, BMP390_ADDR, &reg, 1, 100) != HAL_OK)
        return SENSOR_ERROR;
    if (HAL_I2C_Master_Receive(hi2c, BMP390_ADDR, buf, len, 100) != HAL_OK)
        return SENSOR_ERROR;
    return SENSOR_OK;
}

static uint8_t BMP390_WriteReg(I2C_HandleTypeDef *hi2c,
                                uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return HAL_I2C_Master_Transmit(hi2c, BMP390_ADDR, buf, 2, 100);
}

uint8_t BMP390_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t chip_id = 0;
    if (BMP390_ReadReg(hi2c, BMP390_REG_CHIP_ID, &chip_id, 1) != SENSOR_OK)
        return SENSOR_ERROR;
    if (chip_id != BMP390_CHIP_ID) {
        printf("[BMP390] Bad chip ID: 0x%02X\r\n", chip_id);
        return SENSOR_ERROR;
    }

    /* Enable pressure and temperature, normal mode */
    BMP390_WriteReg(hi2c, BMP390_REG_PWR_CTRL, 0x33);
    /* OSR: pressure x8, temperature x1 */
    BMP390_WriteReg(hi2c, BMP390_REG_OSR, 0x03);
    HAL_Delay(10);
    return SENSOR_OK;
}

uint8_t BMP390_Read(I2C_HandleTypeDef *hi2c, BMP390_Data_t *data) {
    uint8_t buf[6] = {0};
    if (BMP390_ReadReg(hi2c, BMP390_REG_DATA_0, buf, 6) != SENSOR_OK)
        return SENSOR_ERROR;

    /* Raw pressure: 24-bit */
    uint32_t raw_p = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8)
                   | ((uint32_t)buf[2] << 16);
    /* Raw temperature: 24-bit */
    uint32_t raw_t = (uint32_t)buf[3] | ((uint32_t)buf[4] << 8)
                   | ((uint32_t)buf[5] << 16);

    /* TODO: apply BMP390 compensation formulas from datasheet
     * Placeholder: simple scaling for bring-up */
    data->pressure_pa    = (int32_t)(raw_p / 256);
    data->temperature_mc = (int32_t)(raw_t / 100) - 273150; /* rough K to mC */

    return SENSOR_OK;
}

/* ── SHT40 ──────────────────────────────────────────────────── */

uint8_t SHT40_Init(I2C_HandleTypeDef *hi2c) {
    /* SHT40 needs no init — just verify it responds */
    uint8_t cmd = SHT40_CMD_MEASURE_HI;
    if (HAL_I2C_Master_Transmit(hi2c, SHT40_ADDR, &cmd, 1, 100) != HAL_OK)
        return SENSOR_ERROR;
    HAL_Delay(10);
    uint8_t buf[6] = {0};
    if (HAL_I2C_Master_Receive(hi2c, SHT40_ADDR, buf, 6, 100) != HAL_OK)
        return SENSOR_ERROR;
    return SENSOR_OK;
}

uint8_t SHT40_Read(I2C_HandleTypeDef *hi2c, SHT40_Data_t *data) {
    uint8_t cmd = SHT40_CMD_MEASURE_HI;
    if (HAL_I2C_Master_Transmit(hi2c, SHT40_ADDR, &cmd, 1, 100) != HAL_OK)
        return SENSOR_ERROR;
    HAL_Delay(10); /* Measurement time ~10ms at high precision */

    uint8_t buf[6] = {0};
    if (HAL_I2C_Master_Receive(hi2c, SHT40_ADDR, buf, 6, 100) != HAL_OK)
        return SENSOR_ERROR;

    /* SHT40 data format: T_msb, T_lsb, T_crc, RH_msb, RH_lsb, RH_crc */
    /* TODO: add CRC check */
    uint16_t raw_t  = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_rh = ((uint16_t)buf[3] << 8) | buf[4];

    /* Convert per SHT40 datasheet */
    /* T [°C] = -45 + 175 * raw / 65535 */
    data->temperature_mc = (int32_t)(-45000
        + (int64_t)175000 * raw_t / 65535);
    /* RH [%] = -6 + 125 * raw / 65535, clamped 0-100 */
    int32_t rh = (int32_t)(-6000
        + (int64_t)125000 * raw_rh / 65535);
    if (rh < 0)       rh = 0;
    if (rh > 100000)  rh = 100000;
    data->humidity_mpct = rh;

    return SENSOR_OK;
}

/* ── LSM6DSO32 ──────────────────────────────────────────────── */

static uint8_t LSM6_ReadReg(I2C_HandleTypeDef *hi2c,
                              uint8_t reg, uint8_t *buf, uint8_t len) {
    if (HAL_I2C_Master_Transmit(hi2c, LSM6DSO32_ADDR, &reg, 1, 100) != HAL_OK)
        return SENSOR_ERROR;
    if (HAL_I2C_Master_Receive(hi2c, LSM6DSO32_ADDR, buf, len, 100) != HAL_OK)
        return SENSOR_ERROR;
    return SENSOR_OK;
}

static uint8_t LSM6_WriteReg(I2C_HandleTypeDef *hi2c,
                               uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return HAL_I2C_Master_Transmit(hi2c, LSM6DSO32_ADDR, buf, 2, 100);
}

uint8_t LSM6DSO32_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t who = 0;
    if (LSM6_ReadReg(hi2c, LSM6_REG_WHO_AM_I, &who, 1) != SENSOR_OK)
        return SENSOR_ERROR;
    if (who != LSM6_WHO_AM_I) {
        printf("[LSM6] Bad WHO_AM_I: 0x%02X\r\n", who);
        return SENSOR_ERROR;
    }

    /* Software reset */
    LSM6_WriteReg(hi2c, LSM6_REG_CTRL3_C, 0x01);
    HAL_Delay(10);

    /* Accel: 416Hz ODR, ±32G full scale */
    LSM6_WriteReg(hi2c, LSM6_REG_CTRL1_XL, LSM6_ODR_416Hz | LSM6_FS_XL_32G);
    /* Gyro: 416Hz ODR, ±2000dps */
    LSM6_WriteReg(hi2c, LSM6_REG_CTRL2_G, LSM6_ODR_416Hz | LSM6_FS_G_2000DPS);
    /* Enable data ready on INT1 */
    LSM6_WriteReg(hi2c, LSM6_REG_INT1_CTRL, 0x01);

    return SENSOR_OK;
}

uint8_t LSM6DSO32_SetODR(I2C_HandleTypeDef *hi2c, uint8_t odr) {
    /* Read current CTRL1_XL, preserve FS bits, update ODR */
    uint8_t reg = 0;
    if (LSM6_ReadReg(hi2c, LSM6_REG_CTRL1_XL, &reg, 1) != SENSOR_OK)
        return SENSOR_ERROR;
    reg = (reg & 0x0F) | (odr & 0xF0);
    return LSM6_WriteReg(hi2c, LSM6_REG_CTRL1_XL, reg);
}

uint8_t LSM6DSO32_Read(I2C_HandleTypeDef *hi2c, LSM6_Data_t *data) {
    uint8_t buf[12] = {0};

    /* Read gyro + accel in one burst */
    if (LSM6_ReadReg(hi2c, LSM6_REG_OUTX_L_G, buf, 12) != SENSOR_OK)
        return SENSOR_ERROR;

    int16_t gx = (int16_t)(buf[0]  | ((uint16_t)buf[1]  << 8));
    int16_t gy = (int16_t)(buf[2]  | ((uint16_t)buf[3]  << 8));
    int16_t gz = (int16_t)(buf[4]  | ((uint16_t)buf[5]  << 8));
    int16_t ax = (int16_t)(buf[6]  | ((uint16_t)buf[7]  << 8));
    int16_t ay = (int16_t)(buf[8]  | ((uint16_t)buf[9]  << 8));
    int16_t az = (int16_t)(buf[10] | ((uint16_t)buf[11] << 8));

    /* Convert: ±32G FS → sensitivity = 0.976 mg/LSB */
    data->ax_mg = (int32_t)ax * 976 / 1000;
    data->ay_mg = (int32_t)ay * 976 / 1000;
    data->az_mg = (int32_t)az * 976 / 1000;

    /* Convert: ±2000dps FS → sensitivity = 70 mdps/LSB */
    data->gx_mdps = (int32_t)gx * 70;
    data->gy_mdps = (int32_t)gy * 70;
    data->gz_mdps = (int32_t)gz * 70;

    return SENSOR_OK;
}

/* ── CAN transmit helpers ───────────────────────────────────── */

void Sensors_Send_IMU(FDCAN_HandleTypeDef *hfdcan,
                      I2C_HandleTypeDef *hi2c,
                      FlightState_t state) {
    LSM6_Data_t imu = {0};
    if (LSM6DSO32_Read(hi2c, &imu) != SENSOR_OK) return;

    typedef struct __attribute__((packed)) {
        int32_t ax_mg, ay_mg, az_mg;
        int32_t gx_mdps, gy_mdps, gz_mdps;
    } IMU_Packet_t;

    IMU_Packet_t pkt = {
        .ax_mg   = imu.ax_mg,   .ay_mg   = imu.ay_mg,   .az_mg   = imu.az_mg,
        .gx_mdps = imu.gx_mdps, .gy_mdps = imu.gy_mdps, .gz_mdps = imu.gz_mdps,
    };

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier    = CAN_ID_SENSOR_IMU_LO;
    tx.IdType        = FDCAN_STANDARD_ID;
    tx.TxFrameType   = FDCAN_DATA_FRAME;
    tx.DataLength    = FDCAN_DLC_BYTES_24;
    tx.BitRateSwitch = FDCAN_BRS_ON;
    tx.FDFormat      = FDCAN_FD_CAN;
    HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &tx, (uint8_t*)&pkt);
}

void Sensors_Send_Env(FDCAN_HandleTypeDef *hfdcan,
                      I2C_HandleTypeDef *hi2c) {
    SHT40_Data_t env = {0};
    if (SHT40_Read(hi2c, &env) != SENSOR_OK) return;

    typedef struct __attribute__((packed)) {
        int32_t humidity_mpct;
        int32_t temperature_mc;
    } Env_Packet_t;

    Env_Packet_t pkt = {
        .humidity_mpct  = env.humidity_mpct,
        .temperature_mc = env.temperature_mc,
    };

    FDCAN_TxHeaderTypeDef tx = {0};
    tx.Identifier    = CAN_ID_SENSOR_ENV;
    tx.IdType        = FDCAN_STANDARD_ID;
    tx.TxFrameType   = FDCAN_DATA_FRAME;
    tx.DataLength    = FDCAN_DLC_BYTES_8;
    tx.BitRateSwitch = FDCAN_BRS_ON;
    tx.FDFormat      = FDCAN_FD_CAN;
    HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &tx, (uint8_t*)&pkt);
}
