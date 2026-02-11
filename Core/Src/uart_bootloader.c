/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : uart_bootloader.c
  * @brief          : UART Bootloader Protocol Implementation
  ******************************************************************************
  */
/* USER CODE END Header */

#include "uart_bootloader.h"
#include <string.h>

/**
  * @brief  Calculate CRC16 (CCITT)
  * @param  data: Pointer to data buffer
  * @param  length: Length of data
  * @retval CRC16 value
  */
uint16_t BOOT_CalculateCRC16(uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFF;
    
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    
    return crc;
}

/**
  * @brief  Initialize bootloader
  * @param  hboot: Pointer to bootloader handle
  * @param  huart: Pointer to UART handle
  * @param  hflash: Pointer to W25Q128 handle
  * @retval None
  */
void BOOT_Init(BOOT_Handle_t *hboot, UART_HandleTypeDef *huart, W25Q128_Handle_t *hflash)
{
    hboot->huart = huart;
    hboot->hflash = hflash;
    hboot->total_bytes_written = 0;
    hboot->total_bytes_read = 0;
    memset(hboot->rx_buffer, 0, BOOT_BUFFER_SIZE);
}

/**
  * @brief  Send response via UART
  * @param  hboot: Pointer to bootloader handle
  * @param  response: Response byte (ACK or NACK)
  * @retval BOOT_Status_t
  */
BOOT_Status_t BOOT_SendResponse(BOOT_Handle_t *hboot, uint8_t response)
{
    if (HAL_UART_Transmit(hboot->huart, &response, 1, BOOT_TIMEOUT_MS) != HAL_OK)
    {
        return BOOT_ERROR;
    }
    return BOOT_OK;
}

/**
  * @brief  Send data via UART
  * @param  hboot: Pointer to bootloader handle
  * @param  data: Pointer to data buffer
  * @param  length: Length of data
  * @retval BOOT_Status_t
  */
BOOT_Status_t BOOT_SendData(BOOT_Handle_t *hboot, uint8_t *data, uint32_t length)
{
    if (HAL_UART_Transmit(hboot->huart, data, length, BOOT_TIMEOUT_MS) != HAL_OK)
    {
        return BOOT_ERROR;
    }
    return BOOT_OK;
}

/**
  * @brief  Receive data via UART with timeout
  * @param  hboot: Pointer to bootloader handle
  * @param  buffer: Pointer to receive buffer
  * @param  length: Number of bytes to receive
  * @retval BOOT_Status_t
  */
static BOOT_Status_t BOOT_ReceiveData(BOOT_Handle_t *hboot, uint8_t *buffer, uint32_t length)
{
    if (HAL_UART_Receive(hboot->huart, buffer, length, BOOT_TIMEOUT_MS) != HAL_OK)
    {
        return BOOT_TIMEOUT;
    }
    return BOOT_OK;
}

/**
  * @brief  Handle write command
  * @param  hboot: Pointer to bootloader handle
  * @retval BOOT_Status_t
  */
static BOOT_Status_t BOOT_HandleWrite(BOOT_Handle_t *hboot)
{
    uint8_t buffer[8];
    uint32_t data_length;
    uint32_t address;
    uint16_t crc_received, crc_calculated;
    uint8_t *data_buffer;
    
    // Receive data length (4 bytes)
    if (BOOT_ReceiveData(hboot, buffer, 4) != BOOT_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_TIMEOUT;
    }
    data_length = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | 
                  ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
    
    // Check if data length is valid
    if (data_length == 0 || data_length > BOOT_MAX_DATA_SIZE)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_ERROR;
    }
    
    // Receive address (4 bytes)
    if (BOOT_ReceiveData(hboot, buffer, 4) != BOOT_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_TIMEOUT;
    }
    address = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | 
              ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
    
    // Allocate buffer for data (use static buffer to avoid stack overflow)
    static uint8_t large_buffer[BOOT_MAX_DATA_SIZE];
    data_buffer = large_buffer;
    
    // Receive data
    uint32_t remaining = data_length;
    uint32_t offset = 0;
    while (remaining > 0)
    {
        uint32_t chunk_size = (remaining > BOOT_BUFFER_SIZE) ? BOOT_BUFFER_SIZE : remaining;
        if (BOOT_ReceiveData(hboot, &data_buffer[offset], chunk_size) != BOOT_OK)
        {
            BOOT_SendResponse(hboot, BOOT_NACK);
            return BOOT_TIMEOUT;
        }
        offset += chunk_size;
        remaining -= chunk_size;
    }
    
    // Receive CRC (2 bytes)
    if (BOOT_ReceiveData(hboot, buffer, 2) != BOOT_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_TIMEOUT;
    }
    crc_received = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
    
    // Calculate CRC
    crc_calculated = BOOT_CalculateCRC16(data_buffer, data_length);
    
    // Verify CRC
    if (crc_received != crc_calculated)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_CRC_ERR;
    }
    
    // Write data to flash
    if (W25Q128_Write(hboot->hflash, address, data_buffer, data_length) != W25Q128_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_ERROR;
    }
    
    hboot->total_bytes_written += data_length;
    
    // Send ACK
    BOOT_SendResponse(hboot, BOOT_ACK);
    
    return BOOT_OK;
}

/**
  * @brief  Handle read command
  * @param  hboot: Pointer to bootloader handle
  * @retval BOOT_Status_t
  */
static BOOT_Status_t BOOT_HandleRead(BOOT_Handle_t *hboot)
{
    uint8_t buffer[8];
    uint32_t data_length;
    uint32_t address;
    uint16_t crc_calculated;
    uint8_t *data_buffer;
    
    // Receive data length (4 bytes)
    if (BOOT_ReceiveData(hboot, buffer, 4) != BOOT_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_TIMEOUT;
    }
    data_length = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | 
                  ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
    
    // Check if data length is valid
    if (data_length == 0 || data_length > BOOT_MAX_DATA_SIZE)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_ERROR;
    }
    
    // Receive address (4 bytes)
    if (BOOT_ReceiveData(hboot, buffer, 4) != BOOT_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_TIMEOUT;
    }
    address = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | 
              ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
    
    // Allocate buffer for data
    static uint8_t large_buffer[BOOT_MAX_DATA_SIZE];
    data_buffer = large_buffer;
    
    // Read data from flash
    if (W25Q128_Read(hboot->hflash, address, data_buffer, data_length) != W25Q128_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_ERROR;
    }
    
    // Send ACK
    BOOT_SendResponse(hboot, BOOT_ACK);
    
    // Send data
    if (BOOT_SendData(hboot, data_buffer, data_length) != BOOT_OK)
    {
        return BOOT_ERROR;
    }
    
    // Calculate and send CRC
    crc_calculated = BOOT_CalculateCRC16(data_buffer, data_length);
    buffer[0] = crc_calculated & 0xFF;
    buffer[1] = (crc_calculated >> 8) & 0xFF;
    BOOT_SendData(hboot, buffer, 2);
    
    hboot->total_bytes_read += data_length;
    
    return BOOT_OK;
}

/**
  * @brief  Handle erase sector command
  * @param  hboot: Pointer to bootloader handle
  * @retval BOOT_Status_t
  */
static BOOT_Status_t BOOT_HandleEraseSector(BOOT_Handle_t *hboot)
{
    uint8_t buffer[4];
    uint32_t address;
    
    // Receive address (4 bytes)
    if (BOOT_ReceiveData(hboot, buffer, 4) != BOOT_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_TIMEOUT;
    }
    address = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | 
              ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
    
    // Erase sector
    if (W25Q128_EraseSector(hboot->hflash, address) != W25Q128_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_ERROR;
    }
    
    // Send ACK
    BOOT_SendResponse(hboot, BOOT_ACK);
    
    return BOOT_OK;
}

/**
  * @brief  Handle erase chip command
  * @param  hboot: Pointer to bootloader handle
  * @retval BOOT_Status_t
  */
static BOOT_Status_t BOOT_HandleEraseChip(BOOT_Handle_t *hboot)
{
    // Erase entire chip (this takes a long time!)
    if (W25Q128_EraseChip(hboot->hflash) != W25Q128_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_ERROR;
    }
    
    // Send ACK
    BOOT_SendResponse(hboot, BOOT_ACK);
    
    return BOOT_OK;
}

/**
  * @brief  Handle get info command
  * @param  hboot: Pointer to bootloader handle
  * @retval BOOT_Status_t
  */
static BOOT_Status_t BOOT_HandleGetInfo(BOOT_Handle_t *hboot)
{
    uint8_t info[16];
    uint8_t manufacturer_id, device_id;
    uint8_t jedec_id[3];
    
    // Read flash ID
    if (W25Q128_ReadID(hboot->hflash, &manufacturer_id, &device_id) != W25Q128_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_ERROR;
    }
    
    // Read JEDEC ID
    if (W25Q128_ReadJEDECID(hboot->hflash, jedec_id) != W25Q128_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return BOOT_ERROR;
    }
    
    // Prepare info packet
    info[0] = manufacturer_id;
    info[1] = device_id;
    info[2] = jedec_id[0];
    info[3] = jedec_id[1];
    info[4] = jedec_id[2];
    
    // Flash capacity (16MB = 16777216 bytes)
    uint32_t capacity = W25Q128_TOTAL_SIZE;
    info[5] = capacity & 0xFF;
    info[6] = (capacity >> 8) & 0xFF;
    info[7] = (capacity >> 16) & 0xFF;
    info[8] = (capacity >> 24) & 0xFF;
    
    // Page size
    uint16_t page_size = W25Q128_PAGE_SIZE;
    info[9] = page_size & 0xFF;
    info[10] = (page_size >> 8) & 0xFF;
    
    // Sector size
    uint16_t sector_size = W25Q128_SECTOR_SIZE;
    info[11] = sector_size & 0xFF;
    info[12] = (sector_size >> 8) & 0xFF;
    
    // Send ACK
    BOOT_SendResponse(hboot, BOOT_ACK);
    
    // Send info
    BOOT_SendData(hboot, info, 13);
    
    return BOOT_OK;
}

/**
  * @brief  Process bootloader protocol
  * @param  hboot: Pointer to bootloader handle
  * @retval None
  */
void BOOT_Process(BOOT_Handle_t *hboot)
{
    uint8_t header[3];
    uint8_t command;
    
    // Wait for start markers
    if (HAL_UART_Receive(hboot->huart, header, 2, HAL_MAX_DELAY) != HAL_OK)
    {
        return;
    }
    
    // Check start markers
    if (header[0] != BOOT_START_MARKER1 || header[1] != BOOT_START_MARKER2)
    {
        return;
    }
    
    // Receive command
    if (BOOT_ReceiveData(hboot, &command, 1) != BOOT_OK)
    {
        BOOT_SendResponse(hboot, BOOT_NACK);
        return;
    }
    
    // Process command
    switch (command)
    {
        case BOOT_CMD_WRITE:
            BOOT_HandleWrite(hboot);
            break;
            
        case BOOT_CMD_READ:
            BOOT_HandleRead(hboot);
            break;
            
        case BOOT_CMD_ERASE_SECTOR:
            BOOT_HandleEraseSector(hboot);
            break;
            
        case BOOT_CMD_ERASE_CHIP:
            BOOT_HandleEraseChip(hboot);
            break;
            
        case BOOT_CMD_GET_INFO:
            BOOT_HandleGetInfo(hboot);
            break;
            
        default:
            BOOT_SendResponse(hboot, BOOT_NACK);
            break;
    }
}
