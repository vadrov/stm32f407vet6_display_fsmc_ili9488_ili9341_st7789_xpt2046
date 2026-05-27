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
 *  Версия: 1.4
 *
 *  https://www.youtube.com/@VadRov
 *  https://dzen.ru/vadrov
 *  https://vk.com/vadrov
 *  https://t.me/vadrov_channel
 */

#include "ILI9488.h"
#include "display.h"

const uint8_t ILI9488_init_str[] = {
		ILI9488_SOFTRESET, 0,
		0, 50, //pause 50 ms
		ILI9488_MEMCONTROL, 1, ILI9488_MADCTL_BGR,
		//ILI9488_POSITIVEGAMMCORR, 15, 0x00, 0x07, 0x10, 0x09, 0x17, 0x0B, 0x41, 0x89, 0x4B, 0x0A, 0x0C, 0x0E, 0x18, 0x1B, 0x0F,
		//ILI9488_NEGATIVEGAMMCORR, 15, 0x00, 0x17, 0x1A, 0x04, 0x0E, 0x06, 0x2F, 0x45, 0x43, 0x02, 0x0A, 0x09, 0x32, 0x36, 0x0F,
		ILI9488_PIXELFORMAT, 1, ILI9488_COLOR_MODE_16bit,
		ILI9488_INVERTON, 0,
		ILI9488_SLEEPOUT, 0,
		0, 120, //pause 120 ms
		ILI9488_DISPLAYON, 0,
		0, 50, //pause 50 ms
		ILI9488_TEARINGEFFECTON, 1, 0,
		0, 255 //eof
};

static uint8_t	ILI9488_setwin_str[]  = {
									ILI9488_COLADDRSET,  4, 0, 0, 0, 0,	//x0, x1
								 	ILI9488_PAGEADDRSET, 4, 0, 0, 0, 0,	//y0, y1
									ILI9488_MEMORYWRITE, 0,
									0, 255 //eof
								};

const uint8_t ILI9488_sleepin_str[]  = {
									ILI9488_SLEEPIN, 0,
									0, 30, //pause 30 ms
									0, 255 //eof
								};

const uint8_t ILI9488_sleepout_str[] = {
									ILI9488_SLEEPOUT, 0,
									0, 120, //pause 120 ms
									0, 255 //eof
								};

static uint8_t ILI9488_orientation_str[] = { ILI9488_MEMCONTROL, 1, 0,	//mem_config (memory data access config)
											 0, 255 };					//eof
static uint8_t ILI9488_MemoryDataAccessControlConfig(uint8_t mirror_x, uint8_t mirror_y, uint8_t exchange_xy, uint8_t mirror_color, uint8_t refresh_v, uint8_t refresh_h)
{
	uint8_t mem_config = 0;
	if (mirror_x)		mem_config |= ILI9488_MADCTL_MX;
	if (mirror_y)		mem_config |= ILI9488_MADCTL_MY;
	if (exchange_xy)	mem_config |= ILI9488_MADCTL_MV;
	if (mirror_color)	mem_config |= ILI9488_MADCTL_BGR;
	if (refresh_v)		mem_config |= ILI9488_MADCTL_ML;
	if (refresh_h)		mem_config |= ILI9488_MADCTL_MH;
	return mem_config;
}

static uint8_t orient_to_memconfig(LCD_PageOrientation orientation)
{
	uint8_t mem_config = 0;
	if (orientation == PAGE_ORIENTATION_PORTRAIT) 				mem_config = ILI9488_MemoryDataAccessControlConfig(1, 0, 0, 1, 0, 0);
	else if (orientation == PAGE_ORIENTATION_PORTRAIT_MIRROR) 	mem_config = ILI9488_MemoryDataAccessControlConfig(0, 1, 0, 1, 1, 1);
	else if (orientation == PAGE_ORIENTATION_LANDSCAPE) 		mem_config = ILI9488_MemoryDataAccessControlConfig(0, 0, 1, 1, 0, 1);
	else if (orientation == PAGE_ORIENTATION_LANDSCAPE_MIRROR) 	mem_config = ILI9488_MemoryDataAccessControlConfig(1, 1, 1, 1, 1, 0);
	return mem_config;
}

const uint8_t* ILI9488_Init(void)
{
	return ILI9488_init_str;
}

inline const uint8_t* ILI9488_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	*((uint16_t*)&ILI9488_setwin_str[2])  = (x0 >> 8) | (x0 << 8);
	*((uint16_t*)&ILI9488_setwin_str[4])  = (x1 >> 8) | (x1 << 8);
	*((uint16_t*)&ILI9488_setwin_str[8])  = (y0 >> 8) | (y0 << 8);
	*((uint16_t*)&ILI9488_setwin_str[10]) = (y1 >> 8) | (y1 << 8);
	return ILI9488_setwin_str;
}

const uint8_t* ILI9488_SleepIn(void)
{
	return ILI9488_sleepin_str;
}

const uint8_t* ILI9488_SleepOut(void)
{
	return ILI9488_sleepout_str;
}

const uint8_t* ILI9488_SetOrientation(LCD_PageOrientation orientation)
{
	ILI9488_orientation_str[2] = orient_to_memconfig(orientation);
	return ILI9488_orientation_str;
}
