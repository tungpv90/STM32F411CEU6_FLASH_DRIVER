/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : w25q128.c
  * @brief          : W25Q128 SPI Flash Driver Implementation
  ******************************************************************************
  */
/* USER CODE END Header */

#include "w25q128.h"

/* Private helper macros */
#define CS_LOW()   HAL_GPIO_WritePin(hflash->cs_port, hflash->cs_pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(hflash->cs_port, hflash->cs_pin, GPIO_PIN_SET)

/**
  * @brief  Initialize W25Q128 Flash
  * @param  hflash: Pointer to W25Q128 handle
  * @param  hspi: Pointer to SPI handle
  * @param  cs_port: Chip select GPIO port
  * @param  cs_pin: Chip select GPIO pin
  * @retval None
  */
void W25Q128_Init(W25Q128_Handle_t *hflash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    hflash->hspi = hspi;
    hflash->cs_port = cs_port;
    hflash->cs_pin = cs_pin;
    
    CS_HIGH();
    HAL_Delay(100);
    
    // Wake up from power down mode if needed
    W25Q128_WakeUp(hflash);
}

/**
  * @brief  Read Manufacturer and Device ID
  * @param  hflash: Pointer to W25Q128 handle
  * @param  manufacturer_id: Pointer to store manufacturer ID (should be 0xEF for Winbond)
  * @param  device_id: Pointer to store device ID (should be 0x17 for W25Q128)
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_ReadID(W25Q128_Handle_t *hflash, uint8_t *manufacturer_id, uint8_t *device_id)
{
    uint8_t cmd[4] = {W25Q128_CMD_MANUFACTURER_DEVICE_ID, 0x00, 0x00, 0x00};
    uint8_t data[2];
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, cmd, 4, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    if (HAL_SPI_Receive(hflash->hspi, data, 2, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    *manufacturer_id = data[0];
    *device_id = data[1];
    
    return W25Q128_OK;
}

/**
  * @brief  Read JEDEC ID (Manufacturer, Memory Type, Capacity)
  * @param  hflash: Pointer to W25Q128 handle
  * @param  jedec_id: Pointer to 3-byte buffer to store JEDEC ID
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_ReadJEDECID(W25Q128_Handle_t *hflash, uint8_t *jedec_id)
{
    uint8_t cmd = W25Q128_CMD_JEDEC_ID;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, &cmd, 1, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    if (HAL_SPI_Receive(hflash->hspi, jedec_id, 3, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  Read Status Register
  * @param  hflash: Pointer to W25Q128 handle
  * @param  status: Pointer to store status register value
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_ReadStatusRegister(W25Q128_Handle_t *hflash, uint8_t *status)
{
    uint8_t cmd = W25Q128_CMD_READ_STATUS_REG1;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, &cmd, 1, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    if (HAL_SPI_Receive(hflash->hspi, status, 1, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  Wait for write operation to complete
  * @param  hflash: Pointer to W25Q128 handle
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_WaitForWriteEnd(W25Q128_Handle_t *hflash)
{
    uint8_t status = 0;
    uint32_t timeout = HAL_GetTick() + 5000;  // 5 second timeout for erase operations
    
    do
    {
        if (W25Q128_ReadStatusRegister(hflash, &status) != W25Q128_OK)
        {
            return W25Q128_ERROR;
        }
        
        if (HAL_GetTick() > timeout)
        {
            return W25Q128_TIMEOUT;
        }
        
    } while (status & W25Q128_STATUS_BUSY);
    
    return W25Q128_OK;
}

/**
  * @brief  Enable write operations
  * @param  hflash: Pointer to W25Q128 handle
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_WriteEnable(W25Q128_Handle_t *hflash)
{
    uint8_t cmd = W25Q128_CMD_WRITE_ENABLE;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, &cmd, 1, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  Disable write operations
  * @param  hflash: Pointer to W25Q128 handle
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_WriteDisable(W25Q128_Handle_t *hflash)
{
    uint8_t cmd = W25Q128_CMD_WRITE_DISABLE;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, &cmd, 1, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  Read data from flash
  * @param  hflash: Pointer to W25Q128 handle
  * @param  address: Start address to read from (0 to 16MB-1)
  * @param  buffer: Pointer to data buffer
  * @param  length: Number of bytes to read
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_Read(W25Q128_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length)
{
    uint8_t cmd[4];
    
    cmd[0] = W25Q128_CMD_READ_DATA;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, cmd, 4, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    if (HAL_SPI_Receive(hflash->hspi, buffer, length, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  Write a page (up to 256 bytes)
  * @param  hflash: Pointer to W25Q128 handle
  * @param  address: Start address to write to
  * @param  buffer: Pointer to data buffer
  * @param  length: Number of bytes to write (max 256)
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_WritePage(W25Q128_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length)
{
    uint8_t cmd[4];
    
    if (length > W25Q128_PAGE_SIZE)
    {
        return W25Q128_ERROR;
    }
    
    // Enable write
    if (W25Q128_WriteEnable(hflash) != W25Q128_OK)
    {
        return W25Q128_ERROR;
    }
    
    cmd[0] = W25Q128_CMD_PAGE_PROGRAM;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, cmd, 4, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    if (HAL_SPI_Transmit(hflash->hspi, buffer, length, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    // Wait for write to complete
    return W25Q128_WaitForWriteEnd(hflash);
}

/**
  * @brief  Write data to flash (handles multiple pages automatically)
  * @param  hflash: Pointer to W25Q128 handle
  * @param  address: Start address to write to
  * @param  buffer: Pointer to data buffer
  * @param  length: Number of bytes to write
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_Write(W25Q128_Handle_t *hflash, uint32_t address, uint8_t *buffer, uint32_t length)
{
    uint32_t remaining = length;
    uint32_t page_offset;
    uint32_t write_length;
    uint32_t current_address = address;
    uint8_t *current_buffer = buffer;
    
    while (remaining > 0)
    {
        page_offset = current_address % W25Q128_PAGE_SIZE;
        write_length = W25Q128_PAGE_SIZE - page_offset;
        
        if (write_length > remaining)
        {
            write_length = remaining;
        }
        
        if (W25Q128_WritePage(hflash, current_address, current_buffer, write_length) != W25Q128_OK)
        {
            return W25Q128_ERROR;
        }
        
        current_address += write_length;
        current_buffer += write_length;
        remaining -= write_length;
    }
    
    return W25Q128_OK;
}

/**
  * @brief  Erase a 4KB sector
  * @param  hflash: Pointer to W25Q128 handle
  * @param  sector_address: Address within the sector to erase
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_EraseSector(W25Q128_Handle_t *hflash, uint32_t sector_address)
{
    uint8_t cmd[4];
    
    // Enable write
    if (W25Q128_WriteEnable(hflash) != W25Q128_OK)
    {
        return W25Q128_ERROR;
    }
    
    cmd[0] = W25Q128_CMD_SECTOR_ERASE_4KB;
    cmd[1] = (sector_address >> 16) & 0xFF;
    cmd[2] = (sector_address >> 8) & 0xFF;
    cmd[3] = sector_address & 0xFF;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, cmd, 4, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    // Wait for erase to complete
    return W25Q128_WaitForWriteEnd(hflash);
}

/**
  * @brief  Erase a 64KB block
  * @param  hflash: Pointer to W25Q128 handle
  * @param  block_address: Address within the block to erase
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_EraseBlock64KB(W25Q128_Handle_t *hflash, uint32_t block_address)
{
    uint8_t cmd[4];
    
    // Enable write
    if (W25Q128_WriteEnable(hflash) != W25Q128_OK)
    {
        return W25Q128_ERROR;
    }
    
    cmd[0] = W25Q128_CMD_BLOCK_ERASE_64KB;
    cmd[1] = (block_address >> 16) & 0xFF;
    cmd[2] = (block_address >> 8) & 0xFF;
    cmd[3] = block_address & 0xFF;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, cmd, 4, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    // Wait for erase to complete
    return W25Q128_WaitForWriteEnd(hflash);
}

/**
  * @brief  Erase entire chip
  * @param  hflash: Pointer to W25Q128 handle
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_EraseChip(W25Q128_Handle_t *hflash)
{
    uint8_t cmd = W25Q128_CMD_CHIP_ERASE;
    
    // Enable write
    if (W25Q128_WriteEnable(hflash) != W25Q128_OK)
    {
        return W25Q128_ERROR;
    }
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, &cmd, 1, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    // Wait for erase to complete (this can take a long time)
    return W25Q128_WaitForWriteEnd(hflash);
}

/**
  * @brief  Enter power down mode
  * @param  hflash: Pointer to W25Q128 handle
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_PowerDown(W25Q128_Handle_t *hflash)
{
    uint8_t cmd = W25Q128_CMD_POWER_DOWN;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, &cmd, 1, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    return W25Q128_OK;
}

/**
  * @brief  Wake up from power down mode
  * @param  hflash: Pointer to W25Q128 handle
  * @retval W25Q128_Status_t
  */
W25Q128_Status_t W25Q128_WakeUp(W25Q128_Handle_t *hflash)
{
    uint8_t cmd = W25Q128_CMD_RELEASE_POWER_DOWN;
    
    CS_LOW();
    
    if (HAL_SPI_Transmit(hflash->hspi, &cmd, 1, W25Q128_TIMEOUT_MS) != HAL_OK)
    {
        CS_HIGH();
        return W25Q128_ERROR;
    }
    
    CS_HIGH();
    
    HAL_Delay(1);  // Wait for device to wake up
    
    return W25Q128_OK;
}
