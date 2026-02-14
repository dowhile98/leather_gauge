/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DIR_DISPLAY_Pin GPIO_PIN_13
#define DIR_DISPLAY_GPIO_Port GPIOC
#define DI_2_Pin GPIO_PIN_0
#define DI_2_GPIO_Port GPIOC
#define DI_3_Pin GPIO_PIN_1
#define DI_3_GPIO_Port GPIOC
#define DI_4_Pin GPIO_PIN_2
#define DI_4_GPIO_Port GPIOC
#define DI_5_Pin GPIO_PIN_3
#define DI_5_GPIO_Port GPIOC
#define DI_0_INT_Pin GPIO_PIN_0
#define DI_0_INT_GPIO_Port GPIOA
#define DI_0_INT_EXTI_IRQn EXTI0_IRQn
#define DO_0_Pin GPIO_PIN_0
#define DO_0_GPIO_Port GPIOB
#define DO_1_Pin GPIO_PIN_1
#define DO_1_GPIO_Port GPIOB
#define DIR_SENSORES_Pin GPIO_PIN_14
#define DIR_SENSORES_GPIO_Port GPIOB
#define D0_7_Pin GPIO_PIN_15
#define D0_7_GPIO_Port GPIOB
#define D0_2_Pin GPIO_PIN_3
#define D0_2_GPIO_Port GPIOB
#define D0_6_Pin GPIO_PIN_9
#define D0_6_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
