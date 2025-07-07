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
#include "stm32f1xx_hal.h"

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
typedef enum {
  PROJ_OK = 0,
  PROJ_ERROR,
  PROJ_TIMEOUT,
  PROJ_NO_SPACE,
} PROJ_RET_Typedef;
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define XPT2046_TOUCH_IRQ_Pin GPIO_PIN_2
#define XPT2046_TOUCH_IRQ_GPIO_Port GPIOE
#define XPT2046_TOUCH_IRQ_EXTI_IRQn EXTI2_IRQn
#define XPT2046_TOUCH_CS_Pin GPIO_PIN_3
#define XPT2046_TOUCH_CS_GPIO_Port GPIOE
#define ZW101_PendIRQ_Pin GPIO_PIN_4
#define ZW101_PendIRQ_GPIO_Port GPIOA
#define ZW101_PendIRQ_EXTI_IRQn EXTI4_IRQn
#define LED_GREEN_Pin GPIO_PIN_0
#define LED_GREEN_GPIO_Port GPIOB
#define LED_BLUE_Pin GPIO_PIN_1
#define LED_BLUE_GPIO_Port GPIOB
#define ILI9341_BL_Pin GPIO_PIN_12
#define ILI9341_BL_GPIO_Port GPIOD
#define FLASH_CS_Pin GPIO_PIN_6
#define FLASH_CS_GPIO_Port GPIOC
#define ZW101_Power_Pin GPIO_PIN_12
#define ZW101_Power_GPIO_Port GPIOA
#define LED_RED_Pin GPIO_PIN_5
#define LED_RED_GPIO_Port GPIOB
#define ILI9341_RES_Pin GPIO_PIN_1
#define ILI9341_RES_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */
// 绿灯
#define LED_GREEN_ON HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET)
#define LED_GREEN_OFF HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET)
#define LED_GREEN_TOGGLE HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin)

// 蓝灯
#define LED_BLUE_ON HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET)
#define LED_BLUE_OFF HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET)
#define LED_BLUE_TOGGLE HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin)

// 红灯
#define LED_RED_ON HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET)
#define LED_RED_OFF HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET)
#define LED_RED_TOGGLE HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
