#include "lgc_eeprom_i2c_port.h"
#include "i2c.h" // For MX_I2C1_Init and hi2c1 extern
#include "string.h" // For va_list if debug_print is implemented

// Define I2C HAL Handle (extern from i2c.c)
extern I2C_HandleTypeDef hi2c1;

// Timeout for I2C operations
#define EEPROM_I2C_TIMEOUT 100 // ms

void at24cxx_interface_iic_init(void) {
    MX_I2C1_Init(); // Re-initialize I2C1 (assuming it's not already initialized by CubeMX)
}

void at24cxx_interface_iic_deinit(void) {
    HAL_I2C_DeInit(&hi2c1);
}

// Read function for 8-bit address (e.g., AT24C01, AT24C02)
uint8_t at24cxx_interface_iic_read(uint8_t dev_address, uint16_t reg_address, uint8_t *data, uint16_t length) {
    // For AT24Cxx with 8-bit internal address, reg_address is the actual address.
    // For larger EEPROMs, this might be handled by the driver itself.
    // Assuming 8-bit internal address for now.
    if (HAL_I2C_Mem_Read(&hi2c1, dev_address, reg_address, I2C_MEMADD_SIZE_8BIT, data, length, EEPROM_I2C_TIMEOUT) == HAL_OK) {
        return 0; // Success
    }
    return 1; // Failure
}

// Write function for 8-bit address
uint8_t at24cxx_interface_iic_write(uint8_t dev_address, uint16_t reg_address, uint8_t *data, uint16_t length) {
    if (HAL_I2C_Mem_Write(&hi2c1, dev_address, reg_address, I2C_MEMADD_SIZE_8BIT, data, length, EEPROM_I2C_TIMEOUT) == HAL_OK) {
        return 0; // Success
    }
    return 1; // Failure
}

// Read function for 16-bit address (e.g., AT24C256)
uint8_t at24cxx_interface_iic_read_address16(uint8_t dev_address, uint16_t reg_address, uint8_t *data, uint16_t length) {
    if (HAL_I2C_Mem_Read(&hi2c1, dev_address, reg_address, I2C_MEMADD_SIZE_16BIT, data, length, EEPROM_I2C_TIMEOUT) == HAL_OK) {
        return 0; // Success
    }
    return 1; // Failure
}

// Write function for 16-bit address
uint8_t at24cxx_interface_iic_write_address16(uint8_t dev_address, uint16_t reg_address, uint8_t *data, uint16_t length) {
    if (HAL_I2C_Mem_Write(&hi2c1, dev_address, reg_address, I2C_MEMADD_SIZE_16BIT, data, length, EEPROM_I2C_TIMEOUT) == HAL_OK) {
        return 0; // Success
    }
    return 1; // Failure
}

void at24cxx_interface_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

// Optional: Implement debug print if needed. For now, a stub.
void at24cxx_interface_debug_print(const char *format, ...) {
    (void)format; // Suppress unused parameter warning
    // va_list args;
    // va_start(args, format);
    // vprintf(format, args); // Requires stdio.h
    // va_end(args);
}
