#ifndef __ILI9341_H__
#define __ILI9341_H__

#include "main.h"
#include "fonts.h"
#include <stdbool.h>

#define ILI9341_CMD_ADDR ( ( uint32_t ) 0x60000000 )    //  Base data

/* Change this value below if you have multiple devices on FSMC interface
 * and you're not using A16 for ILI9341, e.g:
 * 0x600000000 |  (0x1<<(16+1)) = 0x60020000;
 * And if it's A17, then 0x600000000 | (0x1<<(17+1)) = 0x60040000;
 */
#define ILI9341_DATA_ADDR ( ( uint32_t ) 0x60020000 )   //  LCD Register Select: A16

//  Basic functions to write data via 16-bit parallel interface (FSMC)
#define ILI9341_WRITE_REG ( *(__IO uint16_t *) (ILI9341_CMD_ADDR) )
#define ILI9341_WRITE_DATA ( *(__IO uint16_t *) (ILI9341_DATA_ADDR) )

/********************************************************
 * Pin Maps for LCD and STM32:
 * STM32        |       ILI9341_LCD     |       Comment
 * FSMC_Dx     -->      DBx             |   (16 lines in total)
 * FSMC_NOE    -->      LCD_RD          |   Read
 * FSMC_NWE    -->      LCD_WR          |   Write
 * FSMC_NEx    -->      LCD_CS          |   Chip select
 * FSMC_Ax     -->      LCD_RS          |   Register select (NOT Reset!)
 * User defined reset pin  --> LCD_RST  |   Reset   (See below)
 ********************************************************/

//  Orientation params
#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04


// The x,y orientation scale of the display lcd(ILI9341)
extern uint16_t ILI9341_X_Scale, ILI9341_Y_Scale;
extern uint16_t ILI9341_ScanMode;

// LCD Parameters
#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320


/****************************/

// Color definitions
#define	ILI9341_BLACK   0x0000
#define	ILI9341_BLUE    0x001F
#define	ILI9341_RED     0xF800
#define	ILI9341_GREEN   0x07E0
#define ILI9341_CYAN    0x07FF
#define ILI9341_MAGENTA 0xF81F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_WHITE   0xFFFF
#define ILI9341_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

#define ABS(x) ((x) > 0 ? (x) : -(x))

void ILI9341_BackLight_Control(FunctionalState state);
void ILI9341_Init(void);
void ILI9341_SetOrientation(uint8_t rotation);
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9341_WriteChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bgcolor);
void ILI9341_WriteString(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bgcolor);
void ILI9341_WriteLine(int x, int y, const char* str, uint16_t color, uint16_t bgcolor);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                      uint16_t color);
void ILI9341_DrawFillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ILI9341_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void ILI9341_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void ILI9341_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                            uint16_t x3, uint16_t y3, uint16_t color);
void ILI9341_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                                uint16_t x3, uint16_t y3, uint16_t color);
void ILI9341_DrawCross(uint16_t x, uint16_t y, uint16_t size, uint16_t color);
void ILI9341_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);
void ILI9341_InvertColors(bool invert);
void ILI9341_Test(void);
void FPS_Test(void);


#endif // __ILI9341_H__;
