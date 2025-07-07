#include "for_whole_project.h"

/* 密码相关 */
uint16_t max_user_count = 32;
uint16_t useful_user_count = 0, useless_user_count = 0;     // 有效、无效用户数量

// FLASH密码
uint32_t used_user_passwd = 0, unused_user_passwd = 0;   // 已用、已删除的用户密码

// 指纹
uint32_t used_user_fp = 0, unused_user_fp = 0;    // 已用、已删除的用户指纹

// RFID
uint8_t ucBlock = 0x11;      /* 密码块 */
uint8_t KeyValue_A[6]={0xFF ,0x44, 0xFF, 0xFF, 0xFF, 0xFF};   // 密钥
uint32_t used_user_rfid = 0, unused_user_rfid = 0;    // 已用、已删除的用户RFID

// 蓝牙
uint8_t bt_finish_init = 0;   // 蓝牙初始化完成标志

FATFS projfs;
static FIL  fp;
static UINT buffer[FF_MAX_SS];

static FRESULT create_passwd_file(void);


/**
 * @brief  延时函数
 * @param  us: 延时时间, 单位us
 */
void delay_us(uint32_t us)
{
    uint32_t i;

    while(us--) {
        i=72;
        while(i--);
    }
}

/**
 * @brief  整个工程初始化函数
 */
void proj_init(void)
{
#if PROJ_DEBUG_ON != 0
    printf("开始初始化整个工程...\r\n");
#endif

/* 关闭所有中断 */
    // 暂时关闭触摸屏中断
    HAL_NVIC_DisableIRQ(XPT2046_TOUCH_IRQ_EXTI_IRQn);
    // 暂时关闭指纹模组中断
    HAL_NVIC_DisableIRQ(ZW101_PendIRQ_EXTI_IRQn);
    // 暂时关闭RFID的按键触发中断
    HAL_NVIC_DisableIRQ(RC522_IRQn);
    // 暂时关闭蓝牙模块的中断
    // HAL_NVIC_DisableIRQ(BT_IRQn);

/* ILI9341_LCD */
    // 初始化LCD
    ILI9341_Init();
    ILI9341_SetOrientation(4);
    // XPT2046_UpdateCalibrateParam();
    ILI9341_FillScreen(ILI9341_BLACK);

    // 打开背光
    ILI9341_BackLight_Control(ENABLE);

#if PROJ_DEBUG_ON != 0
    printf("ILI9341_LCD 初始化成功!\r\n");
#endif

/* XPT2046_触摸屏 */
    // 初始化触摸屏
    XPT2046_Init();

    // 手动校准触触摸屏
#if XPT2046_CALIBRATION == 1
    XPT2046_TouchManualCalibrate();
    HAL_Delay(1000);
#endif

    ILI9341_FillScreen(ILI9341_WHITE);
    Set_CurrentFont(&Font_11x18);
    ILI9341_WriteString(10, 20, "LCD, TouchScreen initialized", ILI9341_RED, ILI9341_WHITE);

    HAL_Delay(1000);

    // 关闭背光
    // ILI9341_BackLight_Control(DISABLE);

#if PROJ_DEBUG_ON != 0
    printf("XPT2046_TouchScreen 初始化成功!\r\n");
#endif

/* zw101_指纹模组 */
    // 初始化指纹模组
    zw101_init();

    // 进入睡眠模式
    while( zw101_sleep() != FP_RET_OK );

#if PROJ_DEBUG_ON != 0
    printf("ZW101_Fingerprint 初始化成功!\r\n");
#endif

/* w25q64_flash闪存 */
    // 初始化Flash
    SPI_FLASH_Init();

    // 创建一个密码文件
    while(create_passwd_file() != FR_OK) {
        HAL_Delay(1000);
    }

#if PROJ_DEBUG_ON != 0
    printf("W25Q64_Flash 初始化成功!\r\n");
#endif

/* uln2003_步进电机驱动模块 */
    // 初始化 ULN2003
    ULN2003_Init();

#if PROJ_DEBUG_ON != 0
    printf("ULN2003_步进电机驱动模块 初始化成功!\r\n");
#endif

HAL_GPIO_WritePin(XPT2046_TOUCH_CS_GPIO_Port, XPT2046_TOUCH_CS_Pin, GPIO_PIN_SET);

/* mfrc522_RFID模块 */
    // 初始化RC522
    RC522_Init();
    HAL_Delay(50);
    PcdReset();

    // 设置工作方式
    M500PcdConfigISOType( 'A' );

    HAL_Delay(500);

#if PROJ_DEBUG_ON == 1
    printf("MFRC522_RFID 模块 初始化成功!\r\n");
#endif

/* DX_BT04_E_蓝牙模块 */
    // 暂时使能蓝牙模块的中断
    __HAL_GPIO_EXTI_CLEAR_IT(BT_IRQn);
    HAL_NVIC_EnableIRQ(BT_IRQn);

    // 初始化蓝牙模块
    bt_init();                      // 初始化 BT04_E 蓝牙模块
    bt_disconnect();                // 先强制退出透传模式，因为蓝牙模块只有不在透传模式才可以处理 AT 指令
    HAL_Delay(50);                  // 等待蓝牙模块处理完上一条指令
    bt_control_trandport(0, 100);   // 禁止透传模式
    HAL_Delay(50);                  // 等待蓝牙模块处理完上一条指令
    bt_get_info(100);               // 获取并答应 BT04_E 蓝牙模块基本信息
    bt_control_trandport(1, 100);   // 允许透传模式
    bt_clean_all_recv_buffer();     // 清空接收缓冲区

    bt_finish_init = 1;            // 蓝牙模块初始化完成

    // 关闭蓝牙模块的中断
    HAL_NVIC_DisableIRQ(BT_IRQn);

#if PROJ_DEBUG_ON == 1
    printf("BT04_E 蓝牙模块 初始化成功!\r\n");
#endif
}


static FRESULT create_passwd_file(void)
{
    FRESULT res;

    do {
        // 挂载文件系统
        res = f_mount(&projfs, (TCHAR const*)SPI_FLASH_PATH, 1);
        if(res == FR_NO_FILESYSTEM) {    // 文件系统不存在，创建文件系统
            res = f_mkfs((const TCHAR *)SPI_FLASH_PATH, 0, buffer, FF_MAX_SS);
            printf("No filesystem found, creating a new one...\n");
            if(res == FR_OK) {      // 成功创建文件系统
                res = f_mount(&projfs, (TCHAR const*)SPI_FLASH_PATH, 1);
                if(res == FR_OK) {
                    printf("Create filesystem and mount successfully\n");
                } else {
                    printf("Failed to mount filesystem: %d\n", res);
                    break;
                }
            } else {         // 创建文件系统失败
                printf("Failed to create filesystem: %d\n", res);
                break;
            }
        } else if(res != FR_OK) {    // 其他错误
            printf("Failed to mount filesystem: %d\n", res);
            break;
        }

        // 创建管理员密码文件
        res = f_open(&fp, PROJ_ADMIN_PASSWD_FILENAME, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
        if(res != FR_OK) {
            printf("Failed to open admin passwd file: %d\n", res);
            f_unmount((TCHAR const*)SPI_FLASH_PATH);    // 卸载文件系统
            break;
        }

        // 设置默认的管理员密码为 0000_0000
        uint8_t passwd[PROJ_PASSWD_LEN] = {0};
        UINT bw = 0;

        for(int i=0; i<PROJ_PASSWD_LEN; i++) {
            passwd[i] = 0;
        }
        res = f_write(&fp, passwd, PROJ_PASSWD_LEN, &bw);
        if(res != FR_OK) {
            printf("Failed to write admin passwd: %d\n", res);
            f_close(&fp);
            f_unmount((TCHAR const*)SPI_FLASH_PATH);    // 卸载文件系统
            break;
        }
        
        f_close(&fp);

        // 创建用户密码文件
        res = f_open(&fp, PROJ_USER_PASSWD_FILENAME, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
        if(res != FR_OK) {
            printf("Failed to open user passwd file: %d\n", res);
            f_unmount((TCHAR const*)SPI_FLASH_PATH);    // 卸载文件系统
            break;
        }
        f_close(&fp);

    } while(0);

    return res;
}

void show_passwd(uint8_t *passwd)
{
    for(int i=0; i<PROJ_PASSWD_LEN; i++) {
        printf("%d ", passwd[i]);
    }
    printf("\n");
}

/**
 * @brief  检查密码是否正确
 * @param  passwd: 密码
 * @param  passwd_size: 密码长度
 * @return 读取flash错误返回PROJ_ERROR, 正确返回PROJ_OK
 *         如果为管理员密码, 则passwd[PROJ_PASSWD_LEN]=0,
 *         如果为普通用户密码, 则passwd[PROJ_PASSWD_LEN]=1,
 *         如果不是正确密码, 则passwd[PROJ_PASSWD_LEN]=2
 */
static char mesg[] = "Hello, user 0";
PROJ_RET_Typedef proj_check_passwd(uint8_t *passwd, uint16_t passwd_size)
{
    PROJ_RET_Typedef ret = PROJ_ERROR;
    FRESULT res;
    uint8_t buffer[PROJ_PASSWD_LEN];
    UINT bw = 0;
    uint8_t i;

    // 检查是否是管理员密码
    res = f_open(&fp, PROJ_ADMIN_PASSWD_FILENAME, FA_READ);
    if(res != FR_OK) {
        return ret;
    }

    if((f_read(&fp, buffer, PROJ_PASSWD_LEN, &bw) != FR_OK)) {  // 读取密码失败
        f_close(&fp);
        return ret;
    } else if(bw == 0) {    // 管理员密码为空
        ILI9341_WriteLine(7, 5, "please aet admin passwd", ILI9341_RED, ILI9341_WHITE);
        printf("请先添加管理员密码\n");
        passwd[PROJ_PASSWD_LEN] = 2;
        f_close(&fp);
        return PROJ_OK;
    } else if(bw != PROJ_PASSWD_LEN) {    // 读取密码长度不正确, 即读取过程中发生错误
        ILI9341_WriteLine(7, 5, "Read password fail", ILI9341_RED, ILI9341_WHITE);
        printf("读取密码失败\n");
        f_close(&fp);
        return ret;
    }
    f_close(&fp);

    for(i=0; i<passwd_size; i++) {
        if(passwd[i] != buffer[i]) {
            break;
        }
    }

    if(i != passwd_size) {    // 密码不匹配
        printf("密码与管理员密码不匹配 \r\n");
    } else {    // 密码暂时匹配
        while(i<PROJ_PASSWD_LEN) {
            if(buffer[i++] != 0)
                break;
        }
    }

    if(i == PROJ_PASSWD_LEN) {    // 是管理员密码
        ILI9341_WriteLine(7, 5, "Hello, administrator!", ILI9341_RED, ILI9341_WHITE);
        passwd[PROJ_PASSWD_LEN] = 0;
        return PROJ_OK;
    }

    // 不是管理员密码, 检查是否是普通用户密码
    res = f_open(&fp, PROJ_USER_PASSWD_FILENAME, FA_READ);
    if(res != FR_OK) {
        return ret;
    }

    uint8_t cnt = 0;

    while(1)
    {
        if(f_read(&fp, buffer, PROJ_PASSWD_LEN, &bw) != FR_OK) {  // 读取密码失败
            f_close(&fp);
            return ret;
        } else if(bw == 0) {    // 读取到用户密码结尾
            printf("未能找到匹配的密码\n");
            ILI9341_WriteLine(7, 5, "no matched", ILI9341_RED, ILI9341_WHITE);
            passwd[PROJ_PASSWD_LEN] = 2;
            f_close(&fp);
            return PROJ_OK;
        } else if(bw != PROJ_PASSWD_LEN) {    // 读取密码长度不正确, 即读取过程中发生错误
            printf("读取密码失败\n");
            ILI9341_WriteLine(7, 5, "please aet admin passwd", ILI9341_RED, ILI9341_WHITE);
            f_close(&fp);
            return ret;
        }

        // 成功读取到一则用户密码, 先判断密码是否有效
        show_passwd(buffer);
        if(unused_user_passwd & (1 << cnt++)) {    // 该用户密码无效(被删除)
            continue;
        }

        // 密码有效, 开始比较密码
        for(i=0; i<passwd_size; i++) {
            if(passwd[i] != buffer[i]) {
                break;
            }
        }

        if(i != passwd_size) {    // 密码不匹配
            continue;
        } else {                // 密码暂时匹配
            while(i<PROJ_PASSWD_LEN) {
                if(buffer[i++] != 0)
                    break;
            }
        }

        if(i == PROJ_PASSWD_LEN) {    // 是普通用户密码
            passwd[PROJ_PASSWD_LEN] = 1;
            mesg[sizeof(mesg)-2] = cnt+'0';
            ILI9341_WriteLine(7, 5, mesg, ILI9341_RED, ILI9341_WHITE);
            printf("普通用户 %d\n", cnt);
            f_close(&fp);
            return PROJ_OK;
        }
    }
}

/**
 * @brief  管理员命令处理函数
 * @param  enter_cmd: 输入命令
 * @param  admin_mode: 管理员模式
 * @param  data: 输入数据
 */
static char mesg2[] = "Add user 0!";
void admin_handler(uint16_t enter_cmd, uint8_t *admin_mode, 
            const uint8_t *data, uint16_t *next_data_size)
{
    static int last_cmd = -1;
    uint16_t cmd;

    if(last_cmd != -1) {
        cmd = last_cmd;
    } else {
        cmd = enter_cmd;
    }

    printf("cmd: %d\n", cmd);

    switch(cmd) {
        case 1:    // 开锁
            xSemaphoreGive(OpenLock_Semaphore);     // 发送开锁信号
            *admin_mode = 0;
            *next_data_size = PROJ_PASSWD_LEN;
            break;

        case 2:    // 添加用户密码
            if(cmd != last_cmd) {      // 开始添加密码
                if(useful_user_count >= max_user_count) {
                    printf("用户数量已达到最大值\n");
                    break;
                }

                last_cmd = cmd;
                *next_data_size = PROJ_PASSWD_LEN;
                ILI9341_WriteLine(7, 5, "Input your password", ILI9341_RED, ILI9341_WHITE);
                printf("input your passwd\n");
            } else {        // 用户已经输入密码
                last_cmd = -1;
                *admin_mode = 0;
                
                FRESULT res;
                UINT bw = 0;

                res = f_open(&fp, PROJ_USER_PASSWD_FILENAME, FA_WRITE);
                if(res != FR_OK) {
                    ILI9341_WriteLine(7, 5, "Open file fail", ILI9341_RED, ILI9341_WHITE);
                    printf("Failed to open user passwd file: %d\n", res);
                    break;
                }

                // res = f_lseek(&fp, f_size(&fp));
                res = f_lseek(&fp, useful_user_count * PROJ_PASSWD_LEN);
                if(res != FR_OK) {
                    ILI9341_WriteLine(7, 5, "Seek file fail", ILI9341_RED, ILI9341_WHITE);
                    printf("Failed to seek user passwd file: %d\n", res);
                    f_close(&fp);
                    break;
                }
                
                res = f_write(&fp, data, PROJ_PASSWD_LEN, &bw);
                if(res != FR_OK) {
                    ILI9341_WriteLine(7, 5, "Write file fail", ILI9341_RED, ILI9341_WHITE);
                    printf("Failed to write user passwd: %d\n", res);
                    f_close(&fp);
                    break;
                }

                used_user_passwd |= (1 << useful_user_count);
                useful_user_count++;

                mesg2[sizeof(mesg2)-3] = useful_user_count+'0';
                ILI9341_WriteString(7, 5, mesg2, ILI9341_RED, ILI9341_WHITE);
                printf("succefffully add user %d\n", useful_user_count);

                f_close(&fp);
            }

            break;

        case 3:     // 添加用用户指纹
            // 关闭指纹模组中断
            HAL_NVIC_DisableIRQ(ZW101_PendIRQ_EXTI_IRQn);

            // 唤醒指纹模组
            zw101_wakeup();

            uint8_t ret = zw101_shand_shake();
            while( ret != 0x55 ) {
                printf("fail to shake hand, ret = %d\n", ret);
                ret = zw101_shand_shake();
            }

            // 开始注册指纹
            printf("begin register fingerprint\n");
            if(zw101_register_FP(3, useful_user_count) == FP_RET_OK) {  // 注册指纹成功
                used_user_fp |= (1 << useful_user_count); 
                useful_user_count ++;
                ILI9341_WriteLine(7, 5, "register finished", ILI9341_RED, ILI9341_WHITE);
                printf("successfully register\n");
                zw101_led_set_rgb(ZW101_LED_GREEN);
                vTaskDelay(1000);
            } else {
                ILI9341_WriteLine(7, 5, "register failed", ILI9341_RED, ILI9341_WHITE);
                printf("fail to register fp\n");
                zw101_led_set_rgb(ZW101_LED_RED);
                vTaskDelay(1000);
            }

            // 进入睡眠
            while(zw101_sleep() != FP_RET_OK);
            // 重新使能触指纹模组中断
            __HAL_GPIO_EXTI_CLEAR_IT(ZW101_PendIRQ_Pin);
            HAL_NVIC_EnableIRQ(ZW101_PendIRQ_EXTI_IRQn);
            
            *admin_mode = 0;
            *next_data_size = PROJ_PASSWD_LEN;

            break;

        case 4:     // 添加 RFID 卡
            HAL_NVIC_DisableIRQ(RC522_IRQn);

            ILI9341_WriteLine(7, 5, "Put IC card", ILI9341_RED, ILI9341_WHITE);

            while( ChangeKey(ucBlock/4*4+3, 0, KeyValue_A) != MI_OK );
            
            used_user_rfid |= (1 << useful_user_count);
            useful_user_count++;
            while( PcdWrite(ucBlock, (uint8_t *)&useful_user_count) != MI_OK );

            // 休眠卡片
            PcdHalt();

            mesg2[sizeof(mesg2)-3] = useful_user_count+'0';
            ILI9341_WriteLine(7, 5, mesg2, ILI9341_RED, ILI9341_WHITE);
            printf("修改密码成功\n");
            *admin_mode = 0;
            *next_data_size = PROJ_PASSWD_LEN;

            __HAL_GPIO_EXTI_CLEAR_IT(RC522_IRQn);
            HAL_NVIC_EnableIRQ(RC522_IRQn);

            break;

        case 5:     // 删除用户密码
            if(cmd != last_cmd) {
                last_cmd = cmd;
                *next_data_size = 2;
                ILI9341_WriteLine(7, 5, "Input user's order", ILI9341_RED, ILI9341_WHITE);
                printf("input the user order\n");
            } else {        // 用户已经输入用用户号
                unused_user_passwd |= (1 << --enter_cmd);

                ILI9341_WriteLine(7, 5, "Delete success", ILI9341_RED, ILI9341_WHITE);
                printf("delete user %d\n");

                last_cmd = -1;
                *next_data_size = PROJ_PASSWD_LEN;
                *admin_mode = 0;
            }


            break;

        case 6:     // 更改管理员密码
            if(cmd != last_cmd) {      // 开始添加密码
                last_cmd = cmd;
                *next_data_size = PROJ_PASSWD_LEN;
                ILI9341_WriteLine(7, 5, "Input new password", ILI9341_RED, ILI9341_WHITE);
                printf("input new admin passwd\n");
            } else {        // 用户已经输入密码
                last_cmd = -1;
                *admin_mode = 0;
                
                FRESULT res;
                UINT bw = 0;

                res = f_open(&fp, PROJ_ADMIN_PASSWD_FILENAME, FA_WRITE);
                if(res != FR_OK) {
                    ILI9341_WriteLine(7, 5, "Open file fail", ILI9341_RED, ILI9341_WHITE);
                    printf("Failed to open admin passwd file: %d\n", res);
                    break;
                }
                
                res = f_write(&fp, data, PROJ_PASSWD_LEN, &bw);
                if(res != FR_OK) {
                    ILI9341_WriteLine(7, 5, "Write file fail", ILI9341_RED, ILI9341_WHITE);
                    printf("Failed to write admin passwd: %d\n", res);
                    f_close(&fp);
                    break;
                }

                ILI9341_WriteLine(7, 5, "Change success", ILI9341_RED, ILI9341_WHITE);
                printf("change admin passwd success\n");

                f_close(&fp);
            }

            break;

        case 7:     // 删除指纹
            if(cmd != last_cmd) {
                last_cmd = cmd;
                *next_data_size = 2;
                ILI9341_WriteLine(7, 5, "Input user's order", ILI9341_RED, ILI9341_WHITE);
                printf("input the user order\n");
            } else {        // 用户已经输入用户号
                last_cmd = -1;

                // 关闭指纹模组中断
                HAL_NVIC_DisableIRQ(ZW101_PendIRQ_EXTI_IRQn);

                // 唤醒指纹模组
                zw101_wakeup();
                while( zw101_shand_shake() != 0x55 );

                // 开始删除指纹
                printf("begin delete fingerprint\n");
                if(zw101_delete_FP(--enter_cmd, 1) == FP_RET_OK) {  // 删除指纹成功
                    unused_user_fp |= (1 << enter_cmd);
                    ILI9341_WriteLine(7, 5, "Delete success", ILI9341_RED, ILI9341_WHITE);
                    printf("successfully delete\n");
                } else {
                    ILI9341_WriteLine(7, 5, "Delete failed", ILI9341_RED, ILI9341_WHITE);
                    printf("fail to delete fp\n");
                }

                // 进入睡眠
                while(zw101_sleep() != FP_RET_OK);
                // 重新使能触指纹模组中断
                __HAL_GPIO_EXTI_CLEAR_IT(ZW101_PendIRQ_Pin);
                HAL_NVIC_EnableIRQ(ZW101_PendIRQ_EXTI_IRQn);
                
                *admin_mode = 0;
                *next_data_size = PROJ_PASSWD_LEN;
            }

            break;

        case 8:     // 删除 RFID 卡
            if(cmd != last_cmd) {
                last_cmd = cmd;
                *next_data_size = 2;
                ILI9341_WriteLine(7, 5, "Input user's order", ILI9341_RED, ILI9341_WHITE);
                printf("input the user order\n");
            } else {        // 用户已经输入用户号
                unused_user_rfid |= (1 << --enter_cmd);
                used_user_rfid &= ~(1 << enter_cmd);

                ILI9341_WriteLine(7, 5, "Delete success", ILI9341_RED, ILI9341_WHITE);
                printf("delete user %d\n");

                last_cmd = -1;
                *next_data_size = PROJ_PASSWD_LEN;
                *admin_mode = 0;
            }
            break;

        default:
            last_cmd = -1;
            *admin_mode = 0;
            *next_data_size = PROJ_PASSWD_LEN;
            break;
    }
}

static RTC_DateTypeDef GetDate = {0};
static RTC_TimeTypeDef GetTime = {0};
static char rtc_time_str[] = "2021-01-01 00:00:00";
void proj_show_time(void)
{
    HAL_RTC_GetTime(&hrtc, &GetTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &GetDate, RTC_FORMAT_BIN);

    rtc_time_str[2] = '0' + GetDate.Year / 10;
    rtc_time_str[3] = '0' + GetDate.Year % 10;
    rtc_time_str[5] = '0' + GetDate.Month / 10;
    rtc_time_str[6] = '0' + GetDate.Month % 10;
    rtc_time_str[8] = '0' + GetDate.Date / 10;
    rtc_time_str[9] = '0' + GetDate.Date % 10;

    rtc_time_str[11] = '0' + GetTime.Hours / 10;
    rtc_time_str[12] = '0' + GetTime.Hours % 10;
    rtc_time_str[14] = '0' + GetTime.Minutes / 10;
    rtc_time_str[15] = '0' + GetTime.Minutes % 10;
    rtc_time_str[17] = '0' + GetTime.Seconds / 10;
    rtc_time_str[18] = '0' + GetTime.Seconds % 10;

    ILI9341_WriteLine(ILI9341_X_Scale/(sizeof(rtc_time_str) + 2), 
                          ILI9341_Y_Scale-40, rtc_time_str, ILI9341_BLACK, ILI9341_WHITE);
}

void vApplicationIdleHook( void )
{
    static uint32_t last_time = 0;

    // 每1秒重画一次按钮
    if(HAL_GetTick() - last_time > 1000) {
        proj_show_keyboard_again();
        proj_show_time();
        last_time = HAL_GetTick();
    }
}
