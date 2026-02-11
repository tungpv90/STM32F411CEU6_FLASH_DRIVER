/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : w25q64.h
  * @brief          : Header for W25Q64 SPI Flash Driver
  ******************************************************************************
  * @attention
  *
  * W25Q64 8MB SPI NOR Flash Driver
  * - Supports read, write, erase operations
  * - Sector size: 4KB
  * - Block size: 64KB  
  * - Total capacity: 8MB (64Mbit)
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __W25Q64_H
#define __W25Q64_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* W25Q64 Commands */
#define W25Q64_CMD_WRITE_ENABLE           0x06
#define W25Q64_CMD_WRITE_DISABLE          0x04
#define W25Q64_CMD_READ_STATUS_REG1       0x05
#define W25Q64_CMD_READ_STATUS_REG2       0x35
#define W25Q64_CMD_WRITE_STATUS_REG       0x01
#define W25Q64_CMD_PAGE_PROGRAM           0x02
#define W25Q64_CMD_QUAD_PAGE_PROGRAM      0x32
#define W25Q64_CMD_BLOCK_ERASE_64KB       0xD8
#define W25Q64_CMD_BLOCK_ERASE_32KB       0x52
#define W25Q64_CMD_SECTOR_ERASE_4KB       0x20
#define W25Q64_CMD_CHIP_ERASE             0xC7
#define W25Q64_CMD_ERASE_SUSPEND          0x75
#define W25Q64_CMD_ERASE_RESUME           0x7A
#define W25Q64_CMD_POWER_DOWN             0xB9
#define W25Q64_CMD_READ_DATA              0x03
#define W25Q64_CMD_FAST_READ              0x0B
#define W25Q64_CMD_RELEASE_POWER_DOWN     0xAB
#define W25Q64_CMD_DEVICE_ID              0x90
#define W25Q64_CMD_MANUFACTURER_DEVICE_ID 0x90
#define W25Q64_CMD_JEDEC_ID               0x9F
#define W25Q64_CMD_READ_UNIQUE_ID         0x4B

/* W25Q64 Parameters */
#define W25Q64_PAGE_SIZE                  256
#define W25Q64_SECTOR_SIZE                4096
#define W25Q64_BLOCK_SIZE_32KB            (32 * 1024)
#define W25Q64_BLOCK_SIZE_64KB            (64 * 1024)
#define W25Q64_TOTAL_SIZE                 (8 * 1024 * 1024)  // 8MB

/* Status Register Bits */
#define W25Q64_STATUS_BUSY                0x01
#define W25Q64_STATUS_WEL                 0x02

/* Timeout */
#define W25Q64_TIMEOUT_MS                 1000

/* Return Status */
typedef enum {
    W25Q64_OK       = 0x00,
    W25Q64_ERROR    = 0x01,
    W25Q64_BUSY     = 0x02,
    W25Q64_TIMEOUT  = 0x03
} W25Q64_Status_t;

/* W25Q64 Handle Structure */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} W25Q64_Handle_t;

/* Function Prototypes */
void W25Q64_Init(W25Q64_Handle_t *hflash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
W25Q64_Status_t W25Q64_ReadID(W25Q64_Handle_t *hflash, uint8_t *manufacturer_id, uint8_t *device_id);
W25Q64_Status_t W25Q64_ReadJEDECID(W25Q64_Handle_t *hflash, uint8_t *jedec_id);
W25Q64_Status_t W25Q64_WriteEnable(W25Q64_Handle_t *hflash);
W25Q64_Status_t W25Q64_WriteDisable(W25Q64_Handle_t *hflash);
W25Q64_Status_t W25Q64_WaitForWriteEnd(W25Q64_Handle_t *hflash);
W25Q64_Status_t W25Q64_ReadStatusRegister(W25Q64_Handle_t *hflash, uint8_t *status);
W25Q64_Status_t W25Q64_Read(W25Q64_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length);
W25Q64_Status_t W25Q64_WritePage(W25Q64_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length);
W25Q64_Status_t W25Q64_Write(W25Q64_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length);
W25Q64_Status_t W25Q64_EraseSector(W25Q64_Handle_t *hflash, uint32_t sector_address);
W25Q64_Status_t W25Q64_EraseBlock64KB(W25Q64_Handle_t *hflash, uint32_t block_address);
W25Q64_Status_t W25Q64_EraseChip(W25Q64_Handle_t *hflash);
W25Q64_Status_t W25Q64_PowerDown(W25Q64_Handle_t *hflash);
W25Q64_Status_t W25Q64_WakeUp(W25Q64_Handle_t *hflash);

#ifdef __cplusplus
}
#endif

#endif /* __W25Q64_H */
