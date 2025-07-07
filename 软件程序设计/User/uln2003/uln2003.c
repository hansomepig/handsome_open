#include "uln2003.h"
#include <stdio.h>

// 电机导通相序
static uint8_t phase_4_step[] = {0x01, 0x02, 0x04, 0x08}; // 4相四拍
static uint8_t phase_8_step[] = {0x01, 0x03, 0x02, 0x6, 0x04, 0x0c, 0x08, 0x09}; // 4相八拍

#define ULN2003_SET_PIN(set_value) \
    do { \
        ULN2003_CONFIG_IN1_GPIO_PIN(set_value & 0x01); \
        ULN2003_CONFIG_IN2_GPIO_PIN(set_value & 0x02); \
        ULN2003_CONFIG_IN3_GPIO_PIN(set_value & 0x04); \
        ULN2003_CONFIG_IN4_GPIO_PIN(set_value & 0x08); \
    } while(0)


/**
 * @brief 电机初始化
 */
void ULN2003_Init(void)
{
    Motor_Stop();
}



/**
 * @brief 电机转动, 通过脉冲数控制
 * @param direction 方向
 *        0:顺时针
 *        1:逆时针
 * @param count 发送脉冲数
 * @param way 转动方式
 *        4:四相四拍, 速度快, 精度低
 *        8:四相八拍, 速度慢, 精度高
 * @param delay_time 延迟时间, 单位:ms, 至少为 1ms
 * @note  每发送 512 个周期的脉冲, 4相5线步进电机(28BYJ-48)就旋转一圈
 *        这里的周期是与节拍相关的, 如果是 4 拍, 则 count=512*4 就转一周(脉冲周期为4)
 *        如果是 8 拍, 则 count=512*8 就转一周(脉冲周期为8)
 */
void Motor_Rotate_in_count(uint8_t direction, uint16_t count, 
            uint8_t way, uint32_t delay_time)
{
    uint8_t i, beat;;
    uint8_t leave_cnt;
    uint8_t *phase_step;

    if(way == 4) {      // 四相四拍
        beat = 4;
        phase_step = phase_4_step;
        leave_cnt = count % 4;
        count /= 4;
    } else {          // 四相八拍
        beat = 8;
        phase_step = phase_8_step;
        leave_cnt = count % 8;
        count /= 8;
    }

    if(delay_time == 0)
        delay_time = 1;

    if(direction == 0) {    // 顺时针

        while(count--) {
            for(i=0; i<beat; i++)
            {
                ULN2003_SET_PIN(phase_step[i]);
                HAL_Delay(delay_time);
            }
        }

        for(i=0; i<leave_cnt; i++)
        {
            ULN2003_SET_PIN(phase_step[i]);
            HAL_Delay(delay_time);
        }

    } else {              // 逆时针
        
        while(count--) {
            i = beat;
            while(i--) {
                ULN2003_SET_PIN(phase_step[i]);
                HAL_Delay(delay_time);
            }
        }

        i = leave_cnt;
        while(i--) {
            ULN2003_SET_PIN(phase_step[i]);
            HAL_Delay(delay_time);
        }

    }
}


/**
 * @brief 电机转动, 通过角度控制
 * @param direction 方向
 *        0:顺时针
 *        1:逆时针
 * @param angle 转动角度, 单位:度
 * @param speed 转动速度, 单位:ms/round
 * @param algr  近似方法
 *              0:向下取整
 *              1:四舍五入
 *              2:向上取整
 */
void Motor_Rotate_in_angle(uint8_t direction, uint16_t angle, uint32_t speed, uint8_t algr)
{
    uint16_t count;
    uint16_t delay_time;
    
    if(algr == 0) {     // 向下取整
        count = (uint32_t)2048 * (uint32_t)angle / 360;
        delay_time = speed / 2048;      // 512*4
    } else if(algr == 1) {      // 四舍五入
        count = ((uint32_t)2048 * (uint32_t)angle + 180) / 360;
        delay_time = (speed + 1024) / 2048;
    } else {            // 向上取整
        count = ((uint32_t)2048 * (uint32_t)angle + 359) / 360;
        delay_time = (speed + 2047) / 2048;
    }

    Motor_Rotate_in_count(direction, count, 4, delay_time);

}

/**
 * @brief 停止电机转动
 */
void Motor_Stop(void)
{
    ULN2003_SET_PIN(0x00);
}

