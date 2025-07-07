/* vim: set ai et ts=4 sw=4: */
#include "stm32f1xx_hal.h"
#include "ili9341.h"
#include "string.h"
#include "stdio.h"

uint16_t ILI9341_X_Scale, ILI9341_Y_Scale;
uint16_t ILI9341_ScanMode;

/* Static functions for fundamental instructions */
static void ILI9341_Reset() {
    HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ILI9341_RES_GPIO_Port, ILI9341_RES_Pin, GPIO_PIN_SET);
}

static void ILI9341_WriteCommand(uint16_t cmd) {
    ILI9341_WRITE_REG = cmd;
}

static void ILI9341_WriteData(uint16_t data) {
    ILI9341_WRITE_DATA = data;
}

static void ILI9341_WriteDataMultiple(uint16_t * datas, uint32_t dataNums) {
    while (dataNums--)
    {
        ILI9341_WRITE_DATA = *datas++;
    }
}

static void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // column address set
    ILI9341_WriteCommand(0x2A); // CASET
    {
        ILI9341_WriteData(x0 >> 8);
        ILI9341_WriteData(x0 & 0x00FF);
        ILI9341_WriteData(x1 >> 8);
        ILI9341_WriteData(x1 & 0x00FF);
    }
    // row address set
    ILI9341_WriteCommand(0x2B); // RASET
    {
        ILI9341_WriteData(y0 >> 8);
        ILI9341_WriteData(y0 & 0x00FF);
        ILI9341_WriteData(y1 >> 8);
        ILI9341_WriteData(y1 & 0x00FF);
    }
    // write to RAM
    ILI9341_WriteCommand(0x2C); // RAMWR
}


/**
 * @brief Control the BackLight of the LCD
 * @param state -> Enable or Disable
 * @return none
 */
void ILI9341_BackLight_Control(FunctionalState state) {
    if (state) {
        HAL_GPIO_WritePin(ILI9341_BL_GPIO_Port, ILI9341_BL_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(ILI9341_BL_GPIO_Port, ILI9341_BL_Pin, GPIO_PIN_RESET);
    }
}

/**
 * @brief Initialize the ili9341 controller
 * @param none
 * @return none
 */
void ILI9341_Init() {
    ILI9341_Reset();
    // command list is based on https://github.com/martnak/STM32-ILI9341
    // SOFTWARE RESET
    ILI9341_WriteCommand(0x01);
    HAL_Delay(500);
        
    // POWER CONTROL A
    ILI9341_WriteCommand(0xCB);
    {
        uint16_t data[] = { 0x39, 0x2C, 0x00, 0x34, 0x02 };
        ILI9341_WriteDataMultiple(data, 5);
    }

    // POWER CONTROL B
    ILI9341_WriteCommand(0xCF);
    {
        uint16_t data[] = { 0x00, 0xC1, 0x30 };
        ILI9341_WriteDataMultiple(data, 3);
    }

    // DRIVER TIMING CONTROL A
    ILI9341_WriteCommand(0xE8);
    {
        uint16_t data[] = { 0x85, 0x00, 0x78 };
        ILI9341_WriteDataMultiple(data, 3);
    }

    // DRIVER TIMING CONTROL B
    ILI9341_WriteCommand(0xEA);
    {
        uint16_t data[] = { 0x00, 0x00 };
        ILI9341_WriteDataMultiple(data, 2);
    }

    // POWER ON SEQUENCE CONTROL
    ILI9341_WriteCommand(0xED);
    {
        uint16_t data[] = { 0x04, 0x03, 0x12, 0x81 };
        ILI9341_WriteDataMultiple(data, 4);
    }

    // PUMP RATIO CONTROL
    ILI9341_WriteCommand(0xF7);
    {
        uint16_t data[] = { 0x20 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // POWER CONTROL,VRH[5:0]
    ILI9341_WriteCommand(0xC0);
    {
        uint16_t data[] = { 0x23 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // POWER CONTROL,SAP[2:0];BT[3:0]
    ILI9341_WriteCommand(0xC1);
    {
        uint16_t data[] = { 0x10 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // VCM CONTROL
    ILI9341_WriteCommand(0xC5);
    {
        uint16_t data[] = { 0x3E, 0x28 };
        ILI9341_WriteDataMultiple(data, 2);
    }

    // VCM CONTROL 2
    ILI9341_WriteCommand(0xC7);
    {
        uint16_t data[] = { 0x86 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // MEMORY ACCESS CONTROL
    ILI9341_WriteCommand(0x36);
    {
        uint16_t data[] = { 0x48 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // PIXEL FORMAT
    ILI9341_WriteCommand(0x3A);
    {
        uint16_t data[] = { 0x55 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // FRAME RATIO CONTROL, STANDARD RGB COLOR
    ILI9341_WriteCommand(0xB1);
    {
        uint16_t data[] = { 0x00, 0x18 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // DISPLAY FUNCTION CONTROL
    ILI9341_WriteCommand(0xB6);
    {
        uint16_t data[] = { 0x08, 0x82, 0x27 };
        ILI9341_WriteDataMultiple(data, 3);
    }

    // 3GAMMA FUNCTION DISABLE
    ILI9341_WriteCommand(0xF2);
    {
        uint16_t data[] = { 0x00 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // GAMMA CURVE SELECTED
    ILI9341_WriteCommand(0x26);
    {
        uint16_t data[] = { 0x01 };
        ILI9341_WriteDataMultiple(data, 1);
    }

    // POSITIVE GAMMA CORRECTION
    ILI9341_WriteCommand(0xE0);
    {
        uint16_t data[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                           0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
        ILI9341_WriteDataMultiple(data, 15);
    }

    // NEGATIVE GAMMA CORRECTION
    ILI9341_WriteCommand(0xE1);
    {
        uint16_t data[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                           0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
        ILI9341_WriteDataMultiple(data, 15);
    }

    // EXIT SLEEP
    ILI9341_WriteCommand(0x11);
    HAL_Delay(120);

    // TURN ON DISPLAY
    ILI9341_WriteCommand(0x29);

    // Set the default orientation
    ILI9341_SetOrientation(2);
}

/**
 * @brief  设置 ILI9341 的坐标系方向
 * @param  mode ：选择坐标系方向
 *     @arg 0-7 :参数可选值为0-7这八个方向
 *
 *	！！！其中0、3、5、6 模式适合从左至右显示文字，
 *				不推荐使用其它模式显示文字	其它模式显示文字会有镜像效果			
 *		
 *	其中0、2、4、6 模式的X方向像素为240，Y方向像素为320
 *	其中1、3、5、7 模式下X方向像素为320，Y方向像素为240
 *
 *	其中 6 模式为大部分液晶例程的默认显示方向
 *	其中 3 模式为摄像头例程使用的方向
 *	其中 0 模式为BMP图片显示例程使用的方向
 * 
 * 
 * 参数 mode 的第一位表示是否翻转 X、Y 轴
 * 第二位表示 X 轴的增加方向
 * 第三位表示 Y 轴的增加方向
 *
 * @retval 无
 * @note   箭头所指方向为坐标值增加的方向
 * 
 * 
 * -------------------------------------------------------------
 * 模式0:  000    | 模式2:  010   | 模式4:  100   | 模式6:  110
 *           ^   |           ^   |          Y   |          Y
 *           |   |           |   |          |   |          |
 *           Y   |           Y   |          v   |          v
 *    <--- X     |    X --->     |   <--- X     |   X --->
 * -------------------------------------------------------------
 * 模式1:  001    | 模式3:  011   | 模式5:  101   | 模式7:  111
 *           ^   |           ^   |          X   |           X
 *           |   |           |   |          |   |           |
 *           X   |           X   |          v   |           v
 *    <--- Y     |    Y --->     |   <--- Y     |   Y --->
 * -------------------------------------------------------------
 * 
 * 
 * 使用 API 函数 ILI9341_Init 初始化后 ILI9341 默认为模式 2, 此时lcd屏幕展示如下
 * 
 * 
 * 240-1    ILI9341_WIDTH      0
 * x_max                     x_min
 * -------------------------------
 * |                             | y_max 320-1
 * |                             |
 * |                             |
 * |                             |
 * |                             |
 * |                             |
 * |                             |
 * |                             |
 * |                             |   ILI9341_HEIGHT
 * |                             |
 * |                             |
 * |                             |
 * |                             |
 * |                             |
 * |                             |
 * |                             |
 * |                             | y_min  0
 * -------------------------------
 * ||||||||||||||||||||||||||||||
 *                排针
 * 
 * 
 *  
 */
void ILI9341_SetOrientation(uint8_t mode) {
    ILI9341_ScanMode = mode;

    // 根据模式更新XY方向的像素宽度
	if(mode % 2 == 0)   // XY轴不翻转
	{
		//0 2 4 6模式下, X方向像素大小为屏幕高度240，Y方向像素大小为屏幕宽度320
		ILI9341_X_Scale = ILI9341_WIDTH;
		ILI9341_Y_Scale =	ILI9341_HEIGHT;
	} else {        // XY轴翻转
		//1 3 5 7模式下, X方向像素大小为屏幕宽度320，Y方向像素大小为屏幕高度240
		ILI9341_X_Scale = ILI9341_HEIGHT;
		ILI9341_Y_Scale =	ILI9341_WIDTH;
	}

    // MADCTL
    ILI9341_WriteCommand(0x36);
    ILI9341_WriteData( ILI9341_MADCTL_BGR | (mode << 5) );

    // Set column address scale
    ILI9341_WriteCommand(0x2A);
    {
        uint16_t data[] = { 0x00, 0x00, (ILI9341_X_Scale-1) >> 8, (ILI9341_X_Scale-1) & 0xFF };
        ILI9341_WriteDataMultiple(data, 4);
    }

    // Set page(line) address scale
    ILI9341_WriteCommand(0x2B);
    {
        uint16_t data[] = { 0x00, 0x00, (ILI9341_Y_Scale-1) >> 8, (ILI9341_Y_Scale-1) & 0xFF };
        ILI9341_WriteDataMultiple(data, 4);
    }

    // write to RAM
    ILI9341_WriteCommand(0x2C);

}

/**
 * @brief Draw a pixel on screen
 * @param x, y -> Coordinates to draw
 * @param color -> color to draw
 * @return none
 */
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= ILI9341_X_Scale) || (y >= ILI9341_Y_Scale))
        return;
    ILI9341_SetAddressWindow(x, y, x+1, y+1);
    ILI9341_WriteData(color);
}


/*
 * Some problem occurred...Deserted temporarily
static void ILI9341_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;
    ILI9341_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);
    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if( (b << j) & 0x8000)  {
                ILI9341_WriteData(color);
            } else {
                ILI9341_WriteData(bgcolor);
            }
        }
    }
}
 */

/**
 * @brief Write a character to screen
 * @param x, y -> Coordinates to write
 * @param ch -> character to write
 * @param color -> color of the character
 * @param bgcolor -> Background color of the character
 * @return none
 */
void ILI9341_WriteChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bgcolor)
{
    uint32_t i, b, j;
    FontDef font = *CurrentFont;

    ILI9341_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);
    for (i = 0; i < font.height; i++)
    {
        b = font.data[(ch-32) * font.height + i];
        for (j = 0; j < font.width; j++)
        {
            if ((b << j) & 0x8000)
                ILI9341_DrawPixel(x+j, y+i, color);

            else
                ILI9341_DrawPixel(x+j, y+i, bgcolor);
        }
    }
}

/**
 * @brief Write string to screen
 * @param x, y -> Coordinates to write
 * @param str -> string to write
 * @param color -> color of the string
 * @param bgcolor -> Background color of the string
 * @return none
 */
void ILI9341_WriteString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bgcolor) {
    FontDef font = *CurrentFont;

    while(*str) {
        if(x + font.width >= ILI9341_X_Scale) {
            x = 0;
            y += font.height;
            if(y + font.height >= ILI9341_Y_Scale) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        ILI9341_WriteChar(x, y, *str, color, bgcolor);
        x += font.width;
        str++;
    }
}

void ILI9341_WriteLine(int x, int y, const char* str, uint16_t color, uint16_t bgcolor) {
    FontDef font = *CurrentFont;

    while(*str) {
        if(x + font.width >= ILI9341_X_Scale)
            break;

        ILI9341_WriteChar(x, y, *str, color, bgcolor);
        x += font.width;
        str++;
    }

    while(x + font.width < ILI9341_X_Scale) {
        ILI9341_WriteChar(x, y, ' ', color, bgcolor);
        x += font.width;
    }
}


/**
 * @brief Fill screen with single color
 * @param color -> color to fill with
 * @return none
 */
void ILI9341_FillScreen(uint16_t color) {
    uint16_t x, y;

    ILI9341_SetAddressWindow(0, 0, ILI9341_X_Scale-1, ILI9341_Y_Scale-1);
    for(y = ILI9341_Y_Scale; y > 0; y--) {
        for(x = ILI9341_X_Scale; x > 0; x--) {
            ILI9341_WriteData(color);
        }
    }
}


/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ILI9341_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                     uint16_t color) {
    uint16_t swap;
    uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
        swap = x0;
        x0 = y0;
        y0 = swap;

        swap = x1;
        x1 = y1;
        y1 = swap;
        //_swap_int16_t(x0, y0);
        //_swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        swap = x0;
        x0 = x1;
        x1 = swap;

        swap = y0;
        y0 = y1;
        y1 = swap;
        //_swap_int16_t(x0, x1);
        //_swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = ABS(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            ILI9341_DrawPixel(y0, x0, color);
        } else {
            ILI9341_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * @brief Fill a Rect-Area with single color
 * @param x, y -> Coordinates to start
 * @param w, h -> Width & Height of the Rect.
 * @param color -> color of the Rect.
 * @return none
 */
void ILI9341_DrawFillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= ILI9341_X_Scale) || (y >= ILI9341_Y_Scale)) return;
    if((x + w - 1) >= ILI9341_X_Scale) w = ILI9341_X_Scale - x;
    if((y + h - 1) >= ILI9341_Y_Scale) h = ILI9341_Y_Scale - y;

    ILI9341_SetAddressWindow(x, y, x+w-1, y+h-1);
    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            ILI9341_WriteData(color);
        }
    }
}

/**
 * @brief Draw a Rectangle with single color
 * @param x, y -> Coordinates to start
 * @param w, h -> Width & Height of the Rect.
 * @param color -> color of the Rect.
 * @return none
 */
void ILI9341_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= ILI9341_X_Scale) || (y >= ILI9341_Y_Scale)) return;
    if((x + w - 1) >= ILI9341_X_Scale) w = ILI9341_X_Scale - x;
    if((y + h - 1) >= ILI9341_Y_Scale) h = ILI9341_Y_Scale - y;

    ILI9341_DrawLine(x, y, x+w-1, y, color); // top
    ILI9341_DrawLine(x, y+h-1, x+w-1, y+h-1, color); // bottom
    ILI9341_DrawLine(x, y, x, y+h-1, color); // left
    ILI9341_DrawLine(x+w-1, y, x+w-1, y+h-1, color); // right
}

/**
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ILI9341_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    ILI9341_DrawPixel(x0, y0 + r, color);
    ILI9341_DrawPixel(x0, y0 - r, color);
    ILI9341_DrawPixel(x0 + r, y0, color);
    ILI9341_DrawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        ILI9341_DrawPixel(x0 + x, y0 + y, color);
        ILI9341_DrawPixel(x0 - x, y0 + y, color);
        ILI9341_DrawPixel(x0 + x, y0 - y, color);
        ILI9341_DrawPixel(x0 - x, y0 - y, color);

        ILI9341_DrawPixel(x0 + y, y0 + x, color);
        ILI9341_DrawPixel(x0 - y, y0 + x, color);
        ILI9341_DrawPixel(x0 + y, y0 - x, color);
        ILI9341_DrawPixel(x0 - y, y0 - x, color);
    }
}

/**
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ILI9341_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    ILI9341_DrawPixel(x0, y0 + r, color);
    ILI9341_DrawPixel(x0, y0 - r, color);
    ILI9341_DrawPixel(x0 + r, y0, color);
    ILI9341_DrawPixel(x0 - r, y0, color);
    ILI9341_DrawLine(x0 - r, y0, x0 + r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        ILI9341_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
        ILI9341_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

        ILI9341_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
        ILI9341_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
    }
}

/**
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ILI9341_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                            uint16_t x3, uint16_t y3, uint16_t color)
{
    /* Draw lines */
    ILI9341_DrawLine(x1, y1, x2, y2, color);
    ILI9341_DrawLine(x2, y2, x3, y3, color);
    ILI9341_DrawLine(x3, y3, x1, y1, color);
}

/**
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ILI9341_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                                uint16_t x3, uint16_t y3, uint16_t color)
{
    int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
            yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
            curpixel = 0;

    deltax = ABS(x2 - x1);
    deltay = ABS(y2 - y1);
    x = x1;
    y = y1;

    if (x2 >= x1) {
        xinc1 = 1;
        xinc2 = 1;
    }
    else {
        xinc1 = -1;
        xinc2 = -1;
    }

    if (y2 >= y1) {
        yinc1 = 1;
        yinc2 = 1;
    }
    else {
        yinc1 = -1;
        yinc2 = -1;
    }

    if (deltax >= deltay) {
        xinc1 = 0;
        yinc2 = 0;
        den = deltax;
        num = deltax / 2;
        numadd = deltay;
        numpixels = deltax;
    }
    else {
        xinc2 = 0;
        yinc1 = 0;
        den = deltay;
        num = deltay / 2;
        numadd = deltax;
        numpixels = deltay;
    }

    for (curpixel = 0; curpixel <= numpixels; curpixel++) {
        ILI9341_DrawLine(x, y, x3, y3, color);

        num += numadd;
        if (num >= den) {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
    }
}

/**
 * @brief Draw a Cross with single color
 * @param x, y -> Coordinate of the center
 * @param size -> Size of the cross
 * @param color -> Color of the cross
 * @return none
 */
void ILI9341_DrawCross(uint16_t x, uint16_t y, uint16_t size, uint16_t color) {
    ILI9341_DrawLine(x-size, y     , x+size, y     , color);
    ILI9341_DrawLine(x     , y-size, x     , y+size, color);
}
    
/**
 * @brief Draw an image on the screen
 * @param x, y -> Coordinate of the image's top-left dot (where to start)
 * @param w, h -> Width & Height of the image
 * @param data -> Must be '(uint16_t *)data' ,the image data array
 * @return none
 */
void ILI9341_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if((x >= ILI9341_X_Scale) || (y >= ILI9341_Y_Scale)) return;
    if((x + w - 1) >= ILI9341_X_Scale) return;
    if((y + h - 1) >= ILI9341_Y_Scale) return;

    ILI9341_SetAddressWindow(x, y, x+w-1, y+h-1);
    ILI9341_WriteDataMultiple((uint16_t*)data, w*h);
}

/**
 * @brief Invert screen color
 * @param invert -> Invert or not
 * @return none
 */
void ILI9341_InvertColors(bool invert) {
    ILI9341_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
}

/**
 * @brief Simple test function for almost all functions
 * @param none
 * @return none
 */
void ILI9341_Test(void)
{
    Set_CurrentFont(&Font_11x18);
    ILI9341_WriteString(10, 50, "SUCCESSFULLY", ILI9341_RED, ILI9341_WHITE);
    ILI9341_DrawCross(100, 100, 5, ILI9341_RED);
    ILI9341_DrawCross(200, 100, 5, ILI9341_GREEN);
    HAL_Delay(2000);
    ILI9341_FillScreen(ILI9341_RED);
    HAL_Delay(1000);
    ILI9341_FillScreen(ILI9341_BLUE);
    HAL_Delay(1000);
    ILI9341_FillScreen(ILI9341_GREEN);
    HAL_Delay(1000);
    ILI9341_FillScreen(ILI9341_YELLOW);
    HAL_Delay(1000);
    ILI9341_FillScreen(ILI9341_CYAN);
    HAL_Delay(1000);
    ILI9341_FillScreen(ILI9341_MAGENTA);
    HAL_Delay(1000);
    ILI9341_FillScreen(ILI9341_WHITE);
    HAL_Delay(1000);
    ILI9341_FillScreen(ILI9341_BLACK);
    HAL_Delay(2000);

    ILI9341_DrawLine(10, 10, 30, 30, ILI9341_WHITE);
    HAL_Delay(1500);
    ILI9341_FillScreen(ILI9341_BLACK);

    ILI9341_DrawRectangle(10, 10, 30, 20, ILI9341_WHITE);
    HAL_Delay(1500);
    ILI9341_FillScreen(ILI9341_BLACK);

    ILI9341_DrawFillRectangle(10, 10, 30, 20, ILI9341_WHITE);
    HAL_Delay(1500);
    ILI9341_FillScreen(ILI9341_BLACK);

    ILI9341_DrawCircle(100, 100, 50, ILI9341_WHITE);
    HAL_Delay(1500);
    ILI9341_FillScreen(ILI9341_BLACK);

    ILI9341_DrawFilledCircle(100, 100, 50, ILI9341_WHITE);
    HAL_Delay(1500);
    ILI9341_FillScreen(ILI9341_BLACK);

    ILI9341_DrawTriangle(10, 10, 50, 50, 100, 10, ILI9341_WHITE);
    HAL_Delay(1500);
    ILI9341_FillScreen(ILI9341_BLACK);

    ILI9341_DrawFilledTriangle(10, 10, 50, 50, 100, 10, ILI9341_WHITE);
    HAL_Delay(1500);
    ILI9341_FillScreen(ILI9341_WHITE);
    HAL_Delay(1000);
}

/**
 * @brief Simple FPS(Frams Per Seconds) test function (not so reliable...)
 * @param none
 * @return none
 */
void FPS_Test(void)
{
    uint32_t start = HAL_GetTick();
    uint32_t end = start;
    int fps = 0;
    char message[] = "ABCDEFGHJK";

    ILI9341_FillScreen(ILI9341_WHITE);
    Set_CurrentFont(&Font_11x18);
    do {
        ILI9341_WriteString(10, 10, message, ILI9341_RED, ILI9341_WHITE);

        char ch = message[0];
        memmove(message, message+1, sizeof(message)-2);
        message[sizeof(message)-2] = ch;

        fps++;
        end = HAL_GetTick();
    } while ((end - start) < 5000);

    HAL_Delay(1000);
    char buff[64];
    fps = (float) fps / ((end - start) / 1000.0);
    snprintf(buff, sizeof(buff), "~%d FPS", fps);

    ILI9341_FillScreen(ILI9341_WHITE);
    ILI9341_WriteString(10, 10, buff, ILI9341_RED, ILI9341_WHITE);
    HAL_Delay(3000);
}

