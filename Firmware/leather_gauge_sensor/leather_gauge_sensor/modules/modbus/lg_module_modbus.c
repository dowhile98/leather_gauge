/**
 *
 */
/* ============================================================================
 * includes
 * ========================================================================= */
#include "lg_module_modbus.h"
#include "lwrb.h"
#include "usart.h"
#include "gpio.h"
#include "lg_module_eeprom.h"
#include "lg_module_sensor.h"
/* ============================================================================
 * defines
 * ========================================================================= */
#ifndef LG_UART_RX_BUFFER_SIZE
#define LG_UART_RX_BUFFER_SIZE 64
#endif

#define COILS_ADDR_MAX LG_ADC_SENAOR_MAX_SIZE


/* ============================================================================
 * typedefs
 * ========================================================================= */

/* ============================================================================
 * global variables
 * ========================================================================= */
static uint8_t rx_buffer[LG_UART_RX_BUFFER_SIZE];
static lwrb_t rb;
static uint8_t rb_buffer[LG_UART_RX_BUFFER_SIZE * 2];

static nmbs_bitfield server_coils = {0};
static uint16_t server_registers[LB_MODBUS_ADDR_MAX + 1] = {0};

static nmbs_platform_conf platform_conf;
static nmbs_callbacks callbacks;
static nmbs_t nmbs;
/* ============================================================================
 * private function prototype
 * ========================================================================= */
static int32_t lg_module_read_serial(uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg);

static int32_t lg_module_write_serial(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg);

static nmbs_error handle_read_coils(uint16_t address, uint16_t quantity, nmbs_bitfield coils_out, uint8_t unit_id, void *arg);

static nmbs_error handle_write_multiple_coils(uint16_t address, uint16_t quantity, const nmbs_bitfield coils, uint8_t unit_id,
		void *arg);

static nmbs_error handler_read_holding_registers(uint16_t address, uint16_t quantity, uint16_t *registers_out, uint8_t unit_id,
		void *arg);
static nmbs_error handle_write_multiple_registers(uint16_t address, uint16_t quantity, const uint16_t *registers,
		uint8_t unit_id, void *arg);
static nmbs_error handle_write_single_register(uint16_t address, uint16_t value,
		uint8_t unit_id, void *arg);

static void modbus_server_update(void);
nmbs_error modbus_tcp_write_data(uint16_t address, uint16_t val);
/* ============================================================================
 * public function definition
 * ========================================================================= */
uint8_t lg_module_modbus_init(uint8_t addr)
{
	uint8_t ret = 0;

	/*ring buffer init*/
	lwrb_init(&rb, rb_buffer, LG_UART_RX_BUFFER_SIZE);
	/*start rx with uart*/
	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);

	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, LG_UART_RX_BUFFER_SIZE);

	/*modbus server init*/
	nmbs_platform_conf_create(&platform_conf);
	platform_conf.transport = NMBS_TRANSPORT_RTU;
	platform_conf.read = lg_module_read_serial;
	platform_conf.write = lg_module_write_serial;
	platform_conf.arg = NULL;

	nmbs_callbacks_create(&callbacks);
	callbacks.read_coils = handle_read_coils;
	callbacks.write_multiple_coils = handle_write_multiple_coils;
	callbacks.read_holding_registers = handler_read_holding_registers;
	callbacks.write_multiple_registers = handle_write_multiple_registers;
	callbacks.write_single_register = handle_write_single_register;

	nmbs_bitfield_set(server_coils, 0);

	nmbs_error err = nmbs_server_create(&nmbs, addr, &platform_conf, &callbacks);
	if (err != NMBS_ERROR_NONE)
	{
		return 1;
	}

	nmbs_set_read_timeout(&nmbs, 1000);
	nmbs_set_byte_timeout(&nmbs, 1000);

	return ret;
}

uint8_t lg_module_modbus_set_addr(uint8_t addr)
{
	uint8_t ret = 0;

	nmbs.address_rtu = addr;

	return ret;
}

uint8_t lg_module_modbus_pool(void)
{
	nmbs_error err = nmbs_server_poll(&nmbs);
	if (err != NMBS_ERROR_NONE)
	{
		return 1;
	}

	return 0;
}
/* ============================================================================
 * private function definition
 * ========================================================================= */

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, LG_UART_RX_BUFFER_SIZE);

	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	/*write to ring buffer*/
	lwrb_write(&rb, rx_buffer, Size);
	/*Receive another data*/
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, LG_UART_RX_BUFFER_SIZE);

	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);

	return;
}

static int32_t lg_module_read_serial(uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
	uint32_t ticks = HAL_GetTick();

	while ((HAL_GetTick() - ticks) <= byte_timeout_ms)
	{
		if (lwrb_get_full(&rb) >= count)
		{
			// read
			return lwrb_read(&rb, buf, count);
		}
	}

	return 0;
}

static int32_t lg_module_write_serial(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg)
{
	int32_t ret = count;
	// set output dir
	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_SET);

	if (HAL_UART_Transmit(&huart1, buf, count, byte_timeout_ms) != HAL_OK)
	{
		ret = 0;
	}
	// set input dir
	HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);

	return ret;
}

static nmbs_error handle_read_coils(uint16_t address, uint16_t quantity, nmbs_bitfield coils_out, uint8_t unit_id, void *arg)
{
	if (address + quantity > COILS_ADDR_MAX + 1)
		return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

	// update current data
	modbus_server_update();
	// Read our coils values into coils_out
	for (int i = 0; i < quantity; i++)
	{
		bool value = nmbs_bitfield_read(server_coils, address + i);
		nmbs_bitfield_write(coils_out, i, value);
	}

	return NMBS_ERROR_NONE;
}

static nmbs_error handle_write_multiple_coils(uint16_t address, uint16_t quantity, const nmbs_bitfield coils, uint8_t unit_id,
		void *arg)
{
	if (address + quantity > COILS_ADDR_MAX + 1)
		return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

	// Write coils values to our server_coils
	for (int i = 0; i < quantity; i++)
	{
		nmbs_bitfield_write(server_coils, address + i, nmbs_bitfield_read(coils, i));
	}

	return NMBS_ERROR_NONE;
}

static nmbs_error handler_read_holding_registers(uint16_t address, uint16_t quantity, uint16_t *registers_out, uint8_t unit_id,
		void *arg)
{
	if (address + quantity > LB_MODBUS_ADDR_MAX + 1)
		return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

	// update current data
	modbus_server_update();
	// Read our registers values into registers_out
	for (int i = 0; i < quantity; i++)
		registers_out[i] = server_registers[address + i];

	return NMBS_ERROR_NONE;
}

static nmbs_error handle_write_multiple_registers(uint16_t address, uint16_t quantity, const uint16_t *registers,
		uint8_t unit_id, void *arg)
{
	nmbs_error err = NMBS_ERROR_NONE;

	if (address + quantity > LB_MODBUS_ADDR_MAX + 1)
		return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

	// Write registers values to our server_registers
	for (int i = 0; i < quantity; i++)
	{
		if (modbus_tcp_write_data(address + i, registers[i]) != NMBS_ERROR_NONE)
		{
			err = NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
			break;
		}
	}

	modbus_server_update();

	return err;
}

static nmbs_error handle_write_single_register(uint16_t address, uint16_t value,
		uint8_t unit_id, void *arg)
{
	return handle_write_multiple_registers(address, 1, &value, unit_id, arg);
}

static void modbus_server_update(void)
{
	LG_CONF_TypeDef_t conf = {0};
	LG_SENSOR_TypeDef_t sensor = {0};
	/*get current data */
	lg_module_eeprom_conf_get(&conf);

	lg_module_sensor_get(&sensor);
	/*update register -----------------------------------------------------*/
	memset(server_registers, 0, LB_MODBUS_ADDR_MAX);
	/*conf data**/
	for (uint8_t i = OFFSET_S1_ADDR; i < LG_ADC_SENAOR_MAX_SIZE; i++)
	{
		server_registers[i] = (uint16_t)conf.offset[i];
	}
	server_registers[SERVER_ADDR] = conf.address;
	server_registers[FILTER_FC_ADDR] = (uint16_t)(conf.fc * 10);
	server_registers[SENSOR_THRESHOLD_ADDR] = conf.threshold;
	/*sensor filtered data*/
	for (uint8_t i = S1_ADDR; i < LG_ADC_SENAOR_MAX_SIZE; i++)
	{
		server_registers[i] = sensor.S[i - S1_ADDR];
	}
	/*sensor adc value*/
	for (uint8_t i = A1_ADDR; i < LG_ADC_SENAOR_MAX_SIZE; i++)
	{
		server_registers[i] = sensor.raw[i - A1_ADDR];
	}
	/*offset apply data*/
	for (uint8_t i = D1_ADDR; i < LG_ADC_SENAOR_MAX_SIZE; i++)
	{
		server_registers[i] = sensor.D[i - D1_ADDR];
	}
	server_registers[DI_VALUE_ADDR] = sensor.value;

	/*update cooils*/
	for (uint8_t i = 0; i < LG_ADC_SENAOR_MAX_SIZE; i++)
	{
		if (sensor.value & (1 << i))
		{
			nmbs_bitfield_set(server_coils, i);
		}
		else
		{

			nmbs_bitfield_unset(server_coils, i);
		}
	}
	return;
}

nmbs_error modbus_tcp_write_data(uint16_t address, uint16_t val)
{
	/*local variables*/
	LG_CONF_TypeDef_t conf = {0};
	nmbs_error err = NMBS_ERROR_NONE;
	LG_SENSOR_TypeDef_t sensor = {0};
	/*get current*/
	lg_module_eeprom_conf_get(&conf);

	switch (address)
	{
	case SERVER_ADDR:
		conf.address = val % 128;
		lg_module_modbus_set_addr(conf.address);
		/* code */
		break;
	case OFFSET_S1_ADDR:
		conf.offset[OFFSET_S1_ADDR] = val % 4096;
		break;
	case OFFSET_S2_ADDR:
		conf.offset[OFFSET_S2_ADDR] = val % 4096;
		break;
	case OFFSET_S3_ADDR:
		conf.offset[OFFSET_S3_ADDR] = val % 4096;
		break;
	case OFFSET_S4_ADDR:
		conf.offset[OFFSET_S4_ADDR] = val % 4096;
		break;
	case OFFSET_S5_ADDR:
		conf.offset[OFFSET_S5_ADDR] = val % 4096;
		break;
	case OFFSET_S6_ADDR:
		conf.offset[OFFSET_S6_ADDR] = val % 4096;
		break;
	case OFFSET_S7_ADDR:
		conf.offset[OFFSET_S7_ADDR] = val % 4096;
		break;
	case OFFSET_S8_ADDR:
		conf.offset[OFFSET_S8_ADDR] = val % 4096;
		break;
	case OFFSET_S9_ADDR:
		conf.offset[OFFSET_S9_ADDR] = val % 4096;
		break;
	case OFFSET_S10_ADDR:
		conf.offset[OFFSET_S10_ADDR] = val % 4096;
		break;
	case FILTER_FC_ADDR:
		conf.fc = (float)val / 10;
		lg_module_sensor_filter_set(conf.fc);

		break;
	case SENSOR_THRESHOLD_ADDR:
		conf.threshold = val % 4096;
		break;
	case CALIB_FLAG_ADDR:
		/*CALCULATE OFFSET*/
		lg_module_sensor_get(&sensor);
		for (uint8_t i = 0; i < LG_ADC_SENAOR_MAX_SIZE; i++)
		{
			conf.offset[i] = sensor.S[i];
		}
		break;
	case FACTORY_RESET_ADDR:

		memset(&conf, 0, sizeof(LG_CONF_TypeDef_t));

		conf.address = LG_MODBUS_SERVER_DEFAULT_ADDR;
		conf.fc = LB_FILTER_FC_DEFAULT;
		conf.threshold = LB_THRESHOLD_DEFAULT;

		break;

	default:
		err = NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
		break;
	}
	/*save data*/
	if (err != NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS)
	{
		lg_module_eeprom_conf_set(&conf);
	}

	return err;
}
