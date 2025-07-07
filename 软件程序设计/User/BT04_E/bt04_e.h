#ifndef __BT04_E_H__
#define __BT04_E_H__

#include "main.h"
#include "usart.h"

// bt04_e蓝牙模块复位引脚
#define BT04_E_RST_GPIO_PIN      GPIO_PIN_6
#define BT04_E_RST_GPIO_Port     GPIOB

// bt04_e蓝牙模块串口接收相关定义
#define BT04_E_UART_PORT huart3
extern UART_HandleTypeDef BT04_E_UART_PORT;

#define BT04_E_TRANSMIT_TIMEOUT_TICKS    1000	// 发送超时时间
#define BT04_E_RECV_MIN_TIMEOUT_TICKS    50		// 最小接收等待超时时间

#define BT04_E_RECV_BUFFER_SIZE			30		// 一个接收缓冲区的大小
#define BT04_E_RECV_BUFFER_NUM			5		// 接收缓冲区个数
typedef struct {
	uint8_t bt_receive_buffer[BT04_E_RECV_BUFFER_NUM][BT04_E_RECV_BUFFER_SIZE];
	uint8_t bt_recv_buffer_pos;			// 当前的接收数据的缓冲区索引
	uint8_t bt_shouldread_buffer_pos;	// 下一个应该读取的缓冲区索引
	uint8_t bt_shouldread_num;				// 待读取的缓冲区数量
} BT_RECV_Buffer_Type;
extern BT_RECV_Buffer_Type bt_recv_buffer;

#define BT_IRQn USART3_IRQn

// bt04_e蓝牙模块 AT 命令相关定义
#define BT04_E_AT_CMD_SOFT_RESET	"RESET"
#define BT04_E_AT_CMD_GET_NAME		"NAME"
#define BT04_E_AT_CMD_DISCONNECT	"DISC"
#define BT04_E_AT_CMD_HELP			"HELP"
#define BT04_E_AT_CMD_TRANSPORT		"TRANSPORT"

// 返回值类型
typedef enum {
	BT_OK,
	BT_TIMEOUT,
	BT_OTHER_ERR,
} BT_RET_Type;


// 是否开启调试信息
#define BT_DEBUG_ON	0

#define BT_DEBUG(fmt, arg...) \
	do{ \
		if(BT_DEBUG_ON) \
		printf("<<-BT-DEBUG->> [%d]"fmt, __LINE__, ##arg); \
	}while(0)

#define BT_INFO(fmt,arg...)           printf("<<-BT-INFO->> "fmt"\n",##arg)
#define BT_ERROR(fmt,arg...)          printf("<<-BT-ERROR->> "fmt"\n",##arg)


extern BT_RET_Type bt_init(void);
extern uint8_t bt_clean_recv_buffer(void);
extern void bt_clean_all_recv_buffer(void);
extern uint8_t *bt_recv_data(uint8_t to_newest);
extern uint16_t bt_send_data(const uint8_t *str, uint16_t len);
extern void bt_send_cmd_without_ok(const uint8_t *command, const uint8_t *arg, uint8_t arg_max_len);
extern BT_RET_Type bt_send_cmd_with_ok(const uint8_t *command, const uint8_t *arg, uint8_t arg_max_len, uint32_t wait_ticks);


extern BT_RET_Type bt_soft_reset(uint8_t wait_ok, uint8_t wait_ok_ticks, uint8_t wait_finish, uint8_t wait_finish_ticks);
extern BT_RET_Type bt_set_name(const uint8_t *name, uint8_t wait_ok, uint8_t wait_ticks);
extern void bt_disconnect(void);
extern void bt_get_info(uint32_t wait_tciks);
extern BT_RET_Type bt_control_trandport(uint8_t open, uint8_t wait_ticks);


extern void bt_idle_handler(void);

#endif

