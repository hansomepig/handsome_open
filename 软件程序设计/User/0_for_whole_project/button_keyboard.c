#include "for_whole_project.h"

// 回显区域光标位置、数据
#define ECHO_AREA_X_BEGIN 10
#define ECHO_AREA_Y_BEGIN 10
static uint16_t cursor_x = ECHO_AREA_X_BEGIN, cursor_y = ECHO_AREA_Y_BEGIN;
static uint16_t echo_area_size, echo_area_max_size = PROJ_PASSWD_LEN;
static uint8_t echo_area_data[PROJ_PASSWD_LEN+1];
// 管理员模式标志
static uint8_t admin_mode;

PROJ_RET_Typedef press_callback(uint32_t arg)
{
    // x,y 方向上字与字之间的间距
    static uint16_t x_gap = PROJ_ILI9341_FONT_WIDTH/2;
    static uint16_t y_gap = PROJ_ILI9341_FONT_HEIGHT/2;

    // x 方向上字的个数
    static uint16_t x_text_size;
    x_text_size = (ILI9341_X_Scale - 2*ECHO_AREA_X_BEGIN) / (PROJ_ILI9341_FONT_WIDTH + x_gap);

    // 进入基本临界区, 确保LCD能够正常显示
    vPortEnterCritical();
    // BaseType_t xAlreadyYielded = pdFALSE;
    // vTaskSuspendAll();
    do {
        if(echo_area_size == 0) {
            // 清除回显区域
            proj_clear_echo_area();
            delay_us(10000);
        }

        if(arg<9) {           // 按键1-9
            if(echo_area_size >= echo_area_max_size)       // 密码已满
                break;

            if((PROJ_SHOW_PASSWD != 0) || (admin_mode != 0)) {
                ILI9341_WriteChar(cursor_x, cursor_y, arg+'1', ILI9341_BLACK, ILI9341_WHITE);
            } else {
                ILI9341_WriteChar(cursor_x, cursor_y, '*', ILI9341_BLACK, ILI9341_WHITE);
            }
            delay_us(1000);

            echo_area_data[echo_area_size++] = arg+1;

            if( echo_area_size % x_text_size == 0 ) {           // 换行
                cursor_x = ECHO_AREA_X_BEGIN;
                cursor_y += y_gap + PROJ_ILI9341_FONT_HEIGHT;
            } else {                                        // 继续写
                cursor_x += PROJ_ILI9341_FONT_WIDTH + x_gap;
            }
        } else if(arg == 9) {                 // 按键'X'(退格)
            if( echo_area_size == 0 ) {
                break;
            }

            if( cursor_x == ECHO_AREA_X_BEGIN ) {           // 回到最右边
                cursor_x = ILI9341_X_Scale - ECHO_AREA_X_BEGIN - PROJ_ILI9341_FONT_WIDTH;
                cursor_y -= y_gap + PROJ_ILI9341_FONT_HEIGHT;
            } else {                                        // 回退
                cursor_x -= PROJ_ILI9341_FONT_WIDTH + x_gap;
            }

            ILI9341_WriteChar(cursor_x, cursor_y, ' ', ILI9341_WHITE, ILI9341_WHITE);
            // ILI9341_DrawFillRectangle(cursor_x, cursor_y, 
            //         PROJ_ILI9341_FONT_WIDTH, PROJ_ILI9341_FONT_HEIGHT, ILI9341_WHITE);
            delay_us(1000);
            echo_area_size--;

        } else if(arg == 10) {                // 按键0
            if(echo_area_size >= echo_area_max_size)       // 密码已满
                break;

            if((PROJ_SHOW_PASSWD != 0) || (admin_mode != 0)) {
                ILI9341_WriteChar(cursor_x, cursor_y, arg+'1', ILI9341_BLACK, ILI9341_WHITE);
            } else {
                ILI9341_WriteChar(cursor_x, cursor_y, '*', ILI9341_BLACK, ILI9341_WHITE);
            }
            delay_us(1000);

            echo_area_data[echo_area_size++] = 0;

            if( echo_area_size % x_text_size == 0 ) {           // 换行
                cursor_x = ECHO_AREA_X_BEGIN;
                cursor_y += y_gap + PROJ_ILI9341_FONT_HEIGHT;
            } else {                                        // 继续写
                cursor_x += PROJ_ILI9341_FONT_WIDTH + x_gap;
            }
        } else if(arg == 11) {                  // 按键'E'(Enter, 确认键)
            for(int i=0; i<echo_area_size; i++) {
                printf("%d", echo_area_data[i]);
            }
            printf("\r\n");

            // 清除回显区域
            proj_clear_echo_area();
            delay_us(10000);

            if(admin_mode != 0) {
                uint16_t cmd = 0;
                uint16_t pwr = 1;
                for(uint8_t i = echo_area_size - 1; ; i--) {
                    cmd += echo_area_data[i] * pwr;

                    if(i == 0) {
                        break;
                    }

                    pwr *= 10;
                }

                vPortExitCritical();
                admin_handler(cmd, &admin_mode, echo_area_data, &echo_area_max_size);
                vPortEnterCritical();
                // echo_area_max_size = PROJ_PASSWD_LEN;
                proj_clear_echo_data();
                break;
            }

            // 比对密码是否正确
            PROJ_RET_Typedef ret;
            ret = proj_check_passwd(echo_area_data, echo_area_size);

            if(ret != PROJ_OK) {        // 数据传输错误
                printf("请查看硬件配置\r\n");
            }

            if( echo_area_data[echo_area_max_size] == 0 ) {        // 是管理员密码
                printf("管理员密码正确\r\n");
                admin_mode = 1;     // 进入管理员模式
                proj_clear_echo_area();
                Set_CurrentFont(&Font_7x10);
                ILI9341_WriteString(7, 5,  "1:Open Lock   2:Add U_Pwd", ILI9341_RED, ILI9341_WHITE);
                ILI9341_WriteString(7, 20, "3:Add Fp      4:Add RFID Card", ILI9341_RED, ILI9341_WHITE);
                ILI9341_WriteString(7, 35, "5:Del U_Pwd   6:Change A_Pwd", ILI9341_RED, ILI9341_WHITE);
                ILI9341_WriteString(7, 50, "7:Del Fp      8:Del RFID Card", ILI9341_RED, ILI9341_WHITE);
                delay_us(10000);
                Set_CurrentFont(&Font_11x18);
                echo_area_max_size = 1;
            } else if( echo_area_data[echo_area_max_size] == 1 ) {  // 是用户密码
                printf("用户密码正确\r\n");
                // 发送开锁信号
                xSemaphoreGive(OpenLock_Semaphore);
            } else {                     // 密码错误
            }

            proj_clear_echo_data();
            
        } else {
            vPortExitCritical();
            return PROJ_ERROR;
        }
    } while(0);
    vPortExitCritical();
    // xAlreadyYielded = xTaskResumeAll();

    // if( xAlreadyYielded == pdFALSE ) {
    //     portYIELD_WITHIN_API();
    // } else {
    //     mtCOVERAGE_TEST_MARKER();
    // }
    
    // printf("按下按键%d\r\n", arg);
    return PROJ_OK;
}

PROJ_RET_Typedef release_callback(uint32_t arg)
{
    printf("松开按键%d\r\n", arg);
    return PROJ_OK;
}



/**
 * @brief  显示按键面板
 * @param  press    按键按下回调函数
 * @param  release  按键释放回调函数
 * @return 错误码
 */
XPT2046_Buuton_TypeDef XPT2046_Button[12];
// PROJ_RET_Typedef proj_show_keyboard(XPT2046_Button_Callback_Typedef press, XPT2046_Button_Callback_Typedef release)
PROJ_RET_Typedef proj_show_keyboard(void)
{
#define ILI9341_BEGIN_Y ILI9341_Y_Scale/6
// #define ILI9341_RECTANGLE_FUNC   ILI9341_DrawFillRectangle
#define ILI9341_RECTANGLE_FUNC   ILI9341_DrawRectangle

    PROJ_RET_Typedef ret;
    uint16_t x, y, w, h;
    uint16_t x_gap, y_gap;
    uint16_t text_x, text_y;
    int cnt = 0;

    w = ILI9341_X_Scale/4;
    h = ILI9341_Y_Scale/9;

    x_gap = (ILI9341_X_Scale - 3*w) / 4;
    y_gap = (ILI9341_Y_Scale*2/3 - 4*h) / 5;

    text_x = w*2/5;
    text_y = h/4;
    
    // 创建按键
    for(int i=0; i<4; i++) {
        y = ILI9341_BEGIN_Y + i*(h + y_gap) + y_gap;
        x = x_gap;

        for(int j=0; j<3; j++, cnt++) {
            
            XPT2046_Button[cnt].x = x;
            XPT2046_Button[cnt].y = y;
            XPT2046_Button[cnt].w = w;
            XPT2046_Button[cnt].h = h;
            XPT2046_Button[cnt].value = cnt;
            XPT2046_Button[cnt].press = press_callback;
            XPT2046_Button[cnt].release = release_callback;

            ret = XPT2046_Create_Button(&XPT2046_Button[cnt]);
            if(ret != PROJ_OK) {
                do {
                    XPT2046_Button_Delete(&XPT2046_Button[cnt]);
                } while(cnt--);

                // printf("创建按键失败\r\n");
                
                return ret;
            }
            
            x += w + x_gap;
        }
    }

    // 显示按键1-9
    for(cnt=0; cnt<9; cnt++) {
        x = XPT2046_Button[cnt].x;
        y = XPT2046_Button[cnt].y;

        ILI9341_RECTANGLE_FUNC(x, y, w, h, ILI9341_BLACK);
        ILI9341_WriteChar(x+text_x, y+text_y, cnt+'1', ILI9341_BLACK, ILI9341_WHITE);
    }

    // 显示按键X,0,E
    y = XPT2046_Button[cnt].y;

    x = XPT2046_Button[cnt++].x;
    ILI9341_RECTANGLE_FUNC(x, y, w, h, ILI9341_BLACK);
    ILI9341_WriteChar(x+text_x, y+text_y, 'X', ILI9341_BLACK, ILI9341_WHITE);

    x = XPT2046_Button[cnt++].x;
    ILI9341_RECTANGLE_FUNC(x, y, w, h, ILI9341_BLACK);
    ILI9341_WriteChar(x+text_x, y+text_y, '0', ILI9341_BLACK, ILI9341_WHITE);

    x = XPT2046_Button[cnt++].x;
    ILI9341_RECTANGLE_FUNC(x, y, w, h, ILI9341_BLACK);
    ILI9341_WriteChar(x+text_x, y+text_y, 'E', ILI9341_BLACK, ILI9341_WHITE);

    return PROJ_OK;
}

PROJ_RET_Typedef proj_show_keyboard_again(void)
{
    uint16_t x, y, w, h;
    uint16_t text_x, text_y;
    int cnt = 0;

    w = ILI9341_X_Scale/4;
    h = ILI9341_Y_Scale/9;

    text_x = w*2/5;
    text_y = h/4;

    // 显示按键1-9
    for(cnt=0; cnt<9; cnt++) {
        x = XPT2046_Button[cnt].x;
        y = XPT2046_Button[cnt].y;

        ILI9341_RECTANGLE_FUNC(x, y, w, h, ILI9341_BLACK);
        ILI9341_WriteChar(x+text_x, y+text_y, cnt+'1', ILI9341_BLACK, ILI9341_WHITE);
    }

    // 显示按键X,0,E
    y = XPT2046_Button[cnt].y;

    x = XPT2046_Button[cnt++].x;
    ILI9341_RECTANGLE_FUNC(x, y, w, h, ILI9341_BLACK);
    ILI9341_WriteChar(x+text_x, y+text_y, 'X', ILI9341_BLACK, ILI9341_WHITE);

    x = XPT2046_Button[cnt++].x;
    ILI9341_RECTANGLE_FUNC(x, y, w, h, ILI9341_BLACK);
    ILI9341_WriteChar(x+text_x, y+text_y, '0', ILI9341_BLACK, ILI9341_WHITE);

    x = XPT2046_Button[cnt++].x;
    ILI9341_RECTANGLE_FUNC(x, y, w, h, ILI9341_BLACK);
    ILI9341_WriteChar(x+text_x, y+text_y, 'E', ILI9341_BLACK, ILI9341_WHITE);

    return PROJ_OK;
}


/**
 * @brief  清除回显区域
 * @return 无
 */
void proj_clear_echo_area(void)
{
    cursor_x = ECHO_AREA_X_BEGIN;
    cursor_y = ECHO_AREA_Y_BEGIN;
    ILI9341_DrawFillRectangle(0, 0, ILI9341_X_Scale, ILI9341_BEGIN_Y+7, ILI9341_WHITE);
}

void proj_clear_echo_data(void)
{
    while(echo_area_size) {
        echo_area_data[--echo_area_size] = 0;
    }
}


