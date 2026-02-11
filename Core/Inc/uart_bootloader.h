/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : uart_bootloader.h
  * @brief          : UART Bootloader Protocol Header
  ******************************************************************************
  * @attention
  *
  * UART Bootloader Protocol for flashing firmware to W25Q128
  * 
  * Protocol Format:
  * 1. PC sends: START_MARKER (0xAA, 0x55)
  * 2. PC sends: COMMAND (1 byte)
  * 3. PC sends: DATA_LENGTH (4 bytes, little endian)
  * 4. PC sends: ADDRESS (4 bytes, little endian) - for write commands
  * 5. PC sends: DATA (variable length)
  * 6. PC sends: CHECKSUM (2 bytes, CRC16)
  * 7. STM32 responds: ACK (0x79) or NACK (0x1F)
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __UART_BOOTLOADER_H
#define __UART_BOOTLOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "w25q128.h"

/* Protocol markers and commands */
#define BOOT_START_MARKER1        0xAA
#define BOOT_START_MARKER2        0x55
#define BOOT_ACK                  0x79
#define BOOT_NACK                 0x1F

/* Commands */
#define BOOT_CMD_WRITE            0x01  // Write data to flash
#define BOOT_CMD_READ             0x02  // Read data from flash
#define BOOT_CMD_ERASE_SECTOR     0x03  // Erase 4KB sector
#define BOOT_CMD_ERASE_CHIP       0x04  // Erase entire chip
#define BOOT_CMD_GET_INFO         0x05  // Get flash info
#define BOOT_CMD_VERIFY           0x06  // Verify written data

/* Configuration */
#define BOOT_MAX_DATA_SIZE        4096  // Maximum data size per packet
#define BOOT_TIMEOUT_MS           5000  // 5 seconds timeout
#define BOOT_BUFFER_SIZE          256   // UART buffer size

/* Status codes */
typedef enum {
    BOOT_OK       = 0x00,
    BOOT_ERROR    = 0x01,
    BOOT_TIMEOUT  = 0x02,
    BOOT_CRC_ERR  = 0x03
} BOOT_Status_t;

/* Bootloader handle */
typedef struct {
    UART_HandleTypeDef *huart;
    W25Q128_Handle_t *hflash;
    uint8_t rx_buffer[BOOT_BUFFER_SIZE];
    uint32_t total_bytes_written;
    uint32_t total_bytes_read;
} BOOT_Handle_t;

/* Function prototypes */
void BOOT_Init(BOOT_Handle_t *hboot, UART_HandleTypeDef *huart, W25Q128_Handle_t *hflash);
void BOOT_Process(BOOT_Handle_t *hboot);
BOOT_Status_t BOOT_SendResponse(BOOT_Handle_t *hboot, uint8_t response);
BOOT_Status_t BOOT_SendData(BOOT_Handle_t *hboot, uint8_t *data, uint32_t length);
uint16_t BOOT_CalculateCRC16(uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* __UART_BOOTLOADER_H */
