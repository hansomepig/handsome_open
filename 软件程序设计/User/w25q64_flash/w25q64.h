#ifndef __W25Q64_H
#define __W25Q64_H

#include "main.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
//#define  sFLASH_ID                       0xEF3015     //W25X16
//#define  sFLASH_ID                       0xEF4015	    //W25Q16
#define  sFLASH_ID                        0XEF4017     //W25Q64
//#define  sFLASH_ID                       0XEF4018    //W25Q128


//#define SPI_FLASH_PageSize            4096
#define SPI_FLASH_PageSize              256
#define SPI_FLASH_PerWritePageSize      256

/* Private define ------------------------------------------------------------*/
/*命令定义-开头*******************************/
#define W25X_WriteEnable            0x06 
#define W25X_WriteDisable           0x04 
#define W25X_ReadStatusReg          0x05 
#define W25X_WriteStatusReg         0x01 
#define W25X_ReadData               0x03 
#define W25X_FastReadData           0x0B 
#define W25X_FastReadDual           0x3B 
#define W25X_PageProgram            0x02 
#define W25X_BlockErase             0xD8 
#define W25X_SectorErase            0x20 
#define W25X_ChipErase              0xC7 
#define W25X_PowerDown              0xB9 
#define W25X_ReleasePowerDown	    0xAB 
#define W25X_DeviceID               0xAB 
#define W25X_ManufactDeviceID   	0x90 
#define W25X_JedecDeviceID		    0x9F

#define WIP_Flag                    0x01  /* Write In Progress (WIP) flag */
#define Dummy_Byte                  0xFF
/*命令定义-结尾*******************************/

/* Definition for SPIx Pins */
#define SPIx_SCK_PIN                     GPIO_PIN_13
#define SPIx_SCK_GPIO_PORT               GPIOB

#define SPIx_MISO_PIN                    GPIO_PIN_14
#define SPIx_MISO_GPIO_PORT              GPIOB

#define SPIx_MOSI_PIN                    GPIO_PIN_15
#define SPIx_MOSI_GPIO_PORT              GPIOB

#define FLASH_CS_PIN                     GPIO_PIN_6
#define FLASH_CS_GPIO_PORT               GPIOC


extern SPI_HandleTypeDef hspi2;
#define SpiHandle hspi2


#define	digitalHi(p,i)			    {p->BSRR=i;}			  //设置为高电平		
#define digitalLo(p,i)			    {p->BSRR=(uint32_t)i << 16;}				//输出低电平
#define SPI_FLASH_CS_LOW()          digitalLo(FLASH_CS_GPIO_PORT,FLASH_CS_PIN )
#define SPI_FLASH_CS_HIGH()         digitalHi(FLASH_CS_GPIO_PORT,FLASH_CS_PIN )
/*SPI接口定义-结尾****************************/

/**
 *  起始等待超时时间, 单位为滴答计数周期(毫秒) 
 *  
 * SPIT_LONG_TIMEOUT 为等待写入完成的时间, 
 * SPIT_SHORT_TIMEOUT为等待硬件SPI读写缓冲区可操作所用的时间
 * 
 */
#define SPIT_LONG_TIMEOUT         ((uint32_t)( 1000 ))
#define SPIT_SHORT_TIMEOUT        ((uint32_t)( 10 ))

/*信息输出*/
#undef FLASH_INFO_ON
#ifdef FLASH_INFO_ON
#define FLASH_INFO(fmt,arg...)           printf("<<-FLASH-INFO->> "fmt"\n",##arg)
#endif
/**
 * 另外一种写法
 * #define FLASH_INFO(fmt,...)           printf("<<-FLASH-INFO->> "fmt"\n",##__VA_ARGS__)
 */

#undef FLASH_ERROR_ON
#ifdef FLASH_ERROR_ON
#define FLASH_ERROR(fmt,arg...)          printf("<<-FLASH-ERROR->> "fmt"\n",##arg)
#endif

#undef FLASH_DEBUG_ON
#ifdef FLASH_DEBUG_ON
#define FLASH_DEBUG(fmt,arg...)          printf("<<-FLASH-DEBUG->> [%d]"fmt"\n",__LINE__, ##arg)
#endif


void SPI_FLASH_Init(void);
void SPI_FLASH_SectorErase(uint32_t SectorAddr);
void SPI_FLASH_BulkErase(void);
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
uint32_t SPI_FLASH_ReadID(void);
uint32_t SPI_FLASH_ReadDeviceID(void);
// void SPI_FLASH_StartReadSequence(uint32_t ReadAddr);
void SPI_Flash_PowerDown(void);
void SPI_Flash_WAKEUP(void);


uint8_t SPI_FLASH_ReadByte(void);
uint8_t SPI_FLASH_SendByte(uint8_t byte);
uint16_t SPI_FLASH_SendHalfWord(uint16_t HalfWord);
void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_WriteEnable(void);
void SPI_FLASH_WaitForWriteEnd(void);

#endif /* __SPI_FLASH_H */

