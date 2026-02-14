#ifndef LGC_EEPROM_I2C_PORT_H
#define LGC_EEPROM_I2C_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "driver_at24cxx.h" // For at24cxx_handle_t definitions

// External declaration for I2C HAL Handle, likely defined in i2c.c
extern I2C_HandleTypeDef hi2c1;

// Function prototypes for AT24Cxx driver interface
void at24cxx_interface_iic_init(void);
void at24cxx_interface_iic_deinit(void);
uint8_t at24cxx_interface_iic_read(uint8_t dev_address, uint16_t reg_address, uint8_t *data, uint16_t length);
uint8_t at24cxx_interface_iic_write(uint8_t dev_address, uint16_t reg_address, uint8_t *data, uint16_t length);
uint8_t at24cxx_interface_iic_read_address16(uint8_t dev_address, uint16_t reg_address, uint8_t *data, uint16_t length);
uint8_t at24cxx_interface_iic_write_address16(uint8_t dev_address, uint16_t reg_address, uint8_t *data, uint16_t length);
void at24cxx_interface_delay_ms(uint32_t ms);
void at24cxx_interface_debug_print(const char *format, ...); // If debug print needed

#ifdef __cplusplus
}
#endif

#endif // LGC_EEPROM_I2C_PORT_H
