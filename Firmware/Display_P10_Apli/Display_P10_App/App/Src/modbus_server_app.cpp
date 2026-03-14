/*
 * modbus_server_app.cpp
 *
 *  Created on: Aug 20, 2024
 *      Author: eplim
 */


/*Includes ---------------------------------------------------------------*/
#include "nanomodbus.h"
#include "usart.h"
#include "lwrb.h"
#include "cmsis_os.h"
#include "dmd_app.h"
#include "driver_at24cxx.h"
/*Defines ----------------------------------------------------------------*/
#define UNUSED_PARAM(x)			(void)x
#define RX_TIMEOUT				1000 * 4

/*Typedefs ---------------------------------------------------------------*/
typedef enum modbus_reg_addr{
	BRIGHTNES_ADDR				= 0x0, 	//40001
	BAUD_PARITY_ADDR			= 0x1, 	//40002
	ADDRESS_ADDR				= 0x2, 	//40003
	TIME_TOGGLE_ADDR			= 0x3,	//40004
	PRESSURE1_ADDR				= 0x4,  //40005
	PRESSURE2_ADDR				= 0x5,	//40006
	PRESSURE3_ADDR				= 0x6,	//40007
	TEMPERATURE1_ADDR			= 0x7,	//40008
	TEMPERATURE2_ADDR			= 0x8,	//40009
	//add -----------------------------------------------------------------------------------------------------
	CURRENT_SCREEN_ADDR			= 0x9,  //40010 current screen show (0, 1, 2, 3)
	NUMERIC_VALUE1_ADDR			= 0xA,  //40011 valor numerico T/H (0-9999)
	NUMERIC_VALUE2_ADDR			= 0xB,  //40012 valor numerico A(0-999)
	NUMERIC_VALUE3_ADDR			= 0xC,  //40013 valor numerico %(0-999)

	REGS_ADDR_MAX,

}modbus_reg_addr_t;
/*Global variables -------------------------------------------------------*/
lwrb_t modbusRB;
uint8_t modbusRXbuffer[128];
uint16_t server_registers[REGS_ADDR_MAX] = {0};
uint8_t rxBuff[138];

uint32_t bauds[] = {
		2400,
		4800,
		9600,
		38400,
		115200,
};
SemaphoreHandle_t rxFlag;
nmbs_t nmbsRtu;
extern at24cxx_handle_t eHandle;
uint32_t rxTimeout = 0;

SemaphoreHandle_t p10_update_flag;
/*Function prototype -----------------------------------------------------*/
extern "C"{
	void modbusServerRTU_Task(void const * argument);
	extern SemaphoreHandle_t p10_update;
}
void USART1_UART_Init(uint32_t baud);
/**
 * modbus server callbacks
 */
int32_t modbus_rtu_read(uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg);
int32_t modbus_rtu_write(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg);
nmbs_error handler_read_holding_registers(uint16_t address, uint16_t quantity, uint16_t* registers_out, uint8_t unit_id, void* arg);
nmbs_error handle_write_multiple_registers(uint16_t address, uint16_t quantity, const uint16_t* registers, uint8_t unit_id, void* arg);
nmbs_error handle_write_single_register(uint16_t address, uint16_t value, uint8_t unit_id, void* arg);
nmbs_error modbus_rtu_write_data(uint16_t address, uint16_t val);
/**
 * data update for modbus access
 */
static void modbus_server_data_update(void);
/*Task definition --------------------------------------------------------*/
void modbusServerRTU_Task(void const * argument){
	/*Local variables ----------------------------------------------------*/

	nmbs_callbacks callbacks = {0};
	nmbs_platform_conf platform_conf;
	nmbs_error err;

	/*Wait for init ------------------------------------------------------*/

	osDelay(400);
	//reinit uart config
	HAL_UART_DeInit(&huart1);
	USART1_UART_Init(bauds[data.conf.baud]);
	RS485_DIR_GPIO_Port->BRR |= RS485_DIR_Pin;
	/*Init ---------------------------------------------------------------*/
	rxFlag = xSemaphoreCreateBinary();
	configASSERT(rxFlag != NULL);

	lwrb_init(&modbusRB, modbusRXbuffer, 128);
	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxBuff, 128);
	RS485_DIR_GPIO_Port->BRR |= RS485_DIR_Pin;
	/*ModBus -------------------------------------------------------------*/
	platform_conf.transport = NMBS_TRANSPORT_RTU;
	platform_conf.read = modbus_rtu_read;
	platform_conf.write = modbus_rtu_write;
	platform_conf.arg = NULL;    // We will set the arg (socket fd) later
	/**
	 * callbacks server
	 */
	callbacks.read_holding_registers = handler_read_holding_registers;
	callbacks.write_single_register = handle_write_single_register;
	callbacks.write_multiple_registers = handle_write_multiple_registers;

	/**
	 * create modbus server
	 */
	nmbs_server_create(&nmbsRtu, data.conf.addr, &platform_conf, &callbacks);
	if (err != NMBS_ERROR_NONE) {

	}
	/**
	 * set timeout
	 */
	nmbs_set_read_timeout(&nmbsRtu, 1000);
	nmbs_set_byte_timeout(&nmbsRtu, 1000);
	for(;;){
		err = nmbs_server_poll(&nmbsRtu);		//modbus tcp pool
		if(err != NMBS_ERROR_NONE){

		}
		if((HAL_GetTick() - rxTimeout)> RX_TIMEOUT ){
			rxTimeout = HAL_GetTick();
			HAL_UART_AbortReceive_IT(&huart1);
			/*Reinit receive*/
			HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxBuff, 128);
			RS485_DIR_GPIO_Port->BRR |= RS485_DIR_Pin;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}



/*Callbacks --------------------------------------------------------------*/
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	HAL_UARTEx_ReceiveToIdle_DMA(huart, rxBuff, 128);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	portBASE_TYPE taskWoken = pdFALSE;

	//write rb
	lwrb_write(&modbusRB, rxBuff, 128);
	HAL_UARTEx_ReceiveToIdle_DMA(huart, rxBuff, 128);

	//send semaphore
	if (xSemaphoreGiveFromISR(rxFlag, &taskWoken) == pdTRUE) {
		portEND_SWITCHING_ISR(taskWoken);
	}

	rxTimeout = HAL_GetTick();
}
/*Function definition ----------------------------------------------------*/
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){

	portBASE_TYPE taskWoken = pdFALSE;

	//write to ring buffer
	lwrb_write(&modbusRB, rxBuff, Size);
	HAL_UARTEx_ReceiveToIdle_DMA(huart, rxBuff, 128);

	//send semaphore
	if (xSemaphoreGiveFromISR(rxFlag, &taskWoken) == pdTRUE) {
		portEND_SWITCHING_ISR(taskWoken);
	}
	rxTimeout = HAL_GetTick();
	return;
}
int32_t modbus_rtu_read(uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg){
	uint32_t ticks = xTaskGetTickCount();
	int32_t ret = 0;
	do{
		if((ret =lwrb_get_full(&modbusRB)) >= count){
			ret = lwrb_read(&modbusRB, buf, count);
			break;
		}else{
			ret = 0;
			xSemaphoreTake(rxFlag, byte_timeout_ms / 4);
		}
	}while((xTaskGetTickCount() - ticks) <= pdMS_TO_TICKS(byte_timeout_ms));

	return ret;
}
int32_t modbus_rtu_write(const uint8_t* buf, uint16_t count, int32_t byte_timeout_ms, void* arg){

	//SET DIR
	RS485_DIR_GPIO_Port->BSRR |= RS485_DIR_Pin;
	if(HAL_UART_Transmit(&huart1, buf, count, byte_timeout_ms) != HAL_OK){
		return 0;
	}

	//RESET DIR
	RS485_DIR_GPIO_Port->BRR |= RS485_DIR_Pin;
	return count;
}

/**
 * modbus server callbaks
 */
static void modbus_server_data_update(void){
	/**
	 * configuration address
	 */
	server_registers[BRIGHTNES_ADDR] = data.conf.brightnes;
	server_registers[BAUD_PARITY_ADDR] = data.conf.baud;
	server_registers[ADDRESS_ADDR] = data.conf.addr;
	server_registers[TIME_TOGGLE_ADDR] = data.conf.toggleTime;
	/**
	 * variable data
	 */
	server_registers[TEMPERATURE1_ADDR] = data.temp1 ;
	server_registers[PRESSURE1_ADDR] = data.pressure1 ;
	server_registers[PRESSURE2_ADDR] = data.pressure2 ;
	server_registers[PRESSURE3_ADDR] = data.pressure3 ;
	server_registers[TEMPERATURE2_ADDR] = data.temp2 ;
	//add -----------------------------------------------------------------------------------------------------
	server_registers[CURRENT_SCREEN_ADDR] = data.screen;
	server_registers[NUMERIC_VALUE1_ADDR] = data.numeric1;
	server_registers[NUMERIC_VALUE2_ADDR] = data.numeric2;
	server_registers[NUMERIC_VALUE3_ADDR] = data.numeric3;

	return;
}

nmbs_error handler_read_holding_registers(uint16_t address, uint16_t quantity, uint16_t* registers_out, uint8_t unit_id,
                                          void* arg) {
    UNUSED_PARAM(arg);
    UNUSED_PARAM(unit_id);

    if (address + quantity > REGS_ADDR_MAX + 1)
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    /**
     * udapte modbus data
     */
    modbus_server_data_update();
    // Read our registers values into registers_out
    for (int i = 0; i < quantity; i++)
        registers_out[i] = server_registers[address + i];

    return NMBS_ERROR_NONE;
}

nmbs_error handle_write_single_register(uint16_t address, uint16_t value, uint8_t unit_id, void* arg){
	return handle_write_multiple_registers(address, 1, &value, unit_id, arg);
}

nmbs_error handle_write_multiple_registers(uint16_t address, uint16_t quantity, const uint16_t* registers,
                                           uint8_t unit_id, void* arg) {
    UNUSED_PARAM(arg);
    UNUSED_PARAM(unit_id);
    nmbs_error err = NMBS_ERROR_NONE;

    if (address + quantity > REGS_ADDR_MAX + 1)
        return NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;

    // Write registers values to our server_registers
    for (int i = 0; i < quantity; i++){
    	if(modbus_rtu_write_data(address + i, registers[i]) != NMBS_ERROR_NONE){
    		err = NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
    		break;
    	}
    }

    /**
     * update register
     */
    modbus_server_data_update();
    return err;
}


nmbs_error modbus_rtu_write_data(uint16_t address, uint16_t val){
	nmbs_error err = NMBS_ERROR_NONE;
	switch(address){
	case BRIGHTNES_ADDR:
		if(val>= 1){
			data.conf.brightnes = val;
		}


		//save data
		at24cxx_write(&eHandle, CONF_ADDR, (uint8_t *)&data.conf, sizeof(dmd_p10_config_t));
		break;
	case BAUD_PARITY_ADDR:
		if(val < 5){
			data.conf.baud = val;
		}
		//reconfigure uart
		HAL_UART_DeInit(&huart1);
		USART1_UART_Init(bauds[data.conf.baud]);

		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rxBuff, 128);
		//save data
		at24cxx_write(&eHandle, CONF_ADDR, (uint8_t *)&data.conf, sizeof(dmd_p10_config_t));

		vTaskDelay(pdMS_TO_TICKS(100));
		NVIC_SystemReset();
		break;
	case ADDRESS_ADDR:
		data.conf.addr = val;
		nmbsRtu.address_rtu = val;
		//save data
		at24cxx_write(&eHandle, CONF_ADDR, (uint8_t *)&data.conf, sizeof(dmd_p10_config_t));
		break;
	case TEMPERATURE1_ADDR:
		data.temp1 = val ;
		break;
	case PRESSURE1_ADDR:
		data.pressure1 = val ;
		break;
	case PRESSURE2_ADDR:
		data.pressure2 = val ;
		break;
	case PRESSURE3_ADDR:
		data.pressure3 = val ;
		break;
	case TEMPERATURE2_ADDR:
		data.temp2 = val ;
		break;
	case TIME_TOGGLE_ADDR:
		data.conf.toggleTime = val;
		if(data.conf.toggleTime < 1){
			data.conf.toggleTime = 1;
		}
		//save data
		at24cxx_write(&eHandle, CONF_ADDR, (uint8_t *)&data.conf, sizeof(dmd_p10_config_t));
		break;
	//add -----------------------------------------------------------------------------------------------------
	case CURRENT_SCREEN_ADDR:
		data.screen = val % 4;
		break;
	case NUMERIC_VALUE1_ADDR:
		data.numeric1 = val % (10000);
		break;
	case NUMERIC_VALUE2_ADDR:
		data.numeric2 = val % 1000;
		break;
	case NUMERIC_VALUE3_ADDR:
		data.numeric3 = val;
		break;
	default:
		err = NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS;
	}

	if(err != NMBS_EXCEPTION_ILLEGAL_DATA_ADDRESS)
	{
		xSemaphoreGive(p10_update);
	}
	return err;
}

void USART1_UART_Init(uint32_t baud)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = baud;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }


}
