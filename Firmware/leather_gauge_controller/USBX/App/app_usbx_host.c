/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    app_usbx_host.c
 * @author  MCD Application Team
 * @brief   USBX host applicative file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2021 STMicroelectronics.
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
#include "app_usbx_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lgc_interface_printer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define USBX_MEMORY_SIZE (32 * 1024)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */
/**
  * @brief  Application USBX Host Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT MX_USBX_Host_Init(VOID *memory_ptr)
{
  UINT ret = UX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN MX_USBX_Host_MEM_POOL */
  char *pointer;
  /* USER CODE END MX_USBX_Host_MEM_POOL */

  /* USER CODE BEGIN MX_USBX_Host_Init */
  if (tx_byte_allocate(byte_pool, (VOID **)&pointer,
                       USBX_MEMORY_SIZE, TX_NO_WAIT) != TX_SUCCESS)
  {
    return TX_POOL_ERROR;
  }
  /* Initialize USBX memory. */
  if (ux_system_initialize(pointer, USBX_MEMORY_SIZE, UX_NULL, 0) != UX_SUCCESS)
  {
    return UX_ERROR;
  }

  ret = lgc_interface_printer_init();
  /* USER CODE END MX_USBX_Host_Init */

  return ret;
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
