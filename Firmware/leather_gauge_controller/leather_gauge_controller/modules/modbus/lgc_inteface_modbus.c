/*
 * lgc_inteface_modbus.c
 *
 *  Created on: Jan 6, 2026
 *      Author: tecna-smart-lab
 */

/**
 * @file lgc_interface_modbus.c
 * @brief Modbus RTU interface implementation for UART communication
 */

/* ============================================================================
 * INCLUDES
 * ============================================================================ */
#include "lgc_interface_modbus.h"
#include "nanomodbus.h"
#include "usart.h"
#include "lwrb.h"

/* ============================================================================
 * DEFINES
 * ============================================================================ */
#ifndef NMBS_READ_TIMEOUT
#define NMBS_READ_TIMEOUT 100
#endif

#ifndef NMBS_WRITE_TIMEOUT
#define NMBS_WRITE_TIMEOUT 1000
#endif

#ifndef MODBUS_RX_BUFFER_SIZE
#define MODBUS_RX_BUFFER_SIZE 128
#endif

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================ */
static nmbs_platform_conf platform_conf;
static nmbs_t nmbs;
static OsMutex mutex;
static lwrb_t modbus_rx_rb;
static uint8_t modbus_rx_buffer[MODBUS_RX_BUFFER_SIZE];
static uint8_t modbus_rx_temp_buffer[MODBUS_RX_BUFFER_SIZE];
static OsSemaphore modbus_rx_semaphore;

/* ============================================================================
 * PRIVATE FUNCTION PROTOTYPES
 * ============================================================================ */
static int32_t lgc_modbus_uart_read(uint8_t *buffer, uint16_t count, int32_t timeout, void *args);
static int32_t lgc_modbus_uart_write(const uint8_t *buffer, uint16_t count, int32_t timeout, void *args);
static void lgc_modbus_rx_callback(UART_HandleTypeDef *huart, uint16_t Pos);
static void lgc_modbus_uart_error_callback(UART_HandleTypeDef *huart);

/* ============================================================================
 * PUBLIC FUNCTION DEFINITIONS
 * ============================================================================ */

/**
 * @brief Initialize Modbus interface
 * @return error_t Status of initialization
 */
error_t lgc_interface_modbus_init(void)
{
	error_t err = NO_ERROR;

	nmbs_platform_conf_create(&platform_conf);
	platform_conf.transport = NMBS_TRANSPORT_RTU;
	platform_conf.read = lgc_modbus_uart_read;
	platform_conf.write = lgc_modbus_uart_write;
	platform_conf.arg = NULL;

	err = nmbs_client_create(&nmbs, &platform_conf);

	nmbs_set_destination_rtu_address(&nmbs, 1); // Set Modbus slave address

	// set timeout
	nmbs_set_byte_timeout(&nmbs, NMBS_WRITE_TIMEOUT);
	nmbs_set_read_timeout(&nmbs, NMBS_READ_TIMEOUT);

	nmbs_set_byte_timeout(&nmbs, NMBS_READ_TIMEOUT);

	/* init modbus mutex */
	if (osCreateMutex(&mutex) != TRUE)
	{
		return ERROR_FAILURE;
	}

	/* init modbus rx semaphore */
	if (osCreateSemaphore(&modbus_rx_semaphore, 0) != TRUE)
	{
		return ERROR_FAILURE;
	}

	/* init modbus rx ring buffer */
	lwrb_init(&modbus_rx_rb, modbus_rx_buffer, MODBUS_RX_BUFFER_SIZE);

	/* register callbacks */
	HAL_UART_RegisterRxEventCallback(&huart3, lgc_modbus_rx_callback);
	HAL_UART_RegisterCallback(&huart3, HAL_UART_ERROR_CB_ID, lgc_modbus_uart_error_callback);

	HAL_GPIO_WritePin(DIR_SENSORES_GPIO_Port, DIR_SENSORES_Pin, GPIO_PIN_RESET);

	// start reception
	err = HAL_UARTEx_ReceiveToIdle_DMA(&huart3, &modbus_rx_temp_buffer[0], MODBUS_RX_BUFFER_SIZE / 2);
	
	return err;
}

/**
 * @brief Read holding registers from Modbus device
 * @param dev Device address
 * @param address Starting address
 * @param regs Pointer to register array
 * @param len Number of registers to read
 * @return error_t Status of operation
 */
error_t lgc_modbus_read_holding_regs(uint8_t dev, uint16_t address, uint16_t *regs, size_t len)
{
	osAcquireMutex(&mutex);

	nmbs_set_destination_rtu_address(&nmbs, dev);

	error_t err = nmbs_read_holding_registers(&nmbs, address, len, regs);

	osReleaseMutex(&mutex);

	return err;
}

/**
 * @brief Read coils from Modbus device
 * @param dev Device address
 * @param address Starting address
 * @param coils Pointer to coils array
 * @param len Number of coils to read
 * @return error_t Status of operation
 */
error_t lgc_modbus_read_coils(uint8_t dev, uint16_t address, uint8_t *coils, size_t len)
{
	osAcquireMutex(&mutex);

	nmbs_set_destination_rtu_address(&nmbs, dev);

	error_t err = nmbs_read_coils(&nmbs, address, len, coils);

	osReleaseMutex(&mutex);

	return err;
}

/**
 * @brief Write holding registers to Modbus device
 * @param dev Device address
 * @param address Starting address
 * @param regs Pointer to register array
 * @param len Number of registers to write
 * @return error_t Status of operation
 */
error_t lgc_modbus_write_holding_regs(uint8_t dev, uint16_t address, uint16_t *regs, size_t len)
{
	osAcquireMutex(&mutex);

	nmbs_set_destination_rtu_address(&nmbs, dev);

	error_t err = nmbs_write_multiple_registers(&nmbs, address, len, regs);

	osReleaseMutex(&mutex);

	return err;
}

/* ============================================================================
 * PRIVATE FUNCTION DEFINITIONS
 * ============================================================================ */

/**
 * @brief UART read function for Modbus
 * @param buffer Pointer to read buffer
 * @param count Number of bytes to read
 * @param timeout Read timeout in milliseconds
 * @param args Additional arguments
 * @return int32_t Number of bytes read
 */
static int32_t lgc_modbus_uart_read(uint8_t *buffer, uint16_t count, int32_t timeout, void *args)
{
	/* Set direction for RS485 transceiver to RX mode */
	HAL_GPIO_WritePin(DIR_SENSORES_GPIO_Port, DIR_SENSORES_Pin, GPIO_PIN_RESET);

	if(HAL_UART_Receive(&huart3, buffer, count, NMBS_READ_TIMEOUT) == HAL_OK)
	{
		return count;
	}

	if (lwrb_get_full(&modbus_rx_rb) >= count)
	{
		lwrb_read(&modbus_rx_rb, buffer, count);
		return (int32_t)count;
	}
	else if (osWaitForSemaphore(&modbus_rx_semaphore, NMBS_READ_TIMEOUT) == TRUE)
	{
		if (lwrb_get_full(&modbus_rx_rb) >= count)
		{
			lwrb_read(&modbus_rx_rb, buffer, count);
			return (int32_t)count;
		}
	}

	return 0;
}

/**
 * @brief UART write function for Modbus
 * @param buffer Pointer to write buffer
 * @param count Number of bytes to write
 * @param timeout Write timeout in milliseconds
 * @param args Additional arguments
 * @return int32_t Number of bytes written
 */
static int32_t lgc_modbus_uart_write(const uint8_t *buffer, uint16_t count, int32_t timeout, void *args)
{
	/* Set direction for RS485 transceiver to TX mode */
	HAL_GPIO_WritePin(DIR_SENSORES_GPIO_Port, DIR_SENSORES_Pin, GPIO_PIN_SET);

	if (HAL_UART_Transmit(&huart3, (uint8_t *)buffer, count, timeout) == HAL_OK)
	{
		/* Set direction back to RX mode */
		HAL_GPIO_WritePin(DIR_SENSORES_GPIO_Port, DIR_SENSORES_Pin, GPIO_PIN_RESET);
		return (int32_t)count;
	}

	/* Set direction back to RX mode on error */
	HAL_GPIO_WritePin(DIR_SENSORES_GPIO_Port, DIR_SENSORES_Pin, GPIO_PIN_RESET);

	return 0;
}

/**
 * @brief UART error callback
 * @param huart UART handle
 */
static void lgc_modbus_uart_error_callback(UART_HandleTypeDef *huart)
{
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, &modbus_rx_temp_buffer[0], MODBUS_RX_BUFFER_SIZE / 2);
}

/**
 * @brief UART RX complete callback
 * @param huart UART handle
 * @param Pos Current position in DMA buffer
 */
static void lgc_modbus_rx_callback(UART_HandleTypeDef *huart, uint16_t Pos)
{
	/* Write data to ring buffer */
	lwrb_write(&modbus_rx_rb, &modbus_rx_temp_buffer[0], Pos);

	/* Restart reception */
	HAL_UARTEx_ReceiveToIdle_DMA(&huart3, &modbus_rx_temp_buffer[0], MODBUS_RX_BUFFER_SIZE / 2);

	/* Release semaphore to signal data availability */
	osReleaseSemaphore(&modbus_rx_semaphore);
}
