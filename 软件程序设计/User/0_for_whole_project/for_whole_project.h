#ifndef __FOR_WHOLE_PROJECT__
#define __FOR_WHOLE_PROJECT__

#include "bt04_e.h"
#include "ff.h"
#include "ili9341.h"
#include "mfrc522.h"
#include "sz_system.h"
#include "uln2003.h"
#include "w25q64.h"
#include "xpt2046.h"
#include "zw101.h"

#include "rtc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* 是否开启调试 */
#define PROJ_DEBUG_ON 0

/* 项目使用的字体大小 */
#define PROJ_ILI9341_FONT_WIDTH  11
#define PROJ_ILI9341_FONT_HEIGHT 18

/* 是否手动校准 XPT2046 */
#define XPT2046_CALIBRATION 1

/* 密码相关程序使用到的全局变量 */
extern FATFS projfs;
#define SPI_FLASH_PATH "1:"
#define PROJ_PASSWD_LEN 8
#define PROJ_ADMIN_PASSWD_FILENAME      SPI_FLASH_PATH"adm_pwd.txt"
#define PROJ_USER_PASSWD_FILENAME       SPI_FLASH_PATH"user_pwd.txt"
#define PROJ_SHOW_PASSWD 0   // 是否显示密码, 如果未使能此宏则将会显示'*'

/* 任务用变量 */
extern SemaphoreHandle_t ButtonEvent_Semaphore;         // 按钮事件信号量

extern SemaphoreHandle_t OpenLock_Semaphore;            // 开锁信号量

extern SemaphoreHandle_t Fingerprint_Semaphore;         // 指纹事件信号量

extern SemaphoreHandle_t RFID_Semaphore;              // RFID事件信号量
extern uint8_t ucBlock;
extern uint8_t KeyValue_A[];
extern uint32_t used_user_rfid, unused_user_rfid;    // 已用、已删除的用户RFID;

extern SemaphoreHandle_t BT_Semaphore;              // 蓝牙事件信号量
extern uint8_t bt_finish_init;   // 蓝牙初始化完成标志

void delay_us(uint32_t us);
extern void proj_init(void);
extern BaseType_t proj_create_task(void);
extern PROJ_RET_Typedef proj_show_keyboard(void);
extern PROJ_RET_Typedef proj_show_keyboard_again(void);
extern void proj_clear_echo_area(void);
extern void proj_clear_echo_data(void);
extern PROJ_RET_Typedef proj_check_passwd(uint8_t *passwd, uint16_t passwd_size);
extern void admin_handler(uint16_t enter_cmd, uint8_t *admin_mode, const uint8_t *data, uint16_t *next_data_size);

#endif

