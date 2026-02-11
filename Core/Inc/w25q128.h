/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : w25q128.h
  * @brief          : Header for W25Q128 SPI Flash Driver
  ******************************************************************************
  * @attention
  *
  * W25Q128JVSQ 16MB SPI NOR Flash Driver
  * - Supports read, write, erase operations
  * - Sector size: 4KB
  * - Block size: 64KB  
  * - Total capacity: 16MB (128Mbit)
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __W25Q128_H
#define __W25Q128_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/* W25Q128 Commands */
#define W25Q128_CMD_WRITE_ENABLE           0x06
#define W25Q128_CMD_WRITE_DISABLE          0x04
#define W25Q128_CMD_READ_STATUS_REG1       0x05
#define W25Q128_CMD_READ_STATUS_REG2       0x35
#define W25Q128_CMD_WRITE_STATUS_REG       0x01
#define W25Q128_CMD_PAGE_PROGRAM           0x02
#define W25Q128_CMD_QUAD_PAGE_PROGRAM      0x32
#define W25Q128_CMD_BLOCK_ERASE_64KB       0xD8
#define W25Q128_CMD_BLOCK_ERASE_32KB       0x52
#define W25Q128_CMD_SECTOR_ERASE_4KB       0x20
#define W25Q128_CMD_CHIP_ERASE             0xC7
#define W25Q128_CMD_ERASE_SUSPEND          0x75
#define W25Q128_CMD_ERASE_RESUME           0x7A
#define W25Q128_CMD_POWER_DOWN             0xB9
#define W25Q128_CMD_READ_DATA              0x03
#define W25Q128_CMD_FAST_READ              0x0B
#define W25Q128_CMD_RELEASE_POWER_DOWN     0xAB
#define W25Q128_CMD_DEVICE_ID              0x90
#define W25Q128_CMD_MANUFACTURER_DEVICE_ID 0x90
#define W25Q128_CMD_JEDEC_ID               0x9F
#define W25Q128_CMD_READ_UNIQUE_ID         0x4B

/* W25Q128 Parameters */
#define W25Q128_PAGE_SIZE                  256
#define W25Q128_SECTOR_SIZE                4096
#define W25Q128_BLOCK_SIZE_32KB            (32 * 1024)
#define W25Q128_BLOCK_SIZE_64KB            (64 * 1024)
#define W25Q128_TOTAL_SIZE                 (16 * 1024 * 1024)  // 16MB

/* Status Register Bits */
#define W25Q128_STATUS_BUSY                0x01
#define W25Q128_STATUS_WEL                 0x02

/* Timeout */
#define W25Q128_TIMEOUT_MS                 1000

/* Return Status */
typedef enum {
    W25Q128_OK       = 0x00,
    W25Q128_ERROR    = 0x01,
    W25Q128_BUSY     = 0x02,
    W25Q128_TIMEOUT  = 0x03
} W25Q128_Status_t;

/* W25Q128 Handle Structure */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} W25Q128_Handle_t;

/* Function Prototypes */
void W25Q128_Init(W25Q128_Handle_t *hflash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
W25Q128_Status_t W25Q128_ReadID(W25Q128_Handle_t *hflash, uint8_t *manufacturer_id, uint8_t *device_id);
W25Q128_Status_t W25Q128_ReadJEDECID(W25Q128_Handle_t *hflash, uint8_t *jedec_id);
W25Q128_Status_t W25Q128_WriteEnable(W25Q128_Handle_t *hflash);
W25Q128_Status_t W25Q128_WriteDisable(W25Q128_Handle_t *hflash);
W25Q128_Status_t W25Q128_WaitForWriteEnd(W25Q128_Handle_t *hflash);
W25Q128_Status_t W25Q128_ReadStatusRegister(W25Q128_Handle_t *hflash, uint8_t *status);
W25Q128_Status_t W25Q128_Read(W25Q128_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length);
W25Q128_Status_t W25Q128_WritePage(W25Q128_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length);
W25Q128_Status_t W25Q128_Write(W25Q128_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length);
W25Q128_Status_t W25Q128_EraseSector(W25Q128_Handle_t *hflash, uint32_t sector_address);
W25Q128_Status_t W25Q128_EraseBlock64KB(W25Q128_Handle_t *hflash, uint32_t block_address);
W25Q128_Status_t W25Q128_EraseChip(W25Q128_Handle_t *hflash);
W25Q128_Status_t W25Q128_PowerDown(W25Q128_Handle_t *hflash);
W25Q128_Status_t W25Q128_WakeUp(W25Q128_Handle_t *hflash);

#ifdef __cplusplus
}
#endif

#endif /* __W25Q128_H */
