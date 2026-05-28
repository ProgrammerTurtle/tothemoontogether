/* flash.c — W25Q128 NOR flash driver implementation
 * Simple raw record logging — no filesystem
 * Records written sequentially from address 0
 * Write pointer stored in first sector header
 * TODO: replace with LittleFS once hardware verified
 */

#include "flash.h"
#include <string.h>
#include <stdio.h>

/* Current write address — advances with each record */
static uint32_t write_addr = 0;

/* Small RAM buffer to coalesce small writes into full pages */
static uint8_t  page_buf[W25Q_PAGE_SIZE];
static uint16_t page_buf_idx = 0;
static uint32_t page_buf_addr = 0;

/* ── Low level SPI helpers ─────────────────────────────────── */

static void Flash_CS_Low(void) {
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
}

static void Flash_CS_High(void) {
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

static uint8_t Flash_SPI_TxRx(uint8_t data) {
    uint8_t rx = 0;
    HAL_SPI_TransmitReceive(&hspi1, &data, &rx, 1, HAL_MAX_DELAY);
    return rx;
}

static void Flash_SPI_Tx(uint8_t *data, uint16_t len) {
    HAL_SPI_Transmit(&hspi1, data, len, HAL_MAX_DELAY);
}

static void Flash_SPI_Rx(uint8_t *data, uint16_t len) {
    HAL_SPI_Receive(&hspi1, data, len, HAL_MAX_DELAY);
}

static void Flash_SendAddr(uint32_t addr) {
    Flash_SPI_TxRx((addr >> 16) & 0xFF);
    Flash_SPI_TxRx((addr >> 8)  & 0xFF);
    Flash_SPI_TxRx((addr)       & 0xFF);
}

/* ── Status and control ────────────────────────────────────── */

uint8_t Flash_ReadStatus(void) {
    Flash_CS_Low();
    Flash_SPI_TxRx(W25Q_CMD_READ_STATUS1);
    uint8_t status = Flash_SPI_TxRx(0xFF);
    Flash_CS_High();
    return status;
}

void Flash_WaitBusy(void) {
    uint32_t timeout = HAL_GetTick() + 5000; /* 5s timeout */
    while (Flash_ReadStatus() & W25Q_SR1_BUSY) {
        if (HAL_GetTick() > timeout) {
            printf("[FLASH] Timeout waiting for busy\r\n");
            return;
        }
    }
}

static void Flash_WriteEnable(void) {
    Flash_CS_Low();
    Flash_SPI_TxRx(W25Q_CMD_WRITE_ENABLE);
    Flash_CS_High();
}

/* ── Public API ────────────────────────────────────────────── */

uint8_t Flash_Init(void) {
    /* Release from power down just in case */
    Flash_CS_Low();
    Flash_SPI_TxRx(W25Q_CMD_RELEASE_PD);
    Flash_CS_High();
    HAL_Delay(1);

    /* Verify JEDEC ID */
    uint32_t id = Flash_ReadJEDECID();
    if (id != W25Q_JEDEC_ID) {
        printf("[FLASH] Bad JEDEC ID: 0x%06lX (expected 0x%06lX)\r\n",
               id, (uint32_t)W25Q_JEDEC_ID);
        return FLASH_ERROR;
    }

    /* Find write pointer — scan for first 0xFF byte pair (empty flash)
     * Simple approach: start at address 0, advance past existing records
     * TODO: replace with LittleFS superblock lookup */
    write_addr = 0;
    page_buf_idx = 0;
    page_buf_addr = 0;
    memset(page_buf, 0xFF, W25Q_PAGE_SIZE);

    /* TODO: scan existing records to find write pointer on restart */
    /* For now always start at 0 — data from previous flights will be
     * overwritten. Add flight counter and offset when LittleFS is added. */

    return FLASH_OK;
}

uint32_t Flash_ReadJEDECID(void) {
    Flash_CS_Low();
    Flash_SPI_TxRx(W25Q_CMD_JEDEC_ID);
    uint32_t id = 0;
    id |= ((uint32_t)Flash_SPI_TxRx(0xFF) << 16);
    id |= ((uint32_t)Flash_SPI_TxRx(0xFF) << 8);
    id |= ((uint32_t)Flash_SPI_TxRx(0xFF));
    Flash_CS_High();
    return id;
}

uint8_t Flash_ReadPage(uint32_t addr, uint8_t *buf, uint16_t len) {
    Flash_CS_Low();
    Flash_SPI_TxRx(W25Q_CMD_FAST_READ);
    Flash_SendAddr(addr);
    Flash_SPI_TxRx(0xFF); /* dummy byte for fast read */
    Flash_SPI_Rx(buf, len);
    Flash_CS_High();
    return FLASH_OK;
}

uint8_t Flash_EraseSector(uint32_t addr) {
    /* Align to sector boundary */
    addr &= ~(W25Q_SECTOR_SIZE - 1);
    Flash_WriteEnable();
    Flash_CS_Low();
    Flash_SPI_TxRx(W25Q_CMD_SECTOR_ERASE);
    Flash_SendAddr(addr);
    Flash_CS_High();
    Flash_WaitBusy();
    return FLASH_OK;
}

uint8_t Flash_EraseChip(void) {
    printf("[FLASH] Chip erase — this takes ~40 seconds\r\n");
    Flash_WriteEnable();
    Flash_CS_Low();
    Flash_SPI_TxRx(W25Q_CMD_CHIP_ERASE);
    Flash_CS_High();
    Flash_WaitBusy();
    write_addr = 0;
    page_buf_idx = 0;
    memset(page_buf, 0xFF, W25Q_PAGE_SIZE);
    printf("[FLASH] Chip erase complete\r\n");
    return FLASH_OK;
}

static uint8_t Flash_WritePage(uint32_t addr, uint8_t *data, uint16_t len) {
    if (len == 0 || len > W25Q_PAGE_SIZE) return FLASH_ERROR;

    /* Erase sector if we're at a sector boundary */
    if ((addr % W25Q_SECTOR_SIZE) == 0) {
        Flash_EraseSector(addr);
    }

    Flash_WriteEnable();
    Flash_CS_Low();
    Flash_SPI_TxRx(W25Q_CMD_PAGE_PROGRAM);
    Flash_SendAddr(addr);
    Flash_SPI_Tx(data, len);
    Flash_CS_High();
    Flash_WaitBusy();
    return FLASH_OK;
}

uint8_t Flash_WriteRecord(uint16_t can_id, uint8_t *data, uint8_t len) {
    if (write_addr >= W25Q_TOTAL_SIZE) {
        printf("[FLASH] Full!\r\n");
        return FLASH_FULL;
    }

    Flash_RecordHeader_t hdr = {
        .timestamp_ms = HAL_GetTick(),
        .can_id       = can_id,
        .data_len     = len,
        .reserved     = 0
    };

    uint8_t total_len = sizeof(Flash_RecordHeader_t) + len;
    uint8_t record[sizeof(Flash_RecordHeader_t) + 64]; /* max 64 byte payload */
    if (len > 64) return FLASH_ERROR;

    memcpy(record, &hdr, sizeof(Flash_RecordHeader_t));
    memcpy(record + sizeof(Flash_RecordHeader_t), data, len);

    /* Buffer records into pages */
    for (uint8_t i = 0; i < total_len; i++) {
        if (page_buf_idx == 0) {
            page_buf_addr = write_addr;
        }
        page_buf[page_buf_idx++] = record[i];
        write_addr++;

        /* Flush when page buffer full or at page boundary */
        if (page_buf_idx >= W25Q_PAGE_SIZE) {
            Flash_WritePage(page_buf_addr, page_buf, W25Q_PAGE_SIZE);
            page_buf_idx = 0;
            memset(page_buf, 0xFF, W25Q_PAGE_SIZE);
        }
    }

    return FLASH_OK;
}

void Flash_FlushBuffer(void) {
    if (page_buf_idx > 0) {
        Flash_WritePage(page_buf_addr, page_buf, page_buf_idx);
        page_buf_idx = 0;
        memset(page_buf, 0xFF, W25Q_PAGE_SIZE);
        printf("[FLASH] Buffer flushed at 0x%06lX\r\n", write_addr);
    }
}

uint32_t Flash_GetWriteAddress(void) {
    return write_addr;
}
