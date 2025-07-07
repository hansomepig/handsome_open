/* vim: set ai et ts=4 sw=4: */

#include "stm32f1xx_hal.h"
#include "xpt2046.h"
#include "for_whole_project.h"

#define READ_X 0xD0
#define READ_Y 0x90

uint8_t XPT2046_Pending = 0;

/**
 * LCD x,y 方向的最终校准参数, 不是 x,y 方向的最值, 
 * 会根据 ILI9341 选择的 x,y 模式(是否调换、翻转)进行调整,
 */
uint16_t XPT2046_MaxRefRAW_X, XPT2046_MaxRefRAW_Y;
uint16_t XPT2046_MinRefRAW_X, XPT2046_MinRefRAW_Y;

static void XPT2046_TouchSelect(void) {
    HAL_GPIO_WritePin(XPT2046_TOUCH_CS_GPIO_Port, XPT2046_TOUCH_CS_Pin, GPIO_PIN_RESET);
}

static void XPT2046_TouchUnselect(void) {
    HAL_GPIO_WritePin(XPT2046_TOUCH_CS_GPIO_Port, XPT2046_TOUCH_CS_Pin, GPIO_PIN_SET);
}

static bool XPT2046_TouchPressed(void) {
    return HAL_GPIO_ReadPin(XPT2046_TOUCH_IRQ_GPIO_Port, XPT2046_TOUCH_IRQ_Pin) == GPIO_PIN_RESET;
}

/**
 * 获取触摸芯片 XPT2046 的原始测量电压值
 */
static bool XPT2046_TouchGetRawCoordinates(uint16_t* width, uint16_t* height, uint16_t times)
{
    static const uint8_t cmd_read_x[] = { READ_X };
    static const uint8_t cmd_read_y[] = { READ_Y };
    static const uint8_t zeroes_tx[] = { 0x00, 0x00 };

    uint8_t width_raw[2], height_raw[2];
    uint8_t i;
    uint32_t sum_width = 0, sum_height = 0;

    XPT2046_TouchSelect();

    for(i = 0; i < times; i++) {
        if( XPT2046_TouchPressed() == false )
            break;

        HAL_SPI_Transmit(&XPT2046_TOUCH_SPI_PORT, (uint8_t*)cmd_read_y, sizeof(cmd_read_y), HAL_MAX_DELAY);
        HAL_SPI_TransmitReceive(&XPT2046_TOUCH_SPI_PORT, (uint8_t*)zeroes_tx, height_raw, sizeof(height_raw), HAL_MAX_DELAY);

        HAL_SPI_Transmit(&XPT2046_TOUCH_SPI_PORT, (uint8_t*)cmd_read_x, sizeof(cmd_read_x), HAL_MAX_DELAY);
        HAL_SPI_TransmitReceive(&XPT2046_TOUCH_SPI_PORT, (uint8_t*)zeroes_tx, width_raw, sizeof(width_raw), HAL_MAX_DELAY);

#ifdef XPT2046_TOUCH_CALIBRATE_NORMAL
        sum_width += ( ( ((uint16_t)width_raw[0]) << 8 ) | ( (uint16_t)width_raw[1] ) ) >> 3;
        sum_height += ( ( ((uint16_t)height_raw[0]) << 8 ) | ( (uint16_t)height_raw[1] ) ) >> 3;
#else
        sum_width += ( (((uint16_t)width_raw[0]) << 8) | (((uint16_t)width_raw[1])) >> 3 );
        sum_height += ( (((uint16_t)height_raw[0]) << 8) | (((uint16_t)height_raw[1])) >> 3 );
#endif
    }

    XPT2046_TouchUnselect();

    if(i < times)
        return false;

    *width = (sum_width / times);
    *height = (sum_height / times);

    return true;
}


void XPT2046_Init(void)
{
    // 关闭片选
    XPT2046_TouchUnselect();
    // 更新校准参数
    XPT2046_UpdateCalibrateParam();
}


/** 使用 4 点校准法进行手动校准, 这个 API 将能够获得更准确的原生测量最值
 * 
 * 原理如下所示
 * 
 * ------------------------------ y_max
 * |                            |
 * |       SCALE_WIDTH / 2      |
 * |      |              |      |
 * |   -- +              +      | 
 * |                            |
 * |                            |
 * |                            |
 * |                            |
 * |  SCALE_HEIGHT / 2          |
 * |                            |
 * |                            |
 * |      (tw, th)              |
 * |        o                   |
 * |   -- +              +      | 
 * |   (lx, ly)                 |
 * |                            |
 * |                            |
 * ------------------------------ y_min
 * max_w                      min_w
 * 
 * ||||||||||||||||||||||||||||||
 * 
 * 
 *   -- (tw2 - tw1) = 2*(tw1 - min_w) --   得    -- min_w = tw1 - (tw2 - tw1)/2
 * --|                                 | ------> |
 *   -- (tw2 - tw1) = 2*(max_w - tw2) --         -- max_w = tw2 + (tw2 - tw1)/2
 * 
 * 求 h_min, h_max 得方法同理
 * 
 * 
 * 得到得 w_min, w_max, h_min, h_max 就是 XPT2046 触摸芯片读取到的电压预测最值
 * 
 * 手动校准可能会经常报错，这里给出一点小技巧，
 * 对于 x,y 值较小处往偏大处点，对于 x,y 值较大处往偏小处点
 * 
 */

typedef struct {
    uint16_t max_width, max_height;
    uint16_t min_width, min_height;
} XPT2046_4PointsCalibrationParam;

static XPT2046_4PointsCalibrationParam CalibrateResultParam = {0};
static bool XPT2046_TouchCalibrated = false;

/* 手动校准触摸屏与 LCD */
bool XPT2046_TouchManualCalibrate(void)
{
    uint16_t times = 16;
    // lx,ly means lcd pixels on x,y orientation
    // tw,th means touch raw data on width,height orientation
    uint32_t lx[4], ly[4], tw[4], th[4];
    uint8_t scanmode = ILI9341_ScanMode;

    XPT2046_TouchCalibrated = false;

    // 使用模式 2 进行校准
    /**
     * 所谓的校准，其实指示获取宽、高方向上的原生测量最值，因此选择哪种模式其实都是可行的，
     * 但是下面的代码是按照模式 2 进行编写的，因此这里要提前改成模式 2，校准完后再恢复回来
     */
    ILI9341_SetOrientation(2);

    lx[0] = lx[1] = ILI9341_X_Scale / 4;
    lx[2] = lx[3] = ILI9341_X_Scale * 3 / 4;

    ly[0] = ly[3] = ILI9341_Y_Scale * 3 / 4;
    ly[1] = ly[2] = ILI9341_Y_Scale / 4;

    Set_CurrentFont(&Font_7x10);
    ILI9341_FillScreen(ILI9341_BLACK);

    for(uint8_t i=0; i<4; i++) {
        ILI9341_DrawCross(lx[i], ly[i], 5, ILI9341_YELLOW);
        while(XPT2046_TouchDetect(0, XPT2046_Released) != XPT2046_Pressed);
        if( (false == XPT2046_TouchGetRawCoordinates((uint16_t *)&tw[i], (uint16_t *)&th[i], times)) )
        {
            ILI9341_WriteString(ILI9341_X_Scale / 2 - 63, ILI9341_Y_Scale / 2
                , "Calibrate Failed!", ILI9341_RED, ILI9341_BLACK);
            return false;
        }
        printf("tw = %d, th = %d\r\n", tw[i], th[i]);
        while(XPT2046_TouchDetect(0, XPT2046_Released) != XPT2046_Released);
        ILI9341_DrawCross(lx[i], ly[i], 5, ILI9341_BLACK);
    }

    uint32_t max_x1, max_x2, min_x1, min_x2;
    uint32_t max_y1, max_y2, min_y1, min_y2;

    max_x1 = (3 * tw[3] - tw[0]) / 2;
    min_x1 = (3 * tw[0] - tw[3]) / 2;
    max_x2 = (3 * tw[2] - tw[1]) / 2;
    min_x2 = (3 * tw[1] - tw[2]) / 2;

    max_y1 = (3 * th[0] - th[1]) / 2;
    min_y1 = (3 * th[1] - th[0]) / 2;
    max_y2 = (3 * th[3] - th[2]) / 2;
    min_y2 = (3 * th[2] - th[3]) / 2;
    
    // 由于 min_x, min_y 为 32 位无符号整数, 所以当它们小于零时符号位(最高位)将被置位
    if( ( max_x1|min_x1|max_x2|min_x2|max_y1|min_y1|max_y2|min_y2 ) & 0x80000000 ) {
        ILI9341_WriteString(ILI9341_X_Scale / 2 - 63, ILI9341_Y_Scale / 2
                , "Calibrate Failed!", ILI9341_RED, ILI9341_BLACK);
        return false;
    }

    CalibrateResultParam.max_width = (max_x1 + max_x2) / 2;
    CalibrateResultParam.min_width = (min_x1 + min_x2) / 2;
    CalibrateResultParam.max_height = (max_y1 + max_y2) / 2;
    CalibrateResultParam.min_height = (min_y1 + min_y2) / 2;

#if 1
    printf(
        "max_width = %d, min_width = %d\r\n"
        "max_height = %d, min_height = %d\r\n",
        CalibrateResultParam.max_width,
        CalibrateResultParam.min_width,
        CalibrateResultParam.max_height,
        CalibrateResultParam.min_height);
#endif

    ILI9341_WriteString(ILI9341_X_Scale / 2 - 63, ILI9341_Y_Scale / 2, 
                "Calibrate Successd", ILI9341_GREEN, ILI9341_BLACK);

    // 恢复为原来的 x,y 方向
    ILI9341_SetOrientation(scanmode);

    // 更新校准参数
    XPT2046_UpdateCalibrateParam();

    XPT2046_TouchCalibrated = true;

    return true;
}


/**
 * @brief  更新校准参数
 * @param  None
 * @retval None
 * @note   每次调用 LCD ILI9341 的 ILI9341_SetOrientation 函数后都需要手动调用一次此函数
 */
void XPT2046_UpdateCalibrateParam(void)
{
    uint64_t max_raw_width, max_raw_height;
    uint64_t min_raw_width, min_raw_height;
    uint8_t scanmode = ILI9341_ScanMode << 5;

    if(XPT2046_TouchCalibrated == true) {
        
        max_raw_width = CalibrateResultParam.max_width;
        min_raw_width = CalibrateResultParam.min_width;
        max_raw_height = CalibrateResultParam.max_height;
        min_raw_height = CalibrateResultParam.min_height;
    } else {
        min_raw_width = XPT2046_TOUCH_MIN_RAW_WIDTH;
        max_raw_width = XPT2046_TOUCH_MAX_RAW_WIDTH;
        min_raw_height = XPT2046_TOUCH_MIN_RAW_HEIGHT;
        max_raw_height = XPT2046_TOUCH_MAX_RAW_HEIGHT;
    }

    if( (scanmode & ILI9341_MADCTL_MV) == 0 ) {   // 不调换 x,y 方向
        if( scanmode & ILI9341_MADCTL_MY ) { // y 方向翻转
            XPT2046_MinRefRAW_Y = max_raw_height;
            XPT2046_MaxRefRAW_Y = min_raw_height;
        } else {
            XPT2046_MinRefRAW_Y = min_raw_height;
            XPT2046_MaxRefRAW_Y = max_raw_height;
        }

        if( scanmode & ILI9341_MADCTL_MX ) { // x 方向翻转
            XPT2046_MinRefRAW_X = min_raw_width;
            XPT2046_MaxRefRAW_X = max_raw_width;
        } else {
            XPT2046_MinRefRAW_X = max_raw_width;
            XPT2046_MaxRefRAW_X = min_raw_width;
        }
    } else {                               // 调换 x,y 方向
        if( (scanmode & ILI9341_MADCTL_MY) ) { // y 方向翻转
            XPT2046_MinRefRAW_Y = max_raw_width;
            XPT2046_MaxRefRAW_Y = min_raw_width;
        } else {
            XPT2046_MinRefRAW_Y = min_raw_width;
            XPT2046_MaxRefRAW_Y = max_raw_width;
        }

        if( (scanmode & ILI9341_MADCTL_MX) ) { // x 方向翻转
            XPT2046_MinRefRAW_X = min_raw_height;
            XPT2046_MaxRefRAW_X = max_raw_height;
        } else {
            XPT2046_MinRefRAW_X = max_raw_height;
            XPT2046_MaxRefRAW_X = min_raw_height;
        }
    }
}


// 获取触摸点坐标, 仅当获取成功才返回true
bool XPT2046_TouchGetCoordinates(XPT2046_Touch_Pos_Typedef* pos) {
    uint16_t raw_width, raw_height;
    uint16_t raw_x, raw_y;
    uint16_t times = 16;

    if( XPT2046_TouchGetRawCoordinates(&raw_width, &raw_height, times) == false )
        return false;
    
    if( ILI9341_ScanMode & 0x01 ) {     // x,y 轴发生了调换
        raw_x = raw_height, raw_y = raw_width;
    } else {
        raw_x = raw_width, raw_y = raw_height;
    }

    // printf("raw_x = %d, raw_y = %d\r\n", raw_x, raw_y);

    // XPT2046 raw voltage data
    uint16_t max_x, min_x;
    uint16_t max_y, min_y;

    max_x = XPT2046_MaxRefRAW_X;
    min_x = XPT2046_MinRefRAW_X;
    max_y = XPT2046_MaxRefRAW_Y;
    min_y = XPT2046_MinRefRAW_Y;

    if(max_x > min_x) {
        if(raw_x < min_x) raw_x = min_x;
        if(raw_x > max_x) raw_x = max_x;
        pos->x = (raw_x - min_x) * (ILI9341_X_Scale) / (max_x - min_x);
    } else {
        if(raw_x > min_x) raw_x = min_x;
        if(raw_x < max_x) raw_x = max_x;
        pos->x = (min_x - raw_x) * (ILI9341_X_Scale) / (min_x - max_x);
    }

    if(max_y > min_y) {
        if(raw_y < min_y) raw_y = min_y;
        if(raw_y > max_y) raw_y = max_y;
        pos->y = (raw_y - min_y) * (ILI9341_Y_Scale) / (max_y - min_y);
    } else {
        if(raw_y > min_y) raw_y = min_y;
        if(raw_y < max_y) raw_y = max_y;
        pos->y = (min_y - raw_y) * (ILI9341_Y_Scale) / (min_y - max_y);
    }

    return true;
}

/** get touchscreen's touch state in nonblock way
 * 
 * 
 * 触摸状态机如下所示:
 * 
 *                              --------
 *              --------------- | User | ----------------
 *              |               --------                |
 *    Press     |                                       |   Release
 *              V                                       V
 * -------------------------               ---------------------------
 * | XPT2046_Press_Warning | <---    ----> | XPT2046_Release_Warning |
 * -------------------------    |    |     ---------------------------
 *              |               |    |                   |
 *  Press 10ms  |        Press  |    |                   |   Release 10ms
 *              V               |    |                   V
 * -------------------------    |    |     ---------------------------
 * |    XPT2046_Pressed    |    -----+---- |     XPT2046_Released    |
 * -------------------------         |     ---------------------------
 *              |                    |
 *   Press 1s   |                    |
 *              V                    |  Release
 * -------------------------         |
 * |   XPT2046_LongPress   | ---------
 * -------------------------
 * 
 * @retval: touch status
 * @note:   this API will not block the system
 * 
 */
XPT2046_TouchState XPT2046_TouchDetect(uint8_t use_new_status, XPT2046_TouchState new_status) {
    static XPT2046_TouchState TouchResult = XPT2046_Released;
    static uint32_t LastTick;
    uint32_t CurrentTick;

    if( use_new_status == 1 ) {
        TouchResult = new_status;
    }

    if( XPT2046_TouchPressed() == true ) {
        switch (TouchResult)
        {
        case XPT2046_Press_Warning:
            // 延时消抖
            CurrentTick = HAL_GetTick();
            //HAL_Delay(XPT2046_DebuttonTime);
            if( (CurrentTick - LastTick) < XPT2046_DebuttonTime ) 
                break;

            TouchResult = XPT2046_Pressed;
            LastTick = CurrentTick;

            break;

        case XPT2046_Pressed:
            //HAL_Delay(XPT2046_LongPressTime);
            if( (HAL_GetTick() - LastTick) < XPT2046_LongPressTime ) 
                break;
            
            TouchResult = XPT2046_LongPress;
            break;

        case XPT2046_LongPress:
            break;
        
        default:
            TouchResult = XPT2046_Press_Warning;
            LastTick = HAL_GetTick();
            break;
        }
    } else {
        switch (TouchResult)
        {
        case XPT2046_Release_Warning:
            if( (HAL_GetTick() - LastTick) < XPT2046_DebuttonTime ) 
                break;
            TouchResult = XPT2046_Released;
            break;

        case XPT2046_Released:
            break;

        default:
            TouchResult = XPT2046_Release_Warning;
            LastTick = HAL_GetTick();
            break;
        }
    }

    return TouchResult;
}



#include "FreeRTOS.h"
// 当前创建了的按键数量, 也就是当前按键列表的节点数量
uint8_t XPT2046_Button_List_Size = 0;
// 按键列表头
static XPT2046_List_t XPT2046_Button_List_Head = { 
    .next = &XPT2046_Button_List_Head,
    .prev = &XPT2046_Button_List_Head,
    .value = 0
};

/**
 * @brief Create a new button
 * @param handler -> Pointer to the button handler
 * @return PROJ_OK if success, PROJ_ERROR if error
 */
PROJ_RET_Typedef XPT2046_Create_Button(XPT2046_Buuton_Handler handler)
{
    if((handler == NULL) || (handler->list_node != NULL))
        return PROJ_ERROR;

    XPT2046_List_t *list = pvPortMalloc(sizeof(XPT2046_List_t));
    if(list == NULL) {
        return PROJ_NO_SPACE;
    } else {
        list->value = (uint32_t)(void *)handler;
        handler->list_node = list;
    }

#if XPT2046_BUTTON_DEBUG_ON
    printf("handler=0x%p, list=0x%p\r\n", handler, list);
    printf("Create button, x = %d, y = %d, w = %d, h = %d\r\n", 
            handler->x, handler->y, handler->w, handler->h);
#endif

    list->next = &XPT2046_Button_List_Head;
    list->prev = XPT2046_Button_List_Head.prev;

    XPT2046_Button_List_Head.prev->next = list;
    XPT2046_Button_List_Head.prev = list;

    XPT2046_Button_List_Size++;

    return PROJ_OK;
}


/**
 * @brief Delete a button
 * @param handler -> Pointer to the button handler
 * @return PROJ_OK if success, PROJ_ERROR if error
 */
void XPT2046_Button_Delete(XPT2046_Buuton_Handler handler)
{
    if((handler == NULL) || (handler->list_node == NULL))
        return;

    XPT2046_List_t *list = handler->list_node;

    list->prev->next = list->next;
    list->next->prev = list->prev;

    XPT2046_Button_List_Size--;

    vPortFree(list);
}

XPT2046_Buuton_Handler XPT2046_Match_Button(XPT2046_Touch_Pos_Typedef *pos)
{
    XPT2046_List_t *l = XPT2046_Button_List_Head.next;
    XPT2046_Buuton_Handler b;

    // printf("pos: x = %d, y = %d\r\n", pos->x, pos->y);

    while(l != &XPT2046_Button_List_Head) {
        b = (XPT2046_Buuton_Handler)(l->value);

#if XPT2046_BUTTON_DEBUG_ON
        printf("b=0x%p, l=0x%p\r\n", b, l);
        printf("matching button: x = %d, y = %d, w = %d, h = %d\r\n", 
                b->x, b->y, b->w, b->h);
#endif

        if( (pos->x >= b->x) && (pos->x <= b->x + b->w) &&
            (pos->y >= b->y) && (pos->y <= b->y + b->h) ) {
            return b;
        }
        l = l->next;
    }

    return NULL;
}

void XPT2046_Button_Detect(void)
{
    XPT2046_Touch_Pos_Typedef pos;
    XPT2046_TouchState st;
    XPT2046_Buuton_Handler b;

    st = XPT2046_TouchDetect(0, XPT2046_Released);
    XPT2046_TouchGetCoordinates(&pos);
    b = XPT2046_Match_Button(&pos);

    if( b != NULL ) {
        if(st == XPT2046_Pressed) {        // 按下了某个按键
            if(b->press != NULL)    b->press(b->value);
        } else if(st == XPT2046_Released) {        // 释放了某个按键
            if(b->release != NULL)  b->release(b->value);
        }
    } else {
        // printf("no button match\r\n");
    }
}



void XPT2046_touch_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // 释放按钮事件信号量
    xSemaphoreGiveFromISR(ButtonEvent_Semaphore, &xHigherPriorityTaskWoken);
    
    // LED_BLUE_TOGGLE;

    // 根据返回值执行一次任务调度
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

