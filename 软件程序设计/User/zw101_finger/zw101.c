#include "zw101.h"
#include "for_whole_project.h"

// 注册次数, 指纹模板大小, 指纹库大小(即 PageID 的范围)
uint16_t fp_EnrollTimes, fp_TempSize, fp_DataBaseSize;
// 分数等级, 数据包大小, 波特率系数(*9600才是波特率)
uint16_t fp_ScoreLevel, fp_PktSize, fp_BaudRate;
// 设备地址
uint32_t fp_DeviceAddress;

// 定义数据包
uint8_t zw101_send_packet[FP_SEND_PACK_SIZE];
uint8_t zw101_recv_packet[FP_RECV_PACK_SIZE];

/**
 * @brief 显示接收数据包
 * @param len 要显示的长度
 */
__attribute__((unused)) static void show_zw101_recv_packet(uint8_t len)
{
    if(len == 0) len = FP_RECV_PACK_SIZE;

    for(uint8_t i=0; i<len; i++) {
        printf("%02X ", zw101_recv_packet[i]);
    }
    printf("\n");
}


// 发送指令包
/**
 * @brief 发送指令包
 * @param cmd 指令码
 * @param data 数据
 * @param len 数据长度
 * @return 返回发送结果
 */
static FP_Ret_Type zw101_send_cmd(FP_CMD cmd, uint8_t *data, uint8_t len)
{
#define MAX_SEND_LEN   (FP_SEND_PACK_SIZE - 9)
    uint16_t index;
    // 计算校验和, 指纹模组会检查此值，如果与预期值不符，则认为指令包发送失败, 即在接收模组响应的数据包中错误码为 0x01
    len += 3;
    uint16_t checksum = 1 + len + cmd;
    if(len > MAX_SEND_LEN) len = MAX_SEND_LEN; // 最大发送长度为 MAX_SEND_LEN

    zw101_send_packet[6] = FP_CMD_PKG;
    zw101_send_packet[7] = (len >> 8) & 0xFF;
    zw101_send_packet[8] = len & 0xFF;
    zw101_send_packet[9] = cmd;
    for (index = 0; index < len - 3; index++) {
        zw101_send_packet[10 + index] = data[index];
        checksum += data[index];
    }
    index += 10;
    zw101_send_packet[index++] = (checksum >> 8) & 0xFF;
    zw101_send_packet[index++] = checksum & 0xFF;

    uint16_t i;
    uint32_t cur_tick = HAL_GetTick();
    for(i=0; (i<index) && ((HAL_GetTick() - cur_tick) < FP_TIMEOUT_TIME); i++)
        FP_TRANSMIT(zw101_send_packet+i, 1);

    if(i < index) {
        return FP_RET_TIMEOUT;
    } else {
        return FP_RET_OK;
    }
}

/**
 * @brief 接收响应包
 * @param len 接收长度
 * @return 超时则返回 0xFF，否则返回确认码
 */
static FP_Ret_Type zw101_recv_response(uint8_t len)
{
// MIN_RECV_LEN 是除了有效数据以外的包长度
#define MIN_RECV_LEN   (12)
    uint8_t index = 0;
    uint32_t cur_tick = HAL_GetTick();
    // if(len < MIN_RECV_LEN) len = MIN_RECV_LEN; // 最小接收长度为 MIN_RECV_LEN
    len += MIN_RECV_LEN;

    // 没法接收这么多数据
    if(len > FP_RECV_PACK_SIZE)
        return FP_RET_OTHER_FAILE;

    /* 等待响应包 */
    // 先读取响应包包头
    FP_RECEIVE(zw101_recv_packet, 1);
    if( HAL_GetTick() - cur_tick < FP_TIMEOUT_TIME ) {
        FP_RECEIVE(zw101_recv_packet+1, 1);
    }

    // 如果响应包包头不正确，则读到正确为止
    while( HAL_GetTick() - cur_tick < FP_TIMEOUT_TIME ) {
        if( (zw101_recv_packet[0] == FP_HEAD_HIGH) && (zw101_recv_packet[1] == FP_HEAD_LOW) ) {   // 正确接收到包头
            index = 2;
            break;
        } else {
            zw101_recv_packet[0] = zw101_recv_packet[1];
            FP_RECEIVE(zw101_recv_packet+1, 1);
        }
    }

    // 接下来才是读响应包包头以后的数据
    while ( (HAL_GetTick() - cur_tick < FP_TIMEOUT_TIME) && (index < len) ) {
        FP_RECEIVE(zw101_recv_packet+index, 1);
        index++;
    }

#if FP_DEBUG_ON == 1
    show_zw101_recv_packet(index);
#endif

    // 返回确认码
    if(index < len) {
#if FP_DEBUG_ON == 1
        printf("receive %d bytes, timeout\n", index);
#endif
        return FP_RET_TIMEOUT;        // 超时
    } else {
        return (FP_Ret_Type)zw101_recv_packet[9]; // 确认码
    }
}


/**
 * @brief zw101 电源控制
 * @param act 0 关闭，1 开启
 * @return none
 */
void zw101_power_control(uint8_t act)
{
    if(act == 0) {
        HAL_GPIO_WritePin(ZW101_Power_GPIO_Port, ZW101_Power_Pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(ZW101_Power_GPIO_Port, ZW101_Power_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief 读取 zw121 指纹模组 IRQ 引脚状态
 * @return 指纹按下状态
 */
ZW101_PendIRQ_Statue_Type zw101_GetIRQStatue(
    uint8_t use_new_status, ZW101_PendIRQ_Statue_Type new_status, 
    uint8_t use_new_ticks, uint32_t new_ticks)
{
#define ZW101_DEBUTTON_TIME     10
    static ZW101_PendIRQ_Statue_Type IRQ_statue = ZW101_PendIRQ_RELEASED;
    static uint32_t last_tick = 0;
    uint32_t cur_tick = HAL_GetTick();

    if(use_new_status) {
        IRQ_statue = new_status;
    }

    if(use_new_ticks) {
        last_tick = new_ticks;
    }

    // 有指纹按下时, 指纹模组的 IRQ 引脚将会输出高电平
    if( HAL_GPIO_ReadPin(ZW101_PendIRQ_GPIO_Port, ZW101_PendIRQ_Pin) == GPIO_PIN_SET ) {    // IRQ 引脚为高电平
        switch( IRQ_statue ) {
            case ZW101_PendIRQ_PRESSE_WARNING:
                // 延时消抖时间到才确认指纹模组有指纹按下
                if((cur_tick - last_tick) >= ZW101_DEBUTTON_TIME) {
                    IRQ_statue = ZW101_PendIRQ_PRESSED;
                }
                break;
            case ZW101_PendIRQ_PRESSED:
                break;
            default:
                IRQ_statue = ZW101_PendIRQ_PRESSE_WARNING;
                last_tick = cur_tick;
        }
    } else {                        // IRQ 引脚为低电平
        switch( IRQ_statue ) {
            case ZW101_PendIRQ_RELEASE_WARNING:
                // 延时消抖时间到才确认指纹模组的指纹已经松开
                if((cur_tick - last_tick) >= ZW101_DEBUTTON_TIME) {
                    IRQ_statue = ZW101_PendIRQ_RELEASED;
                }
                break;
            case ZW101_PendIRQ_RELEASED:
                break;
            default:
                IRQ_statue = ZW101_PendIRQ_RELEASE_WARNING;
                last_tick = cur_tick;
        }
    }

    return IRQ_statue;
}


/**
 * @brief 注册用获取指纹图像
 * @return FP_RET_OK 成功，其他失败
 */
static FP_Ret_Type zw101_get_image_register(void)
{
    FP_Ret_Type ret;

    // 获取图像
    ret = zw101_send_cmd(FP_CMD_ENROLL_GETIMAGE, NULL, 0);
    if(ret != FP_RET_OK) return ret;   // 超时返回, 此时是硬件配置、连接问题, 因此就算再循环下去也还是失败的

    // 等待指纹模组响应
    ret = zw101_recv_response(0);
    return ret;
}

/**
 * @brief 检验用获取指纹图像
 * @return FP_RET_OK 成功，其他失败
 */
static FP_Ret_Type zw101_get_image_check(void)
{
    FP_Ret_Type ret;

    // 获取图像
    ret = zw101_send_cmd(FP_CMD_MATCH_GETIMAGE, NULL, 0);
    if(ret != FP_RET_OK) return ret;   // 超时返回, 此时是硬件配置、连接问题, 因此就算再循环下去也还是失败的

    // 等待指纹模组响应
    ret = zw101_recv_response(0);
    return ret;
}


/**
 * @brief 生成指纹特征文件
 * @param buffer_id 特征文件缓冲区ID
 * @return FP_RET_OK 成功，其他失败
 */
static FP_Ret_Type zw101_generate_char(uint8_t buffer_id)
{
    FP_Ret_Type ret;

    // 发送指令包
    ret = zw101_send_cmd(FP_CMD_GEN_CHAR, &buffer_id, 1);
    if(ret != FP_RET_OK) return ret;

    // 等待指纹模组响应
    ret = zw101_recv_response(0);
    return ret;
}

/**
 * @brief 合并特征文件, 生成指纹模板
 * @return FP_RET_OK 成功，其他失败
 */
static FP_Ret_Type zw101_generate_model(void)
{
    FP_Ret_Type ret;

    ret = zw101_send_cmd(FP_CMD_GEN_MODEL, NULL, 0);
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    return ret;
}

/**
 * @brief 存储指纹模板
 * @param load_BufferID 指纹库页中位置 ID 号
 * @param load_PageID 指纹库页 ID 号
 * @return FP_RET_OK 成功，其他失败
 */
static FP_Ret_Type zw101_store_model(uint8_t load_BufferID, uint16_t load_PageID)
{
    FP_Ret_Type ret;

    uint8_t buffer[] = { load_BufferID, (load_PageID >> 8) & 0xFF, load_PageID & 0xFF };
    ret = zw101_send_cmd(FP_CMD_STORE_MODEL, buffer, sizeof(buffer));
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    return ret;
}


/**
 * @brief 搜索指纹
 * @param target_BufferID 要比对的模板缓冲区ID
 * @param begin_PageID 开始搜索的模板页ID
 * @param num_Page 要搜索的模板页数
 * @return FP_RET_OK 成功，其他失败
 */
static FP_Ret_Type zw101_search_FP(uint8_t target_BufferID, uint16_t begin_PageID, uint16_t num_Page)
{
    FP_Ret_Type ret;

    uint8_t buffer[] = { target_BufferID, (begin_PageID >> 8) & 0xFF, begin_PageID & 0xFF, 
                    ((num_Page-1) >> 8) & 0xFF, (num_Page-1) & 0xFF };
    ret = zw101_send_cmd(FP_CMD_SEARCH_MODEL, buffer, sizeof(buffer));
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    return ret;
}



/**
 * @brief 初始化指纹模组
 * @return none
 */
FP_Ret_Type zw101_init(void)
{
    FP_Ret_Type ret;

    zw101_send_packet[0] = FP_HEAD_HIGH;
    zw101_send_packet[1] = FP_HEAD_LOW;
    zw101_send_packet[2] = (ZW101_ADDRESS >> 24) & 0xFF;
    zw101_send_packet[3] = (ZW101_ADDRESS >> 16) & 0xFF;
    zw101_send_packet[4] = (ZW101_ADDRESS >> 8) & 0xFF;
    zw101_send_packet[5] = ZW101_ADDRESS & 0xFF;

    // 开启 zw101 电源
    zw101_power_control(1);
    
    // 等待上电握手信号
    while( zw101_shand_shake() != 0x55 );

    ret = zw101_read_FP_info();
    return ret;
}

/**
 * @brief 删除 delete_Num 个指纹模板
 * @param delete_PageID flash指纹库模板号
 * @param delete_TemplateNum 要删除的模板个数
 * @return FP_RET_OK 成功，其他失败
 */
FP_Ret_Type zw101_delete_FP(uint16_t delete_PageID, uint16_t delete_TemplateNum)
{
    FP_Ret_Type ret;
    uint8_t buffer[] = { (delete_PageID >> 8) & 0xFF, delete_PageID & 0xFF, 
                    (delete_TemplateNum >> 8) & 0xFF, delete_TemplateNum & 0xFF };

    ret = zw101_send_cmd(FP_CMD_DEL_MODEL, buffer, sizeof(buffer));
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    return ret;
}

/**
 * @brief 清空指纹库
 * @return FP_RET_OK 成功，其他失败
 */
FP_Ret_Type zw101_clear_FP_lib(void)
{
    FP_Ret_Type ret;

    ret = zw101_send_cmd(FP_CMD_EMPTY_MODEL, NULL, 0);
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    return ret;
}


/**
 * @brief 进入睡眠模式
 * @return FP_RET_OK 成功，其他失败
 */
FP_Ret_Type zw101_sleep(void)
{
    FP_Ret_Type ret;

    ret = zw101_send_cmd(FP_CMD_INTO_SLEEP, NULL, 0);
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    if(ret == FP_RET_OK) {
        // // 开启中断(用于唤醒)
        // __HAL_GPIO_EXTI_CLEAR_IT(ZW101_PendIRQ_Pin);
        // HAL_NVIC_EnableIRQ(ZW101_PendIRQ_EXTI_IRQn);

        // 关闭 zw101 电源
        zw101_power_control(0);
    }
    return ret;
}

/**
 * @brief 唤醒指纹模组
 */
void zw101_wakeup(void)
{
    // 开启 zw101 电源
    zw101_power_control(1);

    // 关闭中断
    HAL_NVIC_DisableIRQ(ZW101_PendIRQ_EXTI_IRQn);
    
}

/**
 * @brief 获取上电握手信号
 * @return 0x55 上电握手成功，其他失败
 */
uint8_t zw101_shand_shake(void)
{
    uint8_t ret;

    ret = zw101_send_cmd(FP_CMD_HANDSHAKE, NULL, 0);
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    return ret;
}

/**
 * @brief 切换 LED 控制模式
 * @param mode 控制模式, 0 表示切换为自动模式, 非 0 表示切换为手动模式
 * @return FP_RET_OK 成功，其他失败
 */
FP_Ret_Type zw101_led_switch(uint8_t mode)
{
    FP_Ret_Type ret;

    mode = !!mode - 1;
    ret = zw101_send_cmd(FP_CMD_LED_BlnAmSw, &mode, 1);
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    return ret;
}

/**
 * @brief 指纹模组 LED 的控制
 * @param led_ctrl 控制参数
 * @return FP_RET_OK 成功，其他失败
 */
FP_Ret_Type zw101_led_control(ZW101_LED_Control_Type *led_ctrl)
{
    FP_Ret_Type ret;

    uint8_t buffer[] = { led_ctrl->Function, led_ctrl->Start_Color, 
            led_ctrl->EndColor_or_DutyCycle, led_ctrl->Circle_Counts, led_ctrl->Period_Time };
    ret = zw101_send_cmd(FP_CMD_LED_CTRL, buffer, sizeof(buffer));
    if(ret != FP_RET_OK) return ret;

    ret = zw101_recv_response(0);
    return ret;
}

/**
 * @brief 指纹模组 LED 常量颜色模式设置
 * @param rgb 颜色值, 可以使用 ZW101_LED_RED, ZW101_LED_GREEN, ZW101_LED_BLUE 等宏定义及其位或值, 0 则表示关闭 LED
 * @return FP_RET_OK 成功，其他失败
 */
FP_Ret_Type zw101_led_set_rgb(uint8_t rgb)
{
    ZW101_LED_Control_Type led_ctrl = {0x03, rgb, 0x00, 0x00, 0x00};
    return zw101_led_control(&led_ctrl);
}

/**
 * @brief 读模组基本参数
 * @return FP_RET_OK 成功，其他失败
 */
FP_Ret_Type zw101_read_FP_info(void)
{
    FP_Ret_Type ret;

    // 发送指令包
    ret = zw101_send_cmd(FP_CMD_READ_SYSPARA, NULL, 0);
    if( ret != FP_RET_OK ) {
        FP_ERROR_HANDLER("Error occured when send command: %d\n", ret);
    }

    // 获取响应包
    ret = zw101_recv_response(16);
    if( ret != FP_RET_OK ) {
        FP_ERROR_HANDLER("Error occured when receive response: %d\n", ret);
    }


    fp_EnrollTimes = (uint16_t)(zw101_recv_packet[10]<<8) | zw101_recv_packet[11];
    fp_TempSize = (uint16_t)(zw101_recv_packet[12]<<8) | zw101_recv_packet[13];
    fp_DataBaseSize  = (uint16_t)(zw101_recv_packet[14]<<8) | zw101_recv_packet[15];
    fp_ScoreLevel  = (uint16_t)(zw101_recv_packet[16]<<8) | zw101_recv_packet[17];
    fp_DeviceAddress  = (uint32_t)(zw101_recv_packet[18]<<24) | (zw101_recv_packet[19]<<16)| (zw101_recv_packet[20]<<8) | zw101_recv_packet[21];
    fp_PktSize = (uint16_t)(zw101_recv_packet[22]<<8) | zw101_recv_packet[23];
    // if(0 == fp_PktSize) {
    //   fp_PktSize = 32;
    // } else if(1 == fp_PktSize) {
    //   fp_PktSize = 64;
    // } else if(2 == fp_PktSize) {
    //   fp_PktSize = 128;
    // } else if(3 == fp_PktSize) {
    //   fp_PktSize = 256;
    // }
    fp_PktSize  = 32 * (1<<fp_PktSize);
    fp_BaudRate = (uint16_t)(zw101_recv_packet[24]<<8) | zw101_recv_packet[25];
    
#if FP_DEBUG_ON == 1
    printf("register cnt: %d\n", fp_EnrollTimes);
    printf("temp size: %d\n", fp_TempSize);
    printf("lib size: %d\n", fp_DataBaseSize);
    printf("level: %d\n", score_level);
    printf("devece address: 0x%x\n", device_addr);
    printf("data size: %d bits\n", data_pack_size);
    printf("baud: %d bps\n", baud_set*9600);
#endif    

    return FP_RET_OK;
}

/**
 * @brief 获取指纹库中有效指纹模板的个数
 * @return 有效模板个数, 0xFFFF 则表示数据通信出错
 */
uint16_t zw101_read_valid_template_num(void)
{
    FP_Ret_Type ret;

    ret = zw101_send_cmd(FP_CMD_READ_VALID_TEMPLETE_NUMS, NULL, 0);
    if( ret != FP_RET_OK ) return 0xFFFF;

    ret = zw101_recv_response(2);
    if( ret != FP_RET_OK ) {
        return 0xFFFF;
    } else {
        return zw101_recv_packet[10]<<8 | zw101_recv_packet[11];
    }
}

/**
 * @brief 注册指纹
 * @return FP_RET_OK 成功，其他失败
 */
FP_Ret_Type zw101_register_FP(uint8_t regsiter_Count, uint16_t to_PageID)
{
// #define GET_IMAGE_NUM   fp_EnrollTimes
#define GET_IMAGE_NUM   regsiter_Count
    uint8_t BUFFER_ID = 1;   // 定义模板缓冲区ID
    uint8_t cnt = 0;
    uint8_t max_circular_count = 2*GET_IMAGE_NUM;
    FP_Ret_Type ret;

    // 判断 PageID 是否超出指纹库范围
    if( to_PageID >= fp_DataBaseSize ) {
        zw101_led_set_rgb(ZW101_LED_RED);
        HAL_Delay(1000);
        // zw101_led_set_rgb(0);
        zw101_led_switch(0);
        return FP_RET_ADDRESS_OVER;
    }


    while( BUFFER_ID <= GET_IMAGE_NUM ) {
        cnt++;

        // 等待用户将手指撤离传感器
        ILI9341_WriteLine(7, 5, "Remove finger", ILI9341_RED, ILI9341_WHITE);
        printf("Please remove your finger from the sensor\n");
        zw101_GetIRQStatue(1, ZW101_PendIRQ_PRESSED, 0, 0);
        while( zw101_GetIRQStatue(0, 0, 0, 0) != ZW101_PendIRQ_RELEASED );

        // 等待用户重新将手指放在传感器上
        ILI9341_WriteLine(7, 5, "Put finger", ILI9341_RED, ILI9341_WHITE);
        printf("Please put your finger again\n");
        zw101_GetIRQStatue(1, ZW101_PendIRQ_RELEASED, 0, 0);
        while( zw101_GetIRQStatue(0, 0, 0, 0) != ZW101_PendIRQ_PRESSED );

        // 步骤1：获取图像
        ret = zw101_get_image_register();
        if ( ret != FP_RET_OK ) {
            if( (ret == FP_RET_TIMEOUT) || (cnt >= max_circular_count) ) return ret;
#if FP_DEBUG_ON == 1
            printf("Get Image failed: %d\n", ret);
#endif
            continue;
        }

        // 步骤2：生成指纹特征文件, 然后存储在第 BUFFER_ID 个模板缓冲区中
        ret = zw101_generate_char(BUFFER_ID);
        if ( ret == FP_RET_OK ) {
            BUFFER_ID++;
        } else {
            if( cnt >= max_circular_count ) return ret;
#if FP_DEBUG_ON == 1
            printf("Generate caracter failed: %d\n", ret);
#endif
            continue;
        }
    }

    // 步骤3：合并模板缓冲区中的特征文件, 结果仍然存放在模板缓冲区中
    ret = zw101_generate_model();
    if ( ret != FP_RET_OK ) 
        FP_ERROR_HANDLER("Generate template model failed: %d\n", ret);

    // 步骤4：将刚刚生成在模板缓冲区中的特征文件存储到 flash 的 PageID 号地址页中
    ret = zw101_store_model(1, to_PageID);
    if ( ret != FP_RET_OK )
        FP_ERROR_HANDLER("Store template model failed: %d\n", ret);

    return FP_RET_OK;
}


/**
 * @brief 检验指纹
 * @param begin_PageID 开始搜索的页号
 * @param num_Page 页数
 * @return FP_RET_OK 成功，其他失败
 * @note  结果实测, 不管这两个输入参数的值是什么, 模组都会默认搜索整个指纹库
 */
FP_Ret_Type zw101_check_FP(uint16_t begin_PageID, uint16_t num_Page)
{
#define SEARCH_COUNT   5
    uint8_t BUFFER_ID = 1;   // 定义模板缓冲区ID, 手册规定必须设为 1
    uint8_t cnt = 0;
    FP_Ret_Type ret;

    while(cnt <= SEARCH_COUNT) {
        cnt ++;
        
        // 等待用户将手指撤离传感器
        ILI9341_WriteLine(7, 5, "Remove finger", ILI9341_RED, ILI9341_WHITE);
        printf("Please remove your finger from the sensor\n");
        zw101_GetIRQStatue(1, ZW101_PendIRQ_PRESSED, 0, 0);
        while( zw101_GetIRQStatue(0, 0, 0, 0) != ZW101_PendIRQ_RELEASED );

        // 等待用户重新将手指放在传感器上
        ILI9341_WriteLine(7, 5, "Put finger", ILI9341_RED, ILI9341_WHITE);
        printf("Please put your finger again\n");
        zw101_GetIRQStatue(1, ZW101_PendIRQ_RELEASED, 0, 0);
        while( zw101_GetIRQStatue(0, 0, 0, 0) != ZW101_PendIRQ_PRESSED );

        // 步骤1：获取图像
        ret = zw101_get_image_check();
        if ( ret != FP_RET_OK ) {
            if(ret == FP_RET_TIMEOUT) return ret;
#if FP_DEBUG_ON == 1
            printf("Error occured when receive response: %d\n", ret);
#endif
            continue;
        }

        // 步骤2：生成特征
        ret = zw101_generate_char(BUFFER_ID);
        if ( ret == FP_RET_OK ) {
            break;
        } else {
#if FP_DEBUG_ON == 1
            printf("Error occured when generate character: %d\n", ret);
#endif
            continue;
        }
    }

    // 步骤3：搜索指纹
    ret = zw101_search_FP(BUFFER_ID, begin_PageID, num_Page);
    return ret;
}

/**
 * @brief IRQ引脚中断服务函数, 此函数执行时说明应当从睡眠模式中唤醒
 */
void ZW101_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // 发送指纹事件信号
    xSemaphoreGiveFromISR(Fingerprint_Semaphore, &xHigherPriorityTaskWoken);

    // 根据返回值执行一次任务调度
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

