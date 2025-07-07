/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "w25q64.h"

/* Definitions of physical drive number for each drive */
#define SD_CARD 0
#define SPI_FLASH 1


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
	case SPI_FLASH:
		if( SPI_FLASH_ReadID() == sFLASH_ID ) {
			stat = 0;
		} else {
			stat = STA_NOINIT;
		}
		return stat;

	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
	case SPI_FLASH:
		SPI_FLASH_Init();
		HAL_Delay(1);
		SPI_Flash_WAKEUP();
		stat = disk_status(SPI_FLASH);
		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    uint16_t ss;
    
	switch (pdrv) {
	case SPI_FLASH:
		
		disk_ioctl(pdrv, GET_SECTOR_SIZE, &ss);
		// 前2MB空间（512个扇区）用于原始地读写数据, 常用于存储字模等大容量数据
		sector += 512;
		// 计算扇区地址
		uint32_t sectorAddr = sector * ss;
		SPI_FLASH_BufferRead(buff, sectorAddr, count*ss);

		return RES_OK;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    DRESULT res;

	switch (pdrv) {
	case SPI_FLASH:;
        uint16_t ss;
		disk_ioctl(pdrv, GET_SECTOR_SIZE, &ss);
		// 前2MB空间（512个扇区）用于原始地读写数据, 常用于存储字模等大容量数据
		sector += 512;
		// 计算扇区地址
		uint32_t sectorAddr = sector * ss;
		SPI_FLASH_SectorErase(sectorAddr);
		SPI_FLASH_BufferWrite((uint8_t *)buff, sectorAddr, count*ss);

		res = RES_OK;
		return res;

	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;

	switch (pdrv) {
	case SPI_FLASH:
		res = RES_OK;
		switch(cmd) {
			case GET_SECTOR_COUNT:
				/** 前面两 2MB 用于记录目录表等信息, 因此可用于存储数据的扇区数量为
				* 128 * 16 - 512 = 1536
				*/
				*(LBA_t *)buff = (LBA_t)(1536);
				break;
			case GET_SECTOR_SIZE:
				/** 每个扇区大小为 4KB = 4096B */
				*(DWORD *)buff = (DWORD)(4096);
				break;
			case GET_BLOCK_SIZE:
				/** 同时擦除扇区个数，单位为扇区 */
				*(DWORD * )buff = 1;
				break;
			case CTRL_SYNC:
				/** 同步缓存 */
				break;

			default:
				res = RES_PARERR;
		}

		return res;

	}

	return RES_PARERR;
}

#include "rtc.h"
__weak DWORD get_fattime(void) {
    RTC_DateTypeDef GetDate = {0};
    RTC_TimeTypeDef GetTime = {0};
    
    /* Get the RTC current Time */
    HAL_RTC_GetTime(&hrtc, &GetTime, RTC_FORMAT_BIN);
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&hrtc, &GetDate, RTC_FORMAT_BIN);
    
    /* 返回当前时间戳 */
    return	  ((DWORD)(GetDate.Year - 1980) << 25)	/* Year */
            | ((DWORD)GetDate.Month << 21)			/* Month */
            | ((DWORD)GetDate.Date << 16)			/* Mday */
            | ((DWORD)GetTime.Hours << 11)			/* Hour */
            | ((DWORD)GetTime.Minutes << 5)			/* Min */
            | ((DWORD)GetTime.Seconds >> 1);		/* Sec */
}

