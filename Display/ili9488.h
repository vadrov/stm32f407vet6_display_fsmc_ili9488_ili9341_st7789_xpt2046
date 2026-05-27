/*
 *	Драйвер управления дисплеями
 *  Author: VadRov
 *  Copyright (C) 2019 - 2022, VadRov, all right reserved.
 *
 *  Допускается свободное распространение.
 *  При любом способе распространения указание автора ОБЯЗАТЕЛЬНО.
 *  В случае внесения изменений и распространения модификаций указание первоначального автора ОБЯЗАТЕЛЬНО.
 *  Распространяется по типу "как есть", то есть использование осуществляется на свой страх и риск.
 *  Автор не предоставляет никаких гарантий.
 *
 *  Версия: 2.0 F4/FSMC
 *
 *  https://www.youtube.com/@VadRov
 *  https://dzen.ru/vadrov
 *  https://vk.com/vadrov
 *  https://t.me/vadrov_channel
 */

#ifndef INC_ILI9488_H_
#define INC_ILI9488_H_

#include "display.h"

#define ILI9488_CONTROLLER_WIDTH	320
#define ILI9488_CONTROLLER_HEIGHT	480

//команды контроллера ILI9488
#define	ILI9488_NOP					0x00
#define ILI9488_SOFTRESET			0x01
#define ILI9488_READID				0x04
#define ILI9488_READSTATUS			0x09
#define ILI9488_READPOWERMODE		0x0A
#define ILI9488_READMADCTL			0x0B
#define ILI9488_READPIXELFORMAT		0x0C
#define ILI9488_READIMAGEFORMAT		0x0D
#define ILI9488_READSIGNALMODE		0x0E
#define ILI9488_READSELFDIAGNOSTIC	0x0F
#define ILI9488_SLEEPIN				0x10
#define ILI9488_SLEEPOUT			0x11
#define ILI9488_PARTIALMODE			0x12
#define ILI9488_NORMALDISP			0x13
#define ILI9488_INVERTOFF			0x20
#define ILI9488_INVERTON			0x21
#define ILI9488_GAMMASET			0x26
#define ILI9488_DISPLAYOFF			0x28
#define ILI9488_DISPLAYON			0x29
#define ILI9488_COLADDRSET			0x2A
#define ILI9488_PAGEADDRSET			0x2B
#define ILI9488_MEMORYWRITE			0x2C
#define ILI9488_COLORSET			0x2D
#define ILI9488_MEMORYREAD			0x2E
#define ILI9488_PARTIALAREA			0x30
#define ILI9488_VERTICALSCROLING	0x33
#define ILI9488_TEARINGEFFECTOFF	0x34
#define ILI9488_TEARINGEFFECTON		0x35
#define ILI9488_MEMCONTROL			0x36
#define ILI9488_VSCROLLSTARTADDRESS	0x37
#define ILI9488_IDLEMODEOFF			0x38
#define ILI9488_IDLEMODEON			0x39
#define ILI9488_PIXELFORMAT			0x3A
#define ILI9488_WRITEMEMCONTINUE	0x3C
#define ILI9488_READMEMCONTINUE		0x3E
#define ILI9488_SETSCANLINE			0x44
#define ILI9488_GETSCANLINE			0x45
#define ILI9488_WRITEBRIGHTNESS		0x51
#define ILI9488_READBRIGHTNESS		0x52
#define ILI9488_WRITECTRL			0x53
#define ILI9488_READCTRL			0x54
#define ILI9488_WRITECABC			0x55
#define ILI9488_READCABC			0x56
#define ILI9488_WRITECABCMIN		0x5E
#define ILI9488_READCABCMIN			0x5F
#define ILI9488_RGBSIGNALCONTROL	0xB0
#define ILI9488_FRAMECONTROLNORMAL	0xB1
#define ILI9488_FRAMECONTROLIDLE	0xB2
#define ILI9488_FRAMECONTROLPARTIAL	0xB3
#define ILI9488_INVERSIONCONTROL	0xB4
#define ILI9488_BLANKINGPORCHCONT	0xB5
#define ILI9488_DISPLAYFUNC			0xB6
#define ILI9488_ENTRYMODE			0xB7
#define ILI9488_BACKLIGHTCONTROL1	0xB8
#define ILI9488_BACKLIGHTCONTROL2	0xB9
#define ILI9488_BACKLIGHTCONTROL3	0xBA
#define ILI9488_BACKLIGHTCONTROL4	0xBB
#define ILI9488_BACKLIGHTCONTROL5	0xBC
#define ILI9488_BACKLIGHTCONTROL7	0xBE
#define ILI9488_BACKLIGHTCONTROL8	0xBF
#define ILI9488_POWERCONTROL1		0xC0
#define ILI9488_POWERCONTROL2		0xC1
#define ILI9488_VCOMCONTROL1		0xC5
#define ILI9488_VCOMCONTROL2		0xC7
#define ILI9488_POWERCONTROLA		0xCB
#define ILI9488_POWERCONTROLB		0xCF
#define ILI9488_NVMEMORYWRITE		0xD0
#define ILI9488_NVMEMORYKEY			0xD1
#define ILI9488_NVMEMORYSTATUSREAD	0xD2
#define ILI9488_READID4				0xD3
#define ILI9488_READID1				0xDA
#define ILI9488_READID2				0xDB
#define ILI9488_READID3				0xDC
#define ILI9488_POSITIVEGAMMCORR	0xE0
#define ILI9488_NEGATIVEGAMMCORR	0xE1
#define ILI9488_DIGITALGAMMCONTROL1	0xE2
#define ILI9488_DIGITALGAMMCONTROL2	0xE3
#define ILI9488_DRIVERTIMCONTROLA	0xE8
#define ILI9488_SETIMAGE			0xE9
#define ILI9488_DRIVERTIMCONTROLC	0xEA
#define ILI9488_POWERSEQCONTROL		0xED
#define ILI9488_ENABLE3G			0xF2
#define ILI9488_INTERFACECONTROL	0xF6
#define ILI9488_PUMPRATIOCONTROL	0xF7
#define ILI9488_SPIREADCOMMAND		0xFB
#define ILI9488_ADJUSTCONTROL6		0xFC
#define ILI9488_ADJUSTCONTROL7		0xFF

//параметры конфигурации памяти дисплея
#define ILI9488_MADCTL_MY			0x80 //бит D7
#define ILI9488_MADCTL_MX			0x40 //бит D6
#define ILI9488_MADCTL_MV			0x20 //бит D5
#define ILI9488_MADCTL_ML			0x10 //бит D4
#define ILI9488_MADCTL_BGR			0x08 //бит D3
#define ILI9488_MADCTL_MH			0x04 //бит D2

//режимы цвета
#define ILI9488_COLOR_MODE_3bit		0x01 //RGB111 (8 color)
#define ILI9488_COLOR_MODE_16bit	0x55 //RGB565 (16bit color) //not support for SPI
#define ILI9488_COLOR_MODE_18bit	0x66 //RGB666 (18bit color)

const uint8_t* ILI9488_Init(void);
const uint8_t* ILI9488_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
const uint8_t* ILI9488_SleepIn(void);
const uint8_t* ILI9488_SleepOut(void);
const uint8_t* ILI9488_SetOrientation(LCD_PageOrientation orientation);

#endif /* INC_ILI9488_H_ */
