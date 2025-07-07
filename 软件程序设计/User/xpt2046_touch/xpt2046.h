#ifndef __XPT2046_TOUCH_H__
#define __XPT2046_TOUCH_H__

#include "main.h"
#include "ili9341.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

/**
 * 
 * 编写这套代码需要注意
 * 
 * 触摸屏芯片(XPT2046)没有 X,Y 方向之分, 每次测量都是在读取(ADC转换) 宽,高 方向上的电压值
 * LCD芯片(ILI9341)会区分 X,Y 方向, 反而没有 宽,高 方向一说
 * 所以编写 XPT2046 的 API 时，很重要的一点就是要通过 stm32 实现
 * 触摸芯片读取到的 宽,高 方向上电压值(RAW) 与 ILI9341 芯片 X,Y 值之间的转换，
 * 这样才能够继续往后达成各种 触摸屏,LCD 交互功能,
 * 比如 LCD 上显示一个按钮, 触摸芯片负责读取用户的触摸点, 然后 stm32 判断该点是否属于该按钮,
 * 或者用户触摸然后 stm32 在 LCD 进行显示，也就是触摸画板功能
 * 
 */

// Warning! Use SPI bus with < 1.3 Mbit speed, better ~650 Kbit to be save.
#define XPT2046_TOUCH_SPI_PORT hspi1
extern SPI_HandleTypeDef XPT2046_TOUCH_SPI_PORT;

// variable XPT2046_Pending show whether Touch IRQ pin is active low
extern uint8_t XPT2046_Pending;

// 常规法误差比较大，非常规做法更加精准
#undef XPT2046_TOUCH_CALIBRATE_NORMAL

#ifdef XPT2046_TOUCH_CALIBRATE_NORMAL
#define XPT2046_TOUCH_MIN_RAW_WIDTH 0
#define XPT2046_TOUCH_MAX_RAW_WIDTH 4096
#define XPT2046_TOUCH_MIN_RAW_HEIGHT 0
#define XPT2046_TOUCH_MAX_RAW_HEIGHT 4096
#else
#define XPT2046_TOUCH_MIN_RAW_WIDTH 1500
#define XPT2046_TOUCH_MAX_RAW_WIDTH 31000
#define XPT2046_TOUCH_MIN_RAW_HEIGHT 3276
#define XPT2046_TOUCH_MAX_RAW_HEIGHT 30110
#endif

#define XPT2046_DebuttonTime    10          // 触摸屏延时消抖时间(ms)
#define XPT2046_LongPressTime   1000        // 触摸屏触发长按时间(ms)

typedef enum {
    XPT2046_Release_Warning,
    XPT2046_Released,
    XPT2046_Press_Warning,
    XPT2046_Pressed,
    XPT2046_LongPress
} XPT2046_TouchState;

/* Button Type */
typedef struct {
    uint16_t x, y;
} XPT2046_Touch_Pos_Typedef;

typedef struct XPT2046_List_t{
    struct XPT2046_List_t *prev, *next;
    uint32_t value;
} XPT2046_List_t;

typedef PROJ_RET_Typedef (*XPT2046_Button_Callback_Typedef)(uint32_t);
typedef struct {
    XPT2046_List_t *list_node;
    
    uint16_t x, y, w, h;
    uint32_t value;     // 键值
    XPT2046_Button_Callback_Typedef press, release;
} XPT2046_Buuton_TypeDef;
typedef XPT2046_Buuton_TypeDef *XPT2046_Buuton_Handler;

#define XPT2046_BUTTON_DEBUG_ON 0

// init xpt2046
void XPT2046_Init(void);
// Callibrate the touchscreen manually
bool XPT2046_TouchManualCalibrate(void);
void XPT2046_UpdateCalibrateParam(void);
// get coordinates of the pressed point
bool XPT2046_TouchGetCoordinates(XPT2046_Touch_Pos_Typedef *pos);
// detect the touch state
XPT2046_TouchState XPT2046_TouchDetect(uint8_t use_new_status, XPT2046_TouchState new_status);
// xpt2046 interrupt handler
extern void XPT2046_touch_handler(void);

// // Button API
// XPT2046_BasicButtonHandle XPT2046_CreateBasicButton(uint16_t x, uint16_t y, 
//         uint16_t w, uint16_t h, uint16_t color);
// void XPT2046_DeleteBasicButton(XPT2046_BasicButtonHandle bt);

// Button API
extern PROJ_RET_Typedef XPT2046_Create_Button(XPT2046_Buuton_Handler handler);
extern void XPT2046_Button_Delete(XPT2046_Buuton_Handler handler);
extern XPT2046_Buuton_Handler XPT2046_Match_Button(XPT2046_Touch_Pos_Typedef *pos);
extern void XPT2046_Button_Detect(void);

#endif // __XPT20461_TOUCH_H__;

