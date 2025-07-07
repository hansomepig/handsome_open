#ifndef __ULN2003_H__
#define __ULN2003_H__

#include "stm32f1xx_hal.h"
#include "sz_system.h"

// 连接 ULN2003 电机驱动模块 IN1, IN2, IN3, IN4 引脚的 GPIO 定义
#define ULN2003_IN1_GPIO_Port GPIOC
#define ULN2003_IN1_Pin GPIO_PIN_2
#define ULN2003_CONFIG_IN1_GPIO_PIN(value) PCout(2) = !!(value)

#define ULN2003_IN2_GPIO_Port GPIOC
#define ULN2003_IN2_Pin GPIO_PIN_3
#define ULN2003_CONFIG_IN2_GPIO_PIN(value) PCout(3) = !!(value)

#define ULN2003_IN3_GPIO_Port GPIOC
#define ULN2003_IN3_Pin GPIO_PIN_4
#define ULN2003_CONFIG_IN3_GPIO_PIN(value) PCout(4) = !!(value)

#define ULN2003_IN4_GPIO_Port GPIOC
#define ULN2003_IN4_Pin GPIO_PIN_5
#define ULN2003_CONFIG_IN4_GPIO_PIN(value) PCout(5) = !!(value)

#define ABS_DEC(x, y)  (uint16_t)((x)>(y)?(x)-(y):(y)-(x))

extern void ULN2003_Init(void);
extern void Motor_Rotate_in_count(uint8_t direction, uint16_t count, uint8_t way, uint32_t delay_time);
extern void Motor_Rotate_in_angle(uint8_t direction, uint16_t angle, uint32_t speed, uint8_t algr);
extern void Motor_Stop(void);

#endif /* __ULN2003_H__; */

