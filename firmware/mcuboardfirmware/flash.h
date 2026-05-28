/* flash.h — W25Q128 NOR flash driver
 * Simple record-based logging interface
 * Uses raw sector addressing — no filesystem for now
 * TODO: replace with LittleFS once hardware is verified
 */

#ifndef __FLASH_H
#define __FLASH_H

#include "main.h"
#include <stdint.h>

/* W25Q128 commands */
#define W25Q_CMD_WRITE_ENABLE   0x06
#define W25Q_CMD_WRITE_DISABLE  0x04
#define W25Q_CMD_READ_STATUS1   0x05
#define W25Q_CMD_READ_STATUS2   0x35
#define W25Q_CMD_PAGE_PROGRAM   0x02
#define W25Q_CMD_SECTOR_ERASE   0x20  /* 4KB sector erase */
#define W25Q_CMD_CHIP_ERASE     0xC7
#define W25Q_CMD_READ_DATA      0x03
#define W25Q_CMD_FAST_READ      0x0B
#define W25Q_CMD_JEDEC_ID       0x9F
#define W25Q_CMD_POWER_DOWN     0xB9
#define W25Q_CMD_RELEASE_PD     0xAB

/* W25Q128 geometry */
#define W25Q_PAGE_SIZE          256
#define W25Q_SECTOR_SIZE        4096
#define W25Q_TOTAL_SIZE         (16 * 1024 * 1024)  /* 16MB */
#define W25Q_JEDEC_ID           0xEF4018

/* Status register bits */
#define W25Q_SR1_BUSY           0x01
#define W25Q_SR1_WEL            0x02

/* Return codes */
#define FLASH_OK                0
#define FLASH_ERROR             1
#define FLASH_TIMEOUT           2
#define FLASH_FULL              3

/* Log record header — prepended to every record */
typedef struct __attribute__((packed)) {
    uint32_t timestamp_ms;
    uint16_t can_id;
    uint8_t  data_len;
    uint8_t  reserved;
} Flash_RecordHeader_t;

/* Public API */
uint8_t  Flash_Init(void);
uint32_t Flash_ReadJEDECID(void);
uint8_t  Flash_ReadStatus(void);
void     Flash_WaitBusy(void);
uint8_t  Flash_WriteRecord(uint16_t can_id, uint8_t *data, uint8_t len);
uint8_t  Flash_ReadPage(uint32_t addr, uint8_t *buf, uint16_t len);
uint8_t  Flash_EraseSector(uint32_t addr);
uint8_t  Flash_EraseChip(void);
uint32_t Flash_GetWriteAddress(void);
void     Flash_FlushBuffer(void);

#endif /* __FLASH_H */
