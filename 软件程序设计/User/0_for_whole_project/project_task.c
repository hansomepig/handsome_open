#include "for_whole_project.h"


/* 开锁任务 */
#define   OpenLock_Task_PRIO            5
#define   OpenLock_Task_STK_SIZE        512
static void OpenLock_Task(void* parameter);
SemaphoreHandle_t OpenLock_Semaphore;       // 开锁信号量

/* 按钮事件处理任务 */
#define   Button_Task_PRIO              2
#define   Button_Task_STK_SIZE          512
static void Button_Task(void* parameter);
SemaphoreHandle_t ButtonEvent_Semaphore;       // 按钮事件信号量

/* 指纹识别事件处理任务 */
#define   Fingerprint_Task_PRIO         2
#define   Fingerprint_Task_STK_SIZE     512
static void Fingerprint_Task(void* parameter);
SemaphoreHandle_t Fingerprint_Semaphore;       // 指纹事件信号量

/* RFID事件处理任务 */
#define   RFID_Task_PRIO                2
#define   RFID_Task_STK_SIZE            512
static void RFID_Task(void* parameter);
SemaphoreHandle_t RFID_Semaphore;              // RFID事件信号量

/* 蓝牙处理任务 */
#define   BT_Task_PRIO                2
#define   BT_Task_STK_SIZE            512
static void BT_Task(void* parameter);
SemaphoreHandle_t BT_Semaphore;              // 蓝牙事件信号量

/**
 * @brief  创建任务函数
 */
BaseType_t proj_create_task(void)
{
    BaseType_t xReturn = pdPASS;


/* 开锁任务 */
    xReturn = xTaskCreate((TaskFunction_t )OpenLock_Task,
                (const char*    )"OpenLock_Task",
                (uint16_t       )OpenLock_Task_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )OpenLock_Task_PRIO,
                (TaskHandle_t*  )NULL);
    if(pdPASS == xReturn) {
        //printf("创建 OpenLock_Task 任务成功\n");
    } else {
        //printf("创建 OpenLock_Task 任务失败!\r\n");
        return pdFAIL;
    }

    // 开锁用信号量
    OpenLock_Semaphore = xSemaphoreCreateBinary();

/* 按钮事件处理任务 */
    xReturn = xTaskCreate((TaskFunction_t )Button_Task,
                (const char*    )"Button_Task",
                (uint16_t       )Button_Task_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )Button_Task_PRIO,
                (TaskHandle_t*  )NULL);
    if(pdPASS == xReturn) {
        //printf("创建 Button_Task 任务成功\n");
    } else {
        //printf("创建 Button_Task 任务失败!\r\n");
        return pdFAIL;
    }

    // 按钮事件信号量
    ButtonEvent_Semaphore = xSemaphoreCreateBinary();

/* 指纹模组事件处理任务 */
    xReturn = xTaskCreate((TaskFunction_t )Fingerprint_Task,
                (const char*    )"Fingerprint_Task",
                (uint16_t       )Fingerprint_Task_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )Fingerprint_Task_PRIO,
                (TaskHandle_t*  )NULL);
    if(pdPASS == xReturn) {
        //printf("创建 Fingerprint_Task 任务成功\n");
    } else {
        //printf("创建 Fingerprint_Task 任务失败!\r\n");
        return pdFAIL;
    }

    // 指纹事件信号量
    Fingerprint_Semaphore = xSemaphoreCreateBinary();

/* RFID事件处理任务 */
    xReturn = xTaskCreate((TaskFunction_t )RFID_Task,
                (const char*    )"RFID_Task",
                (uint16_t       )RFID_Task_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )RFID_Task_PRIO,
                (TaskHandle_t*  )NULL);
    if(pdPASS == xReturn) {
        //printf("创建 RFID_Task 任务成功\n");
    } else {
        //printf("创建 RFID_Task 任务失败!\r\n");
        return pdFAIL;
    }

    // RFID事件信号量
    RFID_Semaphore = xSemaphoreCreateBinary();

/* 蓝牙处理任务 */
    xReturn = xTaskCreate((TaskFunction_t )BT_Task,
                (const char*    )"BT_Task",
                (uint16_t       )BT_Task_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )BT_Task_PRIO,
                (TaskHandle_t*  )NULL);
    if(pdPASS == xReturn) {
        //printf("创建 BT_Task 任务成功\n");
    } else {
        //printf("创建 BT_Task 任务失败!\r\n");
        return pdFAIL;
    }

    // 蓝牙事件信号量
    BT_Semaphore = xSemaphoreCreateBinary();

    return pdPASS;
}


/* --------------------------------任务函数-------------------------------------- */
static char mesg[] = "Hello user 0!";

static void OpenLock_Task(void* parameter)
{
    while(1)
    {
        // 等待开锁信号
        xSemaphoreTake(OpenLock_Semaphore, portMAX_DELAY);

        ILI9341_WriteLine(7, 5+18, "Opening...", ILI9341_RED, ILI9341_WHITE);
        HAL_Delay(10);
        // 开锁(顺时针转90度)
        Motor_Rotate_in_angle(1, 90, 0, 1);
        LED_GREEN_ON;
        ILI9341_WriteLine(7, 5+18, "Opened!!!", ILI9341_RED, ILI9341_WHITE);
        HAL_Delay(10);

        // 等待用户进入
        // vTaskDelay(1000);
        HAL_Delay(1000);

        ILI9341_WriteLine(7, 5+18, "Closing...", ILI9341_RED, ILI9341_WHITE);
        HAL_Delay(10);
        // 关锁(逆时针转90度)
        Motor_Rotate_in_angle(0, 90, 0, 1);
        LED_GREEN_OFF;
        ILI9341_WriteLine(7, 5+18, "Closed!!!", ILI9341_RED, ILI9341_WHITE);
        HAL_Delay(10);
    }
}


// 按钮事件处理任务
static void Button_Task(void* parameter)
{
    // 绘制按钮键盘面板
    ILI9341_FillScreen(ILI9341_WHITE);
    proj_show_keyboard();

	while(1)
    {
        // 使能触摸屏中断
        __HAL_GPIO_EXTI_CLEAR_IT(XPT2046_TOUCH_IRQ_Pin);
        HAL_NVIC_EnableIRQ(XPT2046_TOUCH_IRQ_EXTI_IRQn);

        // 等待按钮信号量
        xSemaphoreTake(ButtonEvent_Semaphore, portMAX_DELAY);

        HAL_NVIC_DisableIRQ(XPT2046_TOUCH_IRQ_EXTI_IRQn);
        XPT2046_TouchDetect(1, XPT2046_Released);
        __HAL_GPIO_EXTI_CLEAR_IT(XPT2046_TOUCH_IRQ_Pin);
        HAL_NVIC_EnableIRQ(XPT2046_TOUCH_IRQ_EXTI_IRQn);

        // 延时消抖
        xSemaphoreTake(ButtonEvent_Semaphore, XPT2046_DebuttonTime+1);

        HAL_NVIC_DisableIRQ(XPT2046_TOUCH_IRQ_EXTI_IRQn);
        // 处理按钮事件
        XPT2046_Button_Detect();
    }
}


// 指纹识别事件处理任务
static void Fingerprint_Task(void* parameter)
{
    FP_Ret_Type ret;

    // 手动控制指纹模块的led灯
    zw101_led_switch(1);

    while(1)
    {
        // 重新使能触指纹模组中断, 来等待下一次指纹事件
        __HAL_GPIO_EXTI_CLEAR_IT(ZW101_PendIRQ_Pin);
        HAL_NVIC_EnableIRQ(ZW101_PendIRQ_EXTI_IRQn);

        // 等待指纹事件信号量
        xSemaphoreTake(Fingerprint_Semaphore, portMAX_DELAY);
        // 关闭指纹模组中断
        HAL_NVIC_DisableIRQ(ZW101_PendIRQ_EXTI_IRQn);

        // 唤醒指纹模组
        zw101_wakeup();
        while( zw101_shand_shake() != 0x55 );

        // 清除屏幕回显区
        proj_clear_echo_area();

        // 等待用户将手指松开
        ILI9341_WriteLine(7, 5, "Remove your finger", ILI9341_RED, ILI9341_WHITE);
        printf("Remove your finger\n");
        zw101_GetIRQStatue(1, ZW101_PendIRQ_PRESSED, 1, HAL_GetTick());
        while(zw101_GetIRQStatue(0, 0, 0, 0) != ZW101_PendIRQ_RELEASED);

        // 重新打开触指纹模组中断
        __HAL_GPIO_EXTI_CLEAR_IT(ZW101_PendIRQ_Pin);
        HAL_NVIC_EnableIRQ(ZW101_PendIRQ_EXTI_IRQn);

        // 等待用户将手指放到指纹识别器上, 限时 2s
        xSemaphoreTake(Fingerprint_Semaphore, 2000);

        // 关闭指纹模组中断
        HAL_NVIC_DisableIRQ(ZW101_PendIRQ_EXTI_IRQn);

        if(zw101_GetIRQStatue(1, ZW101_PendIRQ_RELEASED, 0, 0) != ZW101_PendIRQ_PRESSE_WARNING)
        {
            // 将指纹模组LED灯的颜色设置为蓝色
            zw101_led_set_rgb(ZW101_LED_BLUE);
            // 进入睡眠
            while(zw101_sleep() != FP_RET_OK);

            continue;
        }

        // 比对指纹
        ret = zw101_check_FP(0, 2);

        if(ret == FP_RET_OK) {
            // 指纹识别成功, 亮绿灯
            zw101_led_set_rgb(ZW101_LED_GREEN);
            // 开锁
            xSemaphoreGive(OpenLock_Semaphore);
        } else {
            // 指纹识别失败, 亮红灯
            zw101_led_set_rgb(ZW101_LED_RED);
            // vTaskDelay(1000);
            HAL_Delay(1000);
        }

        // 进入睡眠
        while(zw101_sleep() != FP_RET_OK);
        // 将指纹模组LED灯的颜色设置为蓝色
        zw101_led_set_rgb(ZW101_LED_BLUE);
    }
}


#define RC522RFID_ERROR_Handler() \
    vTaskDelay(1000); \
    continue; \

void RFID_Task(void* parameter)
{
// RFID寻卡次数
#define RFID_REQUEST_TIMES 3
    uint8_t ucArray_ID [ 4 ];    /* 先后存放IC卡的类型和UID(IC卡序列号) */
    uint8_t readValue[16];       /* 存放读取的数据 */


    while(1)
    {
        // 重新使能RFID按键触发中断, 来等待下一次RFID触发事件
        __HAL_GPIO_EXTI_CLEAR_IT(RC522_IRQn);
        HAL_NVIC_EnableIRQ(RC522_IRQn);

        // 等待RFID事件信号量
        xSemaphoreTake(RFID_Semaphore, portMAX_DELAY);

        // 延时消抖 10 ms
        xSemaphoreTake(RFID_Semaphore, 10);

        // 关闭RFID的按键触发中断
        HAL_NVIC_DisableIRQ(RC522_IRQn);

        // 确认一下按键是否真的被按下
        if(HAL_GPIO_ReadPin(RC522_GPIO_IRQ_PORT, RC522_GPIO_IRQ_PIN) != GPIO_PIN_SET) {
            // 按键未被按下, 直接跳出本次循环
            continue;
        }

        // 确认按键已经按下, 先清除屏幕回显区
        proj_clear_echo_area();

        // 开始处理RFID事件
        uint8_t i;
        for(i=0; i<RFID_REQUEST_TIMES; i++) {
            // 寻卡（天线内的所有卡）, 如果第一次失败就再次寻卡, 第二次还失败那就真的败了
            if( (PcdRequest(PICC_REQALL, ucArray_ID) != MI_OK) &&
                (PcdRequest(PICC_REQALL, ucArray_ID) != MI_OK) ) {
                // 寻卡失败, 延时 1s 后继续寻卡
                printf("寻卡失败\n");
                RC522RFID_ERROR_Handler();
            }

            // 防冲撞
            if( PcdAnticoll(ucArray_ID) != MI_OK ) {
                printf("防冲撞失败\n");
                RC522RFID_ERROR_Handler();
            };

            // 选卡
            if( PcdSelect(ucArray_ID) != MI_OK ) {
                printf("选卡失败\n");
                RC522RFID_ERROR_Handler();
            };

            // 校验密码
            if( PcdAuthState(PICC_AUTHENT1A, ucBlock, KeyValue_A, ucArray_ID) != MI_OK ) {
                printf("密码错误\n");
                RC522RFID_ERROR_Handler();
            };

            // 读取卡内数据
            if( PcdRead(ucBlock, (uint8_t *)readValue) != MI_OK ) {
                printf("读取卡内数据失败\n");
                RC522RFID_ERROR_Handler();
            };

            // 休眠卡片
            PcdHalt();

            if( used_user_rfid & ( 1 << (readValue[0]-1) ) ) {
                // 显示读取到的数据
                mesg[sizeof(mesg)-3] = readValue[0] + '0';
                ILI9341_WriteLine(7, 5, mesg, ILI9341_RED, ILI9341_WHITE);
                printf("Hello, User %d\n", readValue[0]);

                // 密码校验成功, 开锁
                xSemaphoreGive(OpenLock_Semaphore);
            } else {
                ILI9341_WriteLine(7, 5, "Not valid user", ILI9341_RED, ILI9341_WHITE);
                printf("Not valid user\n");
                vTaskDelay(1000);
            }

            printf("读取到的数据为:\n");
            for(int i=0; i<16; i++) {
                printf("%d ", readValue[i]);
            }
            printf("\n");

            break;
        }

        if(i == RFID_REQUEST_TIMES) {
            // 连续三次寻卡失败, 说明卡片已经被移除, 延时 1s 后继续寻卡
            ILI9341_WriteLine(7, 5, "Check card failed", ILI9341_RED, ILI9341_WHITE);
            printf("check card failed\n");
        }

    }
}

static void BT_Task(void* parameter)
{
    uint8_t *recv_buffer, cnt;

    while(1)
    {
        // 使能蓝牙模块的中断
        __HAL_GPIO_EXTI_CLEAR_IT(BT_IRQn);
        HAL_NVIC_EnableIRQ(BT_IRQn);

        HAL_UARTEx_ReceiveToIdle_IT(&BT04_E_UART_PORT, 
            bt_recv_buffer.bt_receive_buffer[bt_recv_buffer.bt_recv_buffer_pos], 
            BT04_E_RECV_BUFFER_SIZE);

        // 等待蓝牙事件信号量
        xSemaphoreTake(BT_Semaphore, portMAX_DELAY);

        // 关闭蓝牙模块的中断
        HAL_NVIC_DisableIRQ(BT_IRQn);

        recv_buffer = bt_recv_data(0);
        if( !recv_buffer ) {    // 接收出错
            printf("error happened in bt_recv_data\n");
            continue;
        }

        // 成功接收到数据
        printf("from bt, get: %s\n", recv_buffer);

        for(cnt=0; recv_buffer[cnt]!='\0'; cnt++) {
            if(cnt >= PROJ_PASSWD_LEN)
                break;

            if((recv_buffer[cnt] >= '0') && (recv_buffer[cnt] <= '9')) {
                recv_buffer[cnt] = recv_buffer[cnt] - '0';
            } else {
                cnt = 0;
                break;
            }
        }
        
        // 清空屏幕回显区
        proj_clear_echo_area();

        if(cnt == 0) {      // 数据格式有误, 不全为数字
            ILI9341_WriteLine(7, 5, "Data format error", ILI9341_RED, ILI9341_WHITE);            
            printf("data format error\n");
            bt_clean_recv_buffer();
            continue;
        }

        // 比对密码
        if(proj_check_passwd(recv_buffer, cnt) != PROJ_OK) {        // 数据传输错误
            printf("请查看硬件配置\r\n");
            bt_clean_recv_buffer();
            continue;
        }

        if(recv_buffer[PROJ_PASSWD_LEN] != 2) {        // 密码正确, 开锁
            xSemaphoreGive(OpenLock_Semaphore);
        }

        bt_clean_recv_buffer();
    }
}

