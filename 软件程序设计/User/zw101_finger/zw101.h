#ifndef __ZW101_H__
#define __ZW101_H__

#include "main.h"
#include "usart.h"

#define ZW101_ADDRESS           ( 0xFFFFFFFF )

#define FP_HEAD_HIGH			( 0xEF ) // 包头 0xEF01
#define FP_HEAD_LOW 		    ( 0x01 ) // 包头 0xEF01

#define FP_CMD_PKG               ( 0x01 ) // 命令包
#define FP_DATA_PKG              ( 0x02 ) // 数据包
#define FP_EOF_PKG               ( 0x08 ) // 结束包

#define FP_TIMEOUT_TIME          ( 1000 ) // 超时时间
#define FP_TRANSMIT(pack, n)   HAL_UART_Transmit(&huart2, (uint8_t *)pack, n, FP_TIMEOUT_TIME)
#define FP_RECEIVE(pack, n)    HAL_UART_Receive(&huart2, (uint8_t *)pack, n, FP_TIMEOUT_TIME)

// 是否打开调试信息, 当且仅当为 1 时打开调试信息
#define FP_DEBUG_ON 0

// 错误处理
#define FP_ERROR_HANDLER(str, ret) \
    do { \
        if( FP_DEBUG_ON == 1 ) \
            printf(str, ret); \
        return ret; \
    } while(0)

//

// 可发送的命令
typedef enum {
    // 通用类指令
 	FP_CMD_MATCH_GETIMAGE             = 0x01, // 验证用获取图像
    FP_CMD_ENROLL_GETIMAGE            = 0x29, // 注册用获取图像
	FP_CMD_GEN_CHAR 	              = 0x02, // 生成特征
	FP_CMD_SEARCH_MODEL               = 0x04, // 搜索指纹
    FP_CMD_GEN_MODEL                  = 0x05, // 合并特征, 生成模板
    FP_CMD_STORE_MODEL                = 0x06, // 存储模板
	FP_CMD_DEL_MODEL     	          = 0x0C, // 删除模板
	FP_CMD_EMPTY_MODEL   		      = 0x0D, // 清空指纹库
    FP_CMD_LOAD_CHAR                  = 0x07, // 载入模板到模板缓冲区中

    FP_CMD_WRITE_SYSPARA              = 0x0E, // 写系统寄存器
    FP_CMD_READ_SYSPARA               = 0x0F, // 读取模组基本参数
    FP_CMD_READ_VALID_TEMPLETE_NUMS   = 0x1D, // 读有效模板个数
    FP_CMD_READ_TEMPLETE_INDEX_TABLE  = 0x1F, // 读索引表

    // 模块指令集
    FP_CMD_AUTO_CANCEL  = 0x30,
    FP_CMD_AUTO_ENROLL  = 0x31,
    FP_CMD_AUTO_MATCH   = 0x32,
    FP_CMD_INTO_SLEEP   = 0x33,

    // 维护类指令
    FP_CMD_HANDSHAKE    = 0x53,
    FP_CMD_LED_BlnAmSw  = 0x60,
    FP_CMD_LED_CTRL     = 0x3C,
    FP_CMD_CHECK_FINGER = 0x9d,
} FP_CMD;

// 传输数据包可能的返回值
typedef enum {
    FP_RET_OK                 = 0x00,    // 成功
    FP_RET_COMM_ERR           = 0x01,    // 通信错误, 一般是校验和出错
    FP_RET_NO_FINGER          = 0x02,    // 没有检测到指纹
    FP_RET_GET_IMG_ERR        = 0x03,    // 获取图像错误
    FP_RET_FP_TOO_DRY         = 0x04,    // 指纹太干
    FP_RET_FP_TOO_WET         = 0x05,    // 指纹太湿
    FP_RET_FP_DISORDER        = 0x06,    // 指纹混乱
    FP_RET_LITTLE_FEATURE     = 0x07,    // 特征点太少
    FP_RET_NOT_MATCH          = 0x08,    // 不匹配
    FP_RET_NOT_SEARCHED       = 0x09,    // 未搜索到
    FP_RET_MERGE_ERR          = 0x0a,    // 合并错误
    FP_RET_ADDRESS_OVER       = 0x0b,    // 地址溢出
    FP_RET_READ_ERR           = 0x0c,    // 读取错误
    FP_RET_UP_TEMP_ERR        = 0x0d,    // 上传模板错误
    FP_RET_RECV_ERR           = 0x0e,    // 接收错误
    FP_RET_UP_IMG_ERR         = 0x0f,    // 上传图像错误
    FP_RET_DEL_TEMP_ERR       = 0x10,    // 删除模板错误
    FP_RET_CLEAR_TEMP_ERR     = 0x11,    // 清除模板错误
    FP_RET_SLEEP_ERR          = 0x12,    // 睡眠错误
    FP_RET_INVALID_PASSWORD   = 0x13,    // 无效密码
    FP_RET_RESET_ERR          = 0x14,    // 复位错误
    FP_RET_INVALID_IMAGE      = 0x15,    // 无效图像
    FP_RET_HANGOVER_UNREMOVE  = 0x17,    // 未移除残留
    FP_RET_FLASH_ERR          = 0x18,    // Flash错误
    FP_RET_TRNG_ERR           = 0x19,    // TRNG错误
    FP_RET_INVALID_REG        = 0x1a,    // 无效寄存器
    FP_RET_REG_SET_ERR        = 0x1b,    // 寄存器设置错误
    FP_RET_NOTEPAD_PAGE_ERR   = 0x1c,    // Notepad页面错误
    FP_RET_ENROLL_ERR         = 0x1e,    // 注册错误
    FP_RET_LIB_FULL_ERR       = 0x1f,    // 指纹库已满
    FP_RET_DEVICE_ADDR_ERR    = 0x20,    // 设备地址错误
    FP_RET_MUST_VERIFY_PWD    = 0x21,    // 必须验证密码
    FP_RET_TMPL_NOT_EMPTY     = 0x22,    // 指纹模板非空
    FP_RET_TMPL_EMPTY         = 0x23,    // 指纹模板为空
    FP_RET_LIB_EMPTY_ERR      = 0x24,    // 指纹库为空
    FP_RET_TMPL_NUM_ERR       = 0x25,    // 录入次数设置错误
    FP_RET_TIME_OUT           = 0x26,    // 超时
    FP_RET_FP_DUPLICATION     = 0x27,    // 指纹已存在
    FP_RET_TMP_RELATION       = 0x28,    // 指纹模板有关联
    FP_RET_CHECK_SENSOR_ERR   = 0x29,    // 传感器初始化失败
    FP_RET_MOD_INF_NOT_EMPTY  = 0x2a,    // 模组信息非空
    FP_RET_MOD_INF_EMPTY      = 0x2b,    // 模组信息空
    FP_RET_ENROLL_CANCEL      = 0x2C,    // 取消注册
    FP_RET_IMAGE_SMALL        = 0x33,    // 图像面积太小
    FP_RET_IMAGE_UNAVAILABLE  = 0x34,    // 图像不可用
    FP_RET_ENROLL_TIMES_NOT_ENOUGH = 0x40, // 注册次数不够

    FP_RET_OTHER_FAILE        = 0xFE,    // 其他错误
    FP_RET_TIMEOUT            = 0xFF     // 超时
} FP_Ret_Type;

// 指纹模组 IRQ 引脚状态类型定义
typedef enum {
    ZW101_PendIRQ_PRESSE_WARNING,
    ZW101_PendIRQ_PRESSED,
    ZW101_PendIRQ_RELEASE_WARNING,
    ZW101_PendIRQ_RELEASED,
} ZW101_PendIRQ_Statue_Type;

// 指纹模组 LED 颜色类型定义
typedef enum {
    ZW101_LED_RED   = 0x04,
    ZW101_LED_GREEN = 0x02,
    ZW101_LED_BLUE  = 0x01
} ZW101_LED_RGB_Type;

// 指纹模组 LED 控制类型定义
typedef struct {
    uint8_t Function;           // 功能码
    uint8_t Start_Color;        // 起始颜色
    uint8_t EndColor_or_DutyCycle;  // 结束颜色或占空比
    uint8_t Circle_Counts;      // 次数
    uint8_t Period_Time;        // 周期
} ZW101_LED_Control_Type;

// 发送和接收数据包的缓冲区定义
#define FP_SEND_PACK_SIZE   20
#define FP_RECV_PACK_SIZE   50

extern uint8_t zw101_recv_packet[FP_RECV_PACK_SIZE];

extern uint16_t fp_EnrollTimes, fp_TempSize, fp_DataBaseSize;
extern uint16_t fp_ScoreLevel, fp_PktSize, fp_BaudRate;
extern uint32_t fp_DeviceAddress;


extern FP_Ret_Type zw101_init(void);
extern ZW101_PendIRQ_Statue_Type zw101_GetIRQStatue(
    uint8_t use_new_status, ZW101_PendIRQ_Statue_Type new_status, 
    uint8_t use_new_ticks, uint32_t new_ticks);
extern void zw101_power_control(uint8_t act);

extern FP_Ret_Type zw101_clear_FP_lib(void);
extern FP_Ret_Type zw101_delete_FP(uint16_t delete_PageID, uint16_t delete_TemplateNum);

extern FP_Ret_Type zw101_sleep(void);
extern void zw101_wakeup(void);
extern uint8_t zw101_shand_shake(void);

extern FP_Ret_Type zw101_led_switch(uint8_t mode);
extern FP_Ret_Type zw101_led_control(ZW101_LED_Control_Type *led_ctrl);
extern FP_Ret_Type zw101_led_set_rgb(uint8_t rgb);

extern FP_Ret_Type zw101_read_FP_info(void);
extern uint16_t zw101_read_valid_template_num(void);

extern FP_Ret_Type zw101_register_FP(uint8_t regsiter_Count, uint16_t to_PageID);
extern FP_Ret_Type zw101_check_FP(uint16_t begin_PageID, uint16_t num_Page);

extern void ZW101_IRQHandler(void);


#endif /* __ZW101_H__; */
