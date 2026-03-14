/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId mainTaskHandle;
osThreadId p10TaskHandle;
osThreadId modbusServerHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
SemaphoreHandle_t p10_update;
/* USER CODE END FunctionPrototypes */

void display_p10_main_task_entry(void const * argument);
void display_p10_udapte_task_entry(void const * argument);
void modbusServerRTU_Task(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	p10_update = xSemaphoreCreateBinary();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of mainTask */
  osThreadDef(mainTask, display_p10_main_task_entry, osPriorityAboveNormal, 0, 128);
  mainTaskHandle = osThreadCreate(osThread(mainTask), NULL);

  /* definition and creation of p10Task */
  osThreadDef(p10Task, display_p10_udapte_task_entry, osPriorityNormal, 0, 256);
  p10TaskHandle = osThreadCreate(osThread(p10Task), NULL);

  /* definition and creation of modbusServer */
  osThreadDef(modbusServer, modbusServerRTU_Task, osPriorityLow, 0, 256);
  modbusServerHandle = osThreadCreate(osThread(modbusServer), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_display_p10_main_task_entry */
/**
  * @brief  Function implementing the mainTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_display_p10_main_task_entry */
__weak void display_p10_main_task_entry(void const * argument)
{
  /* USER CODE BEGIN display_p10_main_task_entry */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END display_p10_main_task_entry */
}

/* USER CODE BEGIN Header_display_p10_udapte_task_entry */
/**
* @brief Function implementing the p10Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_display_p10_udapte_task_entry */
__weak void display_p10_udapte_task_entry(void const * argument)
{
  /* USER CODE BEGIN display_p10_udapte_task_entry */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END display_p10_udapte_task_entry */
}

/* USER CODE BEGIN Header_modbusServerRTU_Task */
/**
* @brief Function implementing the modbusServer thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_modbusServerRTU_Task */
__weak void modbusServerRTU_Task(void const * argument)
{
  /* USER CODE BEGIN modbusServerRTU_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END modbusServerRTU_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

