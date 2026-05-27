/*
 *  Author: VadRov
 *  Copyright (C) 2022 - 2023, VadRov, all right reserved.
 *
 *	Сопряжение библиотеки TJpgDec с драйвером дисплея
 *
 *  Допускается свободное распространение.
 *  При любом способе распространения указание автора ОБЯЗАТЕЛЬНО.
 *  В случае внесения изменений и распространения модификаций указание первоначального автора ОБЯЗАТЕЛЬНО.
 *  Распространяется по типу "как есть", то есть использование осуществляется на свой страх и риск.
 *  Автор не предоставляет никаких гарантий.
 *
 *  https://www.youtube.com/@VadRov
 *  https://dzen.ru/vadrov
 *  https://vk.com/vadrov
 *  https://t.me/vadrov_channel
 */

//#include "ff.h" //Подключение библиотеки FatFS, если предусмотрено проектом
#include "jpeg_chan.h"
#include "tjpgd.h"
#include "display.h"

typedef struct {
	uint8_t *data;
	uint32_t data_left;
} Data_Mem_src;

uint8_t work_buffer[JPEG_CHAN_WORK_BUFFER_SIZE];

//Формат цвета AVI: 1 - R5G6B5 цветной, 3 - R5G6B5 оттенки серого.
uint8_t JPEG_color_mode = 1;

#if defined(FF_DEFINED) //Если подключена библиотека FatFS
//Получение данных с внешнего носителя - sd карты (файл jpeg на sd карте)
static inline unsigned int tjd_input_file (JDEC* jd, uint8_t* buff, unsigned int nd)
{
	IODEV *iodev = (IODEV *)jd->device;
	FIL *file = (FIL *)((iFile_jpg *)iodev->file)->file;
	jd = jd;
	if (((iFile_jpg *)iodev->file)->size < nd) {
		nd = ((iFile_jpg *)iodev->file)->size;
		((iFile_jpg *)iodev->file)->size = 0;
	}
	else {
		((iFile_jpg *)iodev->file)->size -= nd;
	}
	if (buff) {
		UINT read_b = 0;
		f_read(file, buff, nd, &read_b);
		return read_b;
	}
	else {
		FRESULT res = f_lseek(file, f_tell(file) + (FSIZE_t)nd);
		return res == FR_OK ? nd : 0;
	}
}
#endif

//Получение данных из памяти (файл jpeg во Flash или RAM МК)
static inline unsigned int tjd_input_memory (JDEC* jd, uint8_t* buff, unsigned int nd)
{
	IODEV *iodev = (IODEV *)jd->device;
	if (((Data_Mem_src*)iodev->file)->data_left < nd) {
		nd = ((Data_Mem_src*)iodev->file)->data_left;
	}
	if (buff) {
		memcpy(buff, ((Data_Mem_src*)iodev->file)->data, nd);
	}
	((Data_Mem_src*)iodev->file)->data += nd;
	((Data_Mem_src*)iodev->file)->data_left -= nd;
	return nd;
}

//Вывод блока памяти bitmap на дисплей в указанной позиции rect
static inline int tjd_output (JDEC* jd, void* bitmap, JRECT* rect)
{
	IODEV *iodev = (IODEV *)jd->device;
	//Вывод блока на дисплей
	LCD_DrawImage(iodev->lcd, rect->left + iodev->x_offs, rect->top + iodev->y_offs, rect->right - rect->left + 1,
			      rect->bottom - rect->top + 1, (uint16_t*)bitmap, 1);
	return JDR_OK;
}

//Вывод jpeg изображения на дисплей.
//location определяет местоположение файла (на sd карте или во Flash/RAM МК)
uint8_t LCD_Load_JPG_chan (LCD_Handler *lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, void *image_stream, PictureLocation location)
{
	JDEC jd;
	JRESULT rc = JDR_PAR;
	uint8_t scale;

	IODEV iodev;
	iodev.lcd = lcd;
	iodev.x_offs = x;
	iodev.y_offs = y;
	if (location == PICTURE_IN_MEMORY)	{
		Data_Mem_src data;
		data.data = (uint8_t *)((iPicture_jpg*)image_stream)->data;
		data.data_left = ((iPicture_jpg*)image_stream)->size;
		iodev.file = (void *)&data;
		rc = jd_prepare(&jd, tjd_input_memory, work_buffer, JPEG_CHAN_WORK_BUFFER_SIZE, &iodev);
	}
#if defined(FF_DEFINED)
	else if (location == PICTURE_IN_FILE) {
		iodev.file = image_stream;
		rc = jd_prepare(&jd, tjd_input_file, work_buffer, JPEG_CHAN_WORK_BUFFER_SIZE, &iodev);
	}
#endif
	else {
		return JDR_INP;
	}
	if (rc == JDR_OK) {
		for (scale = 0; scale < 3; scale++) {
			if ((jd.width >> scale) <= w && (jd.height >> scale) <= h) break;
		}
		rc = jd_decomp(&jd, tjd_output, scale);
	}
	return rc;
}
