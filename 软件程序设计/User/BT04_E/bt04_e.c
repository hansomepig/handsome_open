/**
 * 说明：
 * 1. 使用 API bt_send_cmd_with_ok 进行数据传输时，建议等待时间不低于50ms，否则可能导致超时失败，这受限于硬件
 * 2. BT04_E 蓝牙模块的 KEY 引脚就是复位引脚
 * 3. 数据接收逻辑：
 *     本例程使用串口空闲中断来接收蓝牙模块发送到主机(stm32)的数据，这样有利于不定长数据的接收，
 *     为了可以方便接收多组数据，以防止上一次的数据还没被读取然后就被下一组数据覆盖，本例程hi用了多个缓冲区，
 *     通过宏 BT04_E_RECV_BUFFER_NUM 可以配置缓冲区的数量，通过宏 BT04_E_RECV_BUFFER_SIZE 可以配置每个缓冲区的大小，
 *     并且创建了 BT_RECV_Buffer_Type 类型类整合接收缓冲区的相关信息，
 *     每次空闲中断发生时，索引 bt_recv_buffer_pos 就会指向下一个接收缓冲区，
 *     每发生一次空闲中断，可读取接收缓冲区数量 bt_shouldread_num 就会加 1，通过这个变量可以得知当前是否有数据可以读取
 *     索引 bt_shouldread_buffer_pos 永远指向当前应该读取的缓冲区，
 *     当我们清空数据缓冲区时，这个索引会变为下一个可读的数据缓冲区
 * 
 * 4. 所有使用 API bt_send_cmd_without_ok 的代码，都必须再调用完此函数后进行一定的延时（50ms左右），
 *    以等待蓝牙模块处理完当前的指令，否则后来的指令将可能会被直接抛弃，或者导致接收到的数据发生混乱
 * 
 * 5. 使用 API bt_send_cmd_with_ok 的代码，不是代表后面就不需要延时了，要看你用的是什么指令，
 *    比如关闭透传指令，即使模块返回了 OK 相应，但是其内部可能还在进行相应的处理，因此需要另外再延时一段时间（50ms左右）
 * 
 * 
 */

#include "bt04_e.h"
#include "for_whole_project.h"

#define BT04_E_RST_LOW  HAL_GPIO_WritePin(BT04_E_RST_GPIO_Port, BT04_E_RST_GPIO_PIN, GPIO_PIN_RESET)
#define BT04_E_RST_HIGH HAL_GPIO_WritePin(BT04_E_RST_GPIO_Port, BT04_E_RST_GPIO_PIN, GPIO_PIN_SET)

BT_RECV_Buffer_Type bt_recv_buffer;

/**
 * @brief  向BT模块发送字符串数据，不检查模块的响应
 * @param   
 *  str:要发送的字符串
 *  len:字符串长度，若为 0 则一直发送直到结束符 '\0'
 * @retval 返回实际发送的长度
 */
static uint16_t bt_send_string(const uint8_t *str, uint16_t len)
{
    uint16_t i=0;

    if( len == 0 ) len = 0xFFFF;

    while( (i < len) && (str[i] != '\0') )
        HAL_UART_Transmit(&BT04_E_UART_PORT, (str + i++), 1, BT04_E_TRANSMIT_TIMEOUT_TICKS);

    return i;
}


/**
 * @brief 初始化 BT04_E 蓝牙模块
 */
BT_RET_Type bt_init(void)
{
    bt_recv_buffer.bt_shouldread_num = 0;
    bt_recv_buffer.bt_recv_buffer_pos = bt_recv_buffer.bt_shouldread_buffer_pos = 0;

    // 硬件复位蓝牙模块
    BT04_E_RST_LOW;
    HAL_Delay(300);
    BT04_E_RST_HIGH;
    HAL_Delay(300);

    // 检查是否启动成功
    return bt_send_cmd_with_ok(0, 0, 0, 1000);
}


/**
 * @brief  清除一个可读的数据缓冲区
 * @retval 返回清除了的缓冲区的内容大小
 */
uint8_t bt_clean_recv_buffer(void)
{
    uint8_t i=0;

    if( bt_recv_buffer.bt_shouldread_num > 0 ) {
        bt_recv_buffer.bt_shouldread_num--;
    } else {
        return 0;
    }

    BT_DEBUG("clean buffer: %s\n", bt_recv_buffer.bt_receive_buffer[bt_recv_buffer.bt_shouldread_buffer_pos]);
    
    while(bt_recv_buffer.bt_receive_buffer[i] && (i<BT04_E_RECV_BUFFER_SIZE)) {
        bt_recv_buffer.bt_receive_buffer[bt_recv_buffer.bt_shouldread_buffer_pos][i++] = 0;
    }

    bt_recv_buffer.bt_shouldread_buffer_pos = (bt_recv_buffer.bt_shouldread_buffer_pos + 1) % BT04_E_RECV_BUFFER_NUM;

    return i;
}

/**
 * @brief  清除所有可读的数据缓冲区
 */
void bt_clean_all_recv_buffer(void)
{
    while( bt_recv_buffer.bt_shouldread_num != 0 )
        bt_clean_recv_buffer();
}

/**
 * @brief  接收数据缓冲区中的数据
 * @param  to_newest  是否要获取到最新的接收缓冲区
 * @return 返回缓冲区的地址
 * @note   此函数不会清空读取的接收缓冲区
 */
uint8_t *bt_recv_data(uint8_t to_newest)
{
    uint8_t *ret = NULL;

    if( bt_recv_buffer.bt_shouldread_num > 0 ) {
        if( to_newest ) {
            ret = bt_recv_buffer.bt_receive_buffer[
                (bt_recv_buffer.bt_shouldread_buffer_pos + bt_recv_buffer.bt_shouldread_num - 1)
                % BT04_E_RECV_BUFFER_NUM];
        } else {
            ret = bt_recv_buffer.bt_receive_buffer[bt_recv_buffer.bt_shouldread_buffer_pos];
        }

        // BT_DEBUG("receive data: %s\n", ret);
    }

    return ret;
}

/* 空闲中断回调函数 */
void bt_idle_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    bt_recv_buffer.bt_shouldread_num++;
    if( bt_recv_buffer.bt_shouldread_num >= BT04_E_RECV_BUFFER_NUM ) {
        bt_clean_recv_buffer();
    }

    bt_recv_buffer.bt_recv_buffer_pos = ( bt_recv_buffer.bt_recv_buffer_pos + 1 ) % BT04_E_RECV_BUFFER_NUM;

    BT_DEBUG("bt_recv_buffer_pos: %d, bt_shouldread_num: %d, bt_shouldread_buffer_pos: %d\n",
            bt_recv_buffer.bt_recv_buffer_pos, bt_recv_buffer.bt_shouldread_num, bt_recv_buffer.bt_shouldread_buffer_pos);

    HAL_UARTEx_ReceiveToIdle_IT(&BT04_E_UART_PORT, 
            bt_recv_buffer.bt_receive_buffer[bt_recv_buffer.bt_recv_buffer_pos], 
            BT04_E_RECV_BUFFER_SIZE);

    if(bt_finish_init == 1) {       // 初始化完成
        // 发送蓝牙事件信号
        xSemaphoreGiveFromISR(BT_Semaphore, &xHigherPriorityTaskWoken);

        // 根据返回值执行一次任务调度
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}


/**
 * @brief  向BT模块以透传模式发送字符串数据
 * @param   
 *  str:要发送的字符串
 *  len:字符串长度，若为 0 则一直发送直到结束符 '\0'
 * @retval 返回实际发送的长度
 */
uint16_t bt_send_data(const uint8_t *str, uint16_t len)
{
    return bt_send_string(str, len);
}

/**
  * @brief  向 BT04_E 写入命令，不检查模块的响应
  * @param  command ，要发送的命令
  * @param  arg，命令参数，为0时不带参数，若command也为0时，发送"AT"命令
  * @param  arg_max_len，命令参数的最大长度, 为 0 表示一直发送直到结束符 '\0'
  * @retval 无
  * @note   此函数不会自动清空接收缓冲区，需要调用 bt_clean_recv_buffer() 函数清空接收缓冲区
  */
void bt_send_cmd_without_ok(const uint8_t *command, const uint8_t *arg, uint8_t arg_max_len)
{
    if( command && (command[0] != 0) ) {
        bt_send_string("AT+", 3);
        bt_send_string(command, 0);
        if (arg && arg[0] != 0) {
            bt_send_string(arg, arg_max_len);
        }
        bt_send_string("\r\n", 2);

        BT_DEBUG("Send to BT: AT+%s%s\r\n", command, arg);
    } else {
        bt_send_string("AT\r\n", 4);
        
        BT_DEBUG("Send to BT: AT\r\n");
    }
}

/**
  * @brief  向 BT04_E 模块发送命令并检查OK。只适用于具有OK应答的命令，最长等待 wait_ticks 个滴答周期直到收到OK
  * @param  cmd 命令字符串，不需要加AT、\r\n
  * @param  arg_max_len，命令参数的最大长度, 为 0 表示一直发送直到结束符 '\0'
  * @template  复位命令：	HC05_Send_CMD("AT+RESET",0,0,0);	
  * @retval 0,设置成功;其他,设置失败.
  * @note   此函数不会自动清空接收缓冲区，需要调用 bt_clean_recv_buffer() 函数清空接收缓冲区
  */
BT_RET_Type bt_send_cmd_with_ok(const uint8_t *command, const uint8_t *arg, uint8_t arg_max_len, uint32_t wait_ticks)
{
    uint8_t *buffer;
    uint16_t i=0;
    uint32_t begin_tick = HAL_GetTick();
    BT_RET_Type ret = BT_OK;

    bt_send_cmd_without_ok(command, arg, arg_max_len);

    if( wait_ticks < BT04_E_RECV_MIN_TIMEOUT_TICKS )
        wait_ticks = BT04_E_RECV_MIN_TIMEOUT_TICKS;

    while( HAL_GetTick() - begin_tick <= wait_ticks ) {
        buffer = bt_recv_data(1);
        if(buffer == NULL) {
            continue;
        }

        while(buffer[++i]);
        BT_DEBUG("recv %d byte: %s\n", i, buffer);
        if( (i < 4) || (buffer[i-4] != 'O') || (buffer[i-3] != 'K') ) {
            ret = BT_OTHER_ERR;
        }

        break;
    }

    if( HAL_GetTick() - begin_tick > wait_ticks ) {
        ret = BT_TIMEOUT;
    }
    
    return ret;
}

/**
 * @brief  软件复位蓝牙模块
 * @param  wait_ok 是否等待OK应答
 * @param  wait_ok_ticks 等待OK的最大滴答周期
 * @param  wait_finish 是否等待蓝牙模块复位完成
 * @param  wait_finish_ticks 等待蓝牙模块复位完成的最大滴答周期
 * @retval BT_OK,设置成功;其他,设置失败.
 */
BT_RET_Type bt_soft_reset(uint8_t wait_ok, uint8_t wait_ok_ticks, uint8_t wait_finish, uint8_t wait_finish_ticks)
{
    BT_RET_Type ret = BT_OK;
    uint32_t begin_tick = HAL_GetTick();

    if(wait_ok) {
        ret = bt_send_cmd_with_ok(BT04_E_AT_CMD_SOFT_RESET, 0, 0, wait_ok_ticks);
    } else {
        bt_send_cmd_without_ok(BT04_E_AT_CMD_SOFT_RESET, 0, 0);
    }

    if( (wait_finish == 0) || (ret != BT_OK) ) return ret;

    uint8_t *buffer;
    while( (HAL_GetTick() - begin_tick) <= wait_finish_ticks ) {
        buffer = bt_recv_data(1);
        if(buffer == NULL) {
            continue;
        }

        // buffer == Power On
        if( (buffer[0] == 'P') && (buffer[1] == 'O') && (buffer[2] == 'W') && (buffer[3] == 'E') && 
            (buffer[4] == 'R') && (buffer[5] == ' ') && (buffer[6] == 'O') && (buffer[7] == 'N') ) {
            bt_clean_recv_buffer();
            return BT_OK;
        } else {
            break;
        }
    }

    return BT_OTHER_ERR;
}

/**
 * @brief  设置蓝牙模块的名称
 * @param  name 要设置的名称
 * @param  wait_ok 是否等待OK应答
 * @param  wait_ticks 等待OK的最大滴答周期
 * @retval BT_OK,设置成功;其他,设置失败.
 * @note   蓝牙模块的名称长度不能超过20字节，超过20字节的名称会被截断，且设置完成后需要重启才能生效
 */
#define BT_MAX_NAME_LEN 20
BT_RET_Type bt_set_name(const uint8_t *name, uint8_t wait_ok, uint8_t wait_ticks)
{
    if(wait_ok) {
        return bt_send_cmd_with_ok(BT04_E_AT_CMD_GET_NAME, name, BT_MAX_NAME_LEN, wait_ticks);
    } else {
        bt_send_cmd_without_ok(BT04_E_AT_CMD_GET_NAME, name, BT_MAX_NAME_LEN);
        return BT_OK;
    }
}

/**
 * @brief  断开与蓝牙模块的连接(退出透传模式)
 * @note   此指令只能在透传模式下使用，且只能由串口端发送
 *         此外，如果当前蓝牙模块没有连接到任何设备，则模块会返回错误信息字符串（ERROR=104），但是不会影响到后续使用
 */
void bt_disconnect(void)
{
    bt_send_cmd_without_ok(BT04_E_AT_CMD_DISCONNECT, 0, 0);
}

/**
 * @brief  获取蓝牙模块的版本信息
 * @param  wait_tciks 等待接收的最大滴答周期
 * @retval 无
 */
void bt_get_info(uint32_t wait_tciks)
{
    uint8_t *buffer;
    uint32_t begin_tick = HAL_GetTick();

    bt_clean_all_recv_buffer();

    bt_send_cmd_without_ok(BT04_E_AT_CMD_HELP, 0, 0);

    if(  wait_tciks < BT04_E_RECV_MIN_TIMEOUT_TICKS )
        wait_tciks = BT04_E_RECV_MIN_TIMEOUT_TICKS;

    while( (HAL_GetTick() - begin_tick) <= wait_tciks ) {
        buffer = bt_recv_data(1);
        if(buffer == NULL) {
            continue;
        }

        printf("%s", buffer);
        begin_tick = HAL_GetTick();
        bt_clean_recv_buffer();
    }

    bt_clean_all_recv_buffer();
}

/**
 * @brief  控制蓝牙模块的透传模式
 * @param  open 0,关闭透传模式;1,开启透传模式
 * @retval BT_OK,设置成功;其他,设置失败.
 * @note   如果设置关闭透传后，模块被连接上可以继续响应AT指令，
 *         如连接上后再发送打开透传命令响应完之后则进入透传模式，然后不再响应命令。此指令掉电保存，
 *         此指令只能当模块不处于透传模式时使用
 * 
 *         如果要关闭透传模式，一般需要在收到 OK 后再延时 50ms，等待蓝牙模块处理完毕，
 *         如果没有延时而是直接发送下一条 AT 指令，那蓝牙模块将会忽略后来接到的指令，直到当前处理完毕位置
 */
BT_RET_Type bt_control_trandport(uint8_t open, uint8_t wait_ticks)
{
    BT_RET_Type ret = BT_OK;
    uint8_t arg = '0' + !!open;

    ret = bt_send_cmd_with_ok(BT04_E_AT_CMD_TRANSPORT, &arg, 1, wait_ticks);

    return ret;
}

