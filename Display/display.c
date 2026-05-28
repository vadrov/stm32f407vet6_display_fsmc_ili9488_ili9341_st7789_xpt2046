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
 *
 */

#include <string.h>
#include <stdlib.h>
#include "display.h"
#include "fonts.h"

//#define LCD_SWAP_BYTES

#define ABS(x) ((x) > 0 ? (x) : -(x))

#define min(x1,x2)	(x1 < x2 ? x1 : x2)
#define max(x1,x2)	(x1 > x2 ? x1 : x2)

#define min3(x1,x2,x3)	min(min(x1,x2),x3)
#define max3(x1,x2,x3)	max(max(x1,x2),x3)

LCD_Handler *LCD = 0; //Список дисплеев

//Коллбэк по прерыванию потока передачи
//этот обработчик необходимо прописать в функциях обработки прерываний в потоках DMA,
//которые используются дисплеями - stm32f4xx_it.c
void Display_TC_Callback(DMA_TypeDef *dma_x, uint32_t stream)
{
	uint8_t shift[8] = {0, 6, 16, 22, 0, 6, 16, 22}; //битовое смещение во флаговом регистре IFCR (L и H)
	volatile uint32_t *ifcr_tx = (stream > 3) ? &(dma_x->HIFCR) : &(dma_x->LIFCR);
	*ifcr_tx = 0x3F << shift[stream]; //сбрасываем флаги прерываний
	uint32_t stream_ct = 0;
	DMA_TypeDef *dma_ct = 0;
	LCD_Handler *lcd = LCD; //указатель на первый дисплей в списке
	while (lcd) { //проходим по списку дисплеев (пока есть следующий в списке)
		//получаем параметры DMA потока дисплея
		dma_ct = lcd->fsmc_data.dma;
		stream_ct = lcd->fsmc_data.dma_stream;
		//проверка на соответствие текущего потока DMA потоку, к которому привязан i-тый дисплей
		if (dma_ct == dma_x && stream_ct == stream && !lcd->dma_complete) {
			//указатель на поток: aдрес контроллера + смещение
			DMA_Stream_TypeDef *dma_TX = (DMA_Stream_TypeDef *)((uint32_t)((uint32_t)dma_x + 24 * stream + 16));
			//dma_TX->CR &= ~DMA_SxCR_EN; //выключаем поток DMA
			//while (dma_TX->CR & DMA_SxCR_EN) ; //ждем отключения потока
			if (lcd->size_mem) { //если переданы не все данные из памяти, то перезапускаем DMA и выходим из прерывания
				if (lcd->size_mem > 65535) {
					dma_TX->NDTR = 65535;
					lcd->size_mem -= 65535;
				}
				else {
					dma_TX->NDTR = lcd->size_mem;
					lcd->size_mem = 0;
				}
				//включаем поток DMA
				dma_TX->CR |= DMA_SxCR_EN;
				return;
			}
			lcd->dma_complete = 1;
			return;
		}
		//переходим к следующему дисплею в списке
		lcd = (LCD_Handler *)lcd->next;
	}
}

void LCD_Delay(uint32_t Delay)
{
	(void)SysTick->CTRL;
	if (Delay < 0xFFFFFFFFU) {
		Delay++;
	}
	while (Delay) {
		if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U) {
			Delay--;
		}
	}
}

/*
 * Интерпретатор командных строк дисплея
 * Выполняет заданную последовательность команд с параметрами (или без них)
 */
void LCD_String_Interpretator(LCD_Handler* lcd, const uint8_t *str)
{
	uint8_t cmd, par_num;
	while(!lcd->dma_complete) ;
	while (1) {
		cmd = *str++;
		par_num = *str++;
		if (par_num == 255) break; 	//Конец командной строки
		if (par_num >= 20)	{		//Пауза, если количество параметров >= 20
			LCD_Delay(par_num);
			continue;
		}
		//Отправляем команду
		*((volatile uint8_t *)lcd->fsmc_data.cmd_addr) = cmd;
		//Отправляем параметры команды (если они есть)
		while (par_num) {
			*((volatile uint8_t *)lcd->fsmc_data.data_addr) = *str++;
			par_num--;
		}
	}
}

/*
 * Аппаратный сброс дисплея, если задан порт и пин, к которому подключен вывод
 * RESET/RST дисплея.
 */
void LCD_HardWareReset (LCD_Handler* lcd)
{
	if (lcd->fsmc_data.reset_port) {
		lcd->fsmc_data.reset_port->BSRR = (uint32_t)lcd->fsmc_data.reset_pin << 16U;
		LCD_Delay(50);
		lcd->fsmc_data.reset_port->BSRR = lcd->fsmc_data.reset_pin;
		LCD_Delay(50);
	}
}

/*
 * Устанавливает ориентацию страницы дисплея:
 * PAGE_ORIENTATION_PORTRAIT - портрет (по умолчанию при инициализации дисплея)
 * PAGE_ORIENTATION_LANDSCAPE - пейзаж
 * PAGE_ORIENTATION_PORTRAIT_MIRROR - портрет перевернуто
 * PAGE_ORIENTATION_LANDSCAPE_MIRROR - пейзаж перевернуто
 */
void LCD_SetOrientation(LCD_Handler* lcd, LCD_PageOrientation orientation)
{
	if (!lcd->SetOrientation_callback) return;
	uint16_t max_res = max(lcd->Width, lcd->Height);
	uint16_t min_res = min(lcd->Width, lcd->Height);
	if (orientation == PAGE_ORIENTATION_PORTRAIT || orientation == PAGE_ORIENTATION_PORTRAIT_MIRROR) {
		lcd->Width = min_res;
		lcd->Height = max_res;
		lcd->Width_Controller = lcd->w_cntrl;
		lcd->Height_Controller = lcd->h_cntrl;
		if (orientation == PAGE_ORIENTATION_PORTRAIT) {
			lcd->x_offs = lcd->w_offs;
			lcd->y_offs = lcd->h_offs;
		}
		else {
			lcd->x_offs = lcd->Width_Controller - lcd->Width - lcd->w_offs;
			lcd->y_offs = lcd->Height_Controller - lcd->Height - lcd->h_offs;
		}
	}
	else if (orientation == PAGE_ORIENTATION_LANDSCAPE || orientation == PAGE_ORIENTATION_LANDSCAPE_MIRROR)	{
		lcd->Width = max_res;
		lcd->Height = min_res;
		lcd->Width_Controller = lcd->h_cntrl;
		lcd->Height_Controller = lcd->w_cntrl;
		if (orientation == PAGE_ORIENTATION_LANDSCAPE) {
			lcd->x_offs = lcd->h_offs;
			lcd->y_offs = lcd->Height_Controller - lcd->Height - lcd->w_offs;
		}
		else {
			lcd->x_offs = lcd->Width_Controller - lcd->Width - lcd->h_offs;
			lcd->y_offs = lcd->w_offs;
		}
	}
	else return;
	LCD_String_Interpretator(lcd, lcd->SetOrientation_callback(orientation));
	lcd->Orientation = orientation;
}

//Создание обработчика дисплея и добавление его в список дисплеев lcds (LCD - объявлен, как глобальная переменная)
//Возвращает указатель на созданный обработчик либо 0 при неудаче
LCD_Handler* LCD_DisplayAdd(LCD_Handler *lcds,
							uint16_t resolution1,
							uint16_t resolution2,
							uint16_t width_controller,
							uint16_t height_controller,
							int w_offs,
							int h_offs,
							DisplayInitCallback init,
							DisplaySetWindowCallback set_win,
							DisplaySleepInCallback sleep_in,
							DisplaySleepOutCallback sleep_out,
							DisplaySetOrientationCallback set_orientation,
							void *connection_data,
							LCD_BackLight_data bkl_data
					   )
{
	LCD_Handler* lcd = (LCD_Handler*)calloc(1, sizeof(LCD_Handler));
	if (!lcd) return 0;
	//инициализация данных подключения
	lcd->fsmc_data = *((LCD_FSMC_Connected_data*)connection_data);
	//настройка DMA
	if (lcd->fsmc_data.dma) {
		DMA_Stream_TypeDef *dma_x = (DMA_Stream_TypeDef *)((uint32_t)((uint32_t)lcd->fsmc_data.dma + 24 * lcd->fsmc_data.dma_stream + 16));
		dma_x->CR &= ~DMA_SxCR_EN; //отключаем канал DMA
		while(dma_x->CR & DMA_SxCR_EN) ; //ждем отключения канала
		//Размер данных памяти источника и приемника - полуслово (16 бит)
		dma_x->CR &= ~(DMA_SxCR_MSIZE | DMA_SxCR_PSIZE);
		dma_x->CR |= DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0;
		dma_x->CR &= ~DMA_SxCR_PINC; //инкремент адреса источника отключен
		dma_x->CR &= ~DMA_SxCR_MINC;  //инкремент адреса приемника отключен
		//запрещаем прерывания по некоторым событиям канала передачи tx и режим двойного буфера
		dma_x->CR &= ~(DMA_SxCR_DMEIE | DMA_SxCR_HTIE | DMA_SxCR_DBM | DMA_SxCR_TEIE);
		dma_x->FCR &= ~DMA_SxFCR_FEIE;
		//разрешаем прерывание по окончанию передачи
		dma_x->CR |= DMA_SxCR_TCIE;
		//адрес приемника
		dma_x->M0AR = lcd->fsmc_data.data_addr;
	}
	//----------- внутренние служебные переменные ------------
	lcd->w_offs = w_offs;
	lcd->h_offs = h_offs;
	lcd->w_cntrl = width_controller;
	lcd->h_cntrl = height_controller;
	//настройка ориентации дисплея и смещения начала координат
	uint16_t max_res = max(resolution1, resolution2);
	uint16_t min_res = min(resolution1, resolution2);
	//------------ default setting screen page orientation -------------
	LCD_PageOrientation orientation = PAGE_ORIENTATION_PORTRAIT;
	lcd->Width = min_res;
	lcd->Height = max_res;
	lcd->Width_Controller = width_controller;
	lcd->Height_Controller = height_controller;
	lcd->x_offs = w_offs;
	lcd->y_offs = h_offs;
	//------------------------------------------------------------------
	if (lcd->Width_Controller < lcd->Width ||
		lcd->Height_Controller < lcd->Height ||
		init == NULL ||
		set_win == NULL )	{
		LCD_Delete(lcd);
		return 0;
	}
	lcd->Orientation = orientation;
	lcd->Init_callback = init;
	lcd->SetActiveWindow_callback = set_win;
	lcd->SleepIn_callback = sleep_in;
	lcd->SleepOut_callback = sleep_out;
	lcd->SetOrientation_callback = set_orientation;
	lcd->bkl_data = bkl_data;
	lcd->display_number = 0;
	lcd->next = 0;
	lcd->prev = 0;
	lcd->dma_complete = 1;
	if (!lcds) {
		return lcd;
	}
	LCD_Handler *prev = lcds;
	while (prev->next) {
		prev = (LCD_Handler *)prev->next;
		lcd->display_number++;
	}
	lcd->prev = (void*)prev;
	prev->next = (void*)lcd;
	return lcd;
}

//Удаляет дисплей
void LCD_Delete(LCD_Handler* lcd)
{
	if (lcd) {
		free(lcd);
	}
}

//Инициализирует дисплей
void LCD_Init(LCD_Handler* lcd)
{
	LCD_HardWareReset(lcd);
	LCD_String_Interpretator(lcd, lcd->Init_callback());
	LCD_SetOrientation(lcd, lcd->Orientation);
	LCD_SetBackLight(lcd, lcd->bkl_data.bk_percent);
}

//Возвращает яркость подсветки, %
inline uint8_t LCD_GetBackLight(LCD_Handler* lcd)
{
	return lcd->bkl_data.bk_percent;
}

//Возвращает ширину дисплея, пиксели
inline uint16_t LCD_GetWidth(LCD_Handler* lcd)
{
	return lcd->Width;
}

//Возвращает высоту дисплея, пиксели
inline uint16_t LCD_GetHeight(LCD_Handler* lcd)
{
	return lcd->Height;
}

//Возвращает статус дисплея: занят либо свободен (требуется для отправки новых данных на дисплей)
//Дисплей занят, если занято spi, к которому он подключен
inline LCD_State LCD_GetState(LCD_Handler* lcd)
{
	if (!lcd->dma_complete) {
		return LCD_STATE_BUSY;
	}
	return LCD_STATE_READY;
}

//Управление подсветкой
void LCD_SetBackLight(LCD_Handler* lcd, uint8_t bk_percent)
{
	if (bk_percent > 100) {
		bk_percent = 100;
	}
	lcd->bkl_data.bk_percent = bk_percent;
	//подсветка с использованием PWM
	if (lcd->bkl_data.tim_bk) {
		//вычисляем % яркости, как часть от периода счетчика
		uint32_t bk_value = lcd->bkl_data.tim_bk->ARR * bk_percent / 100;
		//задаем скважность PWM конкретного канала
		switch(lcd->bkl_data.channel_tim_bk) {
			case 1:
				lcd->bkl_data.tim_bk->CCR1 = bk_value;
				break;
			case 2:
				lcd->bkl_data.tim_bk->CCR2 = bk_value;
				break;
			case 3:
				lcd->bkl_data.tim_bk->CCR3 = bk_value;
				break;
			case 4:
				lcd->bkl_data.tim_bk->CCR4 = bk_value;
				break;
			default:
				break;
		}
		//если таймер не запущен, то запускаем его
		if (!(lcd->bkl_data.tim_bk->CR1 & TIM_CR1_CEN)) {
			//включаем канал
			lcd->bkl_data.tim_bk->CCER |= (1UL << ((lcd->bkl_data.channel_tim_bk - 1) * 4));
			//включаем счетчик
			lcd->bkl_data.tim_bk->CR1 |= TIM_CR1_CEN;
		}
	}
	//подсветка без PWM (просто вкл./выкл.), если таймер с PWM недоступен
	else if (lcd->bkl_data.blk_port) {
		if (bk_percent) {
			lcd->bkl_data.blk_port->BSRR = lcd->bkl_data.blk_pin;
		}
		else {
			lcd->bkl_data.blk_port->BSRR = (uint32_t)lcd->bkl_data.blk_pin << 16U;
		}
	}
}

//Перевод дисплея в "спящий режим" (выключение отображения дисплея и подсветки)
void LCD_SleepIn(LCD_Handler* lcd)
{
	//подсветка с использованием PWM
	if (lcd->bkl_data.tim_bk) {
		//выключаем подсветку, установив нулевую скважность
		switch(lcd->bkl_data.channel_tim_bk) {
			case 1:
				lcd->bkl_data.tim_bk->CCR1 = 0;
				break;
			case 2:
				lcd->bkl_data.tim_bk->CCR2 = 0;
				break;
			case 3:
				lcd->bkl_data.tim_bk->CCR3 = 0;
				break;
			case 4:
				lcd->bkl_data.tim_bk->CCR4 = 0;
				break;
			default:
				break;
		}
	}
	//подсветка без PWM (просто вкл./выкл.), если таймер с PWM недоступен
	else if (lcd->bkl_data.blk_port) {
		lcd->bkl_data.blk_port->BSRR = (uint32_t)lcd->bkl_data.blk_pin << 16U;
	}
	if (lcd->SleepIn_callback) {
		LCD_String_Interpretator(lcd, lcd->SleepIn_callback());
	}
}

//Вывод дисплея из "спящего режима" (включение отображения дисплея и подсветки)
void LCD_SleepOut(LCD_Handler* lcd)
{
	if (lcd->SleepOut_callback) {
		LCD_String_Interpretator(lcd, lcd->SleepOut_callback());
	}
	//включение подсветки
	LCD_SetBackLight(lcd, lcd->bkl_data.bk_percent);
}

/*
 * Устанавливает прямоугольное окно вывода на дисплее
 * lcd - указатель на обработчик дисплея
 * x1, y1, x2, y2 - координаты левого верхнего и правого нижнего углов области дисплея;
 */
void LCD_SetActiveWindow(LCD_Handler* lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	LCD_String_Interpretator(lcd, lcd->SetActiveWindow_callback(x1 + lcd->x_offs, y1 + lcd->y_offs, x2 + lcd->x_offs, y2 + lcd->y_offs));
}

/*
 * Передача данных с DMA
 * lcd - указатель на обработчик дисплея
 * data - указатель на массив
 * len - количество данных для передачи в полусловах (16 бит)
 */
void LCD_WriteDataDMA(LCD_Handler *lcd, uint16_t *data, uint32_t len)
{
	if (!len) return;
	if (lcd->fsmc_data.dma) { //Если DMA доступно
		while (!lcd->dma_complete) ;
		lcd->dma_complete = 0;
		DMA_TypeDef *dma_x = lcd->fsmc_data.dma;
		uint32_t stream = lcd->fsmc_data.dma_stream;
		DMA_Stream_TypeDef *dma_TX = (DMA_Stream_TypeDef *)((uint32_t)((uint32_t)dma_x + 24 * stream + 16));
		uint8_t shift[8] = {0, 6, 16, 22, 0, 6, 16, 22}; //битовое смещение во флаговых регистрах IFCR (L и H)
		volatile uint32_t *ifcr_tx = (stream > 3) ? &(dma_x->HIFCR) : &(dma_x->LIFCR);
		//сбрасываем флаги прерываний tx
		*ifcr_tx = 0x3F << shift[stream];
		dma_TX->PAR = (uint32_t)data; //адрес источника
		dma_TX->M0AR = lcd->fsmc_data.data_addr; //адрес приемника
		//Инкремент адреса источника включен
		dma_TX->CR |= DMA_SxCR_PINC;
		if (len <= 65535) {
			dma_TX->NDTR = len; //размер передаваемых данных
			lcd->size_mem = 0;
		}
		else {
			dma_TX->NDTR = 65535;
			lcd->size_mem = len - 65535;
		}
		dma_TX->CR |= (DMA_SxCR_EN); //старт DMA передачи
	}
	else { //DMA недоступно
		LCD_WriteData(lcd, data, len);
	}
}

/*
 * Передача данных без DMA
 * lcd - указатель на обработчик дисплея
 * data - указатель на массив
 * len - количество данных для передачи в полусловах (16 бит)
 */
void LCD_WriteData(LCD_Handler *lcd, uint16_t *data, uint32_t len)
{
	while (!lcd->dma_complete) ;
	volatile uint16_t *mem = (volatile uint16_t *)lcd->fsmc_data.data_addr;
	while(len) {
		*mem = *data++;
		len--;
	}
}

/*
 * Заливка прямоугольной области дисплея
 * lcd - указатель на обработчик дисплея
 * x1, y1, x2, y2 - координаты левого верхнего и правого нижнего углов области дисплея;
 * color - цвет в формате 0xRRGGBB
 */
void LCD_FillWindow(LCD_Handler* lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color)
{
	uint16_t tmp;
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	if (x1 > lcd->Width - 1 || y1 > lcd->Height - 1) return;
	if (x2 > lcd->Width - 1)  x2 = lcd->Width - 1;
	if (y2 > lcd->Height - 1) y2 = lcd->Height - 1;
	uint32_t len = (x2 - x1 + 1) * (y2 - y1 + 1);
	LCD_SetActiveWindow(lcd, x1, y1, x2, y2);
	uint16_t color16 = LCD_Color_24b_to_16b(lcd, color);
	if (!lcd->fsmc_data.dma) { //DMA запрещено
		volatile uint16_t *mem = (volatile uint16_t *)lcd->fsmc_data.data_addr;
		while (len) {
			*mem = color16;
			len--;
		}
	}
	else { //DMA разрешено
		lcd->dma_complete = 0;
		lcd->fill_color = color16;
		DMA_TypeDef *dma_x = lcd->fsmc_data.dma;
		uint32_t stream = lcd->fsmc_data.dma_stream;
		DMA_Stream_TypeDef *dma_TX = (DMA_Stream_TypeDef *)((uint32_t)((uint32_t)dma_x + 24 * stream + 16));
		uint8_t shift[8] = {0, 6, 16, 22, 0, 6, 16, 22}; //битовое смещение во флаговых регистрах IFCR (L и H)
		volatile uint32_t *ifcr_tx = (stream > 3) ? &(dma_x->HIFCR) : &(dma_x->LIFCR);
		//сбрасываем флаги прерываний tx
		*ifcr_tx = 0x3F << shift[stream];
		dma_TX->PAR = (uint32_t)(&lcd->fill_color); //адрес источника
		dma_TX->M0AR = lcd->fsmc_data.data_addr; //адрес приемника
		//Инкремент адреса источника отключен
		dma_TX->CR &= ~DMA_SxCR_PINC;
		if (len <= 65535) {
			dma_TX->NDTR = len; //размер передаваемых данных
			lcd->size_mem = 0;
		}
		else {
			dma_TX->NDTR = 65535;
			lcd->size_mem = len - 65535;
		}
		dma_TX->CR |= (DMA_SxCR_EN); //старт DMA передачи
	}
}

/*
 * Выводит в заданную область дисплея блок памяти (изображение) по адресу в data:
 * lcd - указатель на обработчик дисплея
 * x, y - координата левого верхнего угла области дисплея;
 * w, h - ширина и высота области дисплея;
 * data - указатель на блок памяти (изображение) для вывода на дисплей;
 * dma_use_flag - флаг, определяющий задействование DMA (0 - без DMA, !=0 - с DMA)
 */
void LCD_DrawImage(LCD_Handler* lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data, uint8_t dma_use_flag)
{
	if ((x > lcd->Width - 1) || (y > lcd->Height - 1) || x + w > lcd->Width || y + h > lcd->Height) return;
	LCD_SetActiveWindow(lcd, x, y, x + w - 1, y + h - 1);
	if (dma_use_flag && lcd->fsmc_data.dma) {
		LCD_WriteDataDMA(lcd, data, w * h);
	}
	else {
		LCD_WriteData(lcd, data, w * h);
	}
}

/* Закрашивает весь дисплей заданным цветом */
void LCD_Fill(LCD_Handler* lcd, uint32_t color)
{
	LCD_FillWindow(lcd, 0, 0, lcd->Width - 1, lcd->Height - 1, color);
}

/* Рисует точку в заданных координатах */
void LCD_DrawPixel(LCD_Handler* lcd, int16_t x, int16_t y, uint32_t color)
{
	if (x > lcd->Width - 1 || y > lcd->Height - 1 || x < 0 || y < 0) return;
	LCD_SetActiveWindow(lcd, x, y, x, y);
	*((volatile uint16_t *)lcd->fsmc_data.data_addr) = LCD_Color_24b_to_16b(lcd, color);
}

/*
 * Рисует линию по координатам двух точек
 * Горизонтальные и вертикальные линии рисуются быстрее
 */
void LCD_DrawLine(LCD_Handler* lcd, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color)
{
	if(x0 == x1 || y0 == y1) {
		int16_t tmp;
		if (x0 > x1) { tmp = x0; x0 = x1; x1 = tmp; }
		if (y0 > y1) { tmp = y0; y0 = y1; y1 = tmp; }
		if (x1 < 0 || x0 > lcd->Width - 1)  return;
		if (y1 < 0 || y0 > lcd->Height - 1) return;
		if (x0 < 0) x0 = 0;
		if (y0 < 0) y0 = 0;
		LCD_FillWindow(lcd, x0, y0, x1, y1, color);
		return;
	}
	int16_t swap;
    uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
		swap = x0;
		x0 = y0;
		y0 = swap;

		swap = x1;
		x1 = y1;
		y1 = swap;
    }

    if (x0 > x1) {
		swap = x0;
		x0 = x1;
		x1 = swap;

		swap = y0;
		y0 = y1;
		y1 = swap;
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
            LCD_DrawPixel(lcd, y0, x0, color);
        } else {
            LCD_DrawPixel(lcd, x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/* Рисует прямоугольник по координатам левого верхнего и правого нижнего углов */
void LCD_DrawRectangle(LCD_Handler* lcd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color)
{
	LCD_DrawLine(lcd, x1, y1, x2, y1, color);
	LCD_DrawLine(lcd, x1, y2, x2, y2, color);
	LCD_DrawLine(lcd, x1, y1, x1, y2, color);
	LCD_DrawLine(lcd, x2, y1, x2, y2, color);
}

/* Рисует закрашенный прямоугольник по координатам левого верхнего и правого нижнего углов */
void LCD_DrawFilledRectangle(LCD_Handler* lcd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color)
{
	int16_t tmp;
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	if (x2 < 0 || x1 > lcd->Width - 1)  return;
	if (y2 < 0 || y1 > lcd->Height - 1) return;
	if (x1 < 0) x1 = 0;
	if (y1 < 0) y1 = 0;
	LCD_FillWindow(lcd, x1, y1, x2, y2, color);
}

/* Рисует треугольник по координатам трех точек */
void LCD_DrawTriangle(LCD_Handler* lcd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint32_t color)
{
	LCD_DrawLine(lcd, x1, y1, x2, y2, color);
	LCD_DrawLine(lcd, x2, y2, x3, y3, color);
	LCD_DrawLine(lcd, x3, y3, x1, y1, color);
}

/* Виды пересечений отрезков */
typedef enum {
	LINES_NO_INTERSECT = 0, //не пересекаются
	LINES_INTERSECT,		//пересекаются
	LINES_MATCH				//совпадают (накладываются)
} INTERSECTION_TYPES;

/*
 * Определение вида пересечения и координат (по оси х) пересечения отрезка с координатами (x1,y1)-(x2,y2)
 * с горизонтальной прямой y = y0
 * Возвращает один из видов пересечения типа INTERSECTION_TYPES, а в переменных x_min, x_max - координату
 * либо диапазон пересечения (если накладываются).
 * В match инкрементирует количество накладываний (считаем результаты со всех нужных вызовов)
 */
static INTERSECTION_TYPES LinesIntersection(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t y0, int16_t *x_min, int16_t *x_max, uint8_t *match)
{
	if (y1 == y2) { //Частный случай - отрезок параллелен оси х
		if (y0 == y1) { //Проверка на совпадение
			*x_min = min(x1, x2);
			*x_max = max(x1, x2);
			(*match)++;
			return LINES_MATCH;
		}
		return LINES_NO_INTERSECT;
	}
	if (x1 == x2) { //Частный случай - отрезок параллелен оси y
		if (min(y1, y2) <= y0 && y0 <= max(y1, y2)) {
			*x_min = *x_max = x1;
			return LINES_INTERSECT;
		}
		return LINES_NO_INTERSECT;
	}
	//Определяем точку пересечения прямых (уравнение прямой получаем из координат точек, задающих отрезок)
	*x_min = *x_max = (x2 - x1) * (y0 - y1) / (y2 - y1) + x1;
	if (min(x1, x2) <= *x_min && *x_min <= max(x1, x2)) { //Если координата x точки пересечения принадлежит отрезку,
		return LINES_INTERSECT;							  //то есть пересечение
	}
	return LINES_NO_INTERSECT;
}

/* Рисует закрашенный треугольник по координатам трех точек */
void LCD_DrawFilledTriangle(LCD_Handler* lcd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint32_t color)
{
	//Сортируем координаты в порядке возрастания y
	int16_t tmp;
	if (y1 > y2) {
		tmp = y1; y1 = y2; y2 = tmp;
		tmp = x1; x1 = x2; x2 = tmp;
	}
	if (y1 > y3) {
		tmp = y1; y1 = y3; y3 = tmp;
		tmp = x1; x1 = x3; x3 = tmp;
	}
	if (y2 > y3) {
		tmp = y2; y2 = y3; y3 = tmp;
		tmp = x2; x2 = x3; x3 = tmp;
	}
	//Проверяем, попадает ли треугольник в область вывода
	if (y1 > lcd->Height - 1 ||	y3 < 0) return;
	int16_t xmin = min3(x1, x2, x3);
	int16_t xmax = max3(x1, x2, x3);
	if (xmax < 0 || xmin > lcd->Width - 1) return;
	uint8_t c_mas, match;
	int16_t x_mas[8], x_min, x_max;
	//"Обрезаем" координаты, выходящие за рабочую область дисплея
	int16_t y_start = y1 < 0 ? 0: y1;
	int16_t y_end = y3 > lcd->Height - 1 ? lcd->Height - 1: y3;
	//Проходим в цикле по точкам диапазона координаты y и ищем пересечение отрезка y = y[i] (где y[i]=y1...y3, 1)
	//со сторонами треугольника
	for (int16_t y = y_start; y < y_end; y++) {
		c_mas = match = 0;
		if (LinesIntersection(x1, y1, x2, y2, y, &x_mas[c_mas], &x_mas[c_mas + 1], &match)) {
			c_mas += 2;
		}
		if (LinesIntersection(x2, y2, x3, y3, y, &x_mas[c_mas], &x_mas[c_mas + 1], &match)) {
			c_mas += 2;
		}
		if (LinesIntersection(x3, y3, x1, y1, y, &x_mas[c_mas], &x_mas[c_mas + 1], &match)) {
			c_mas += 2;
		}
		if (!c_mas) continue;
		x_min = x_max = x_mas[0];
		while (c_mas) {
			x_min = min(x_min, x_mas[c_mas - 2]);
			x_max = max(x_max, x_mas[c_mas - 1]);
			c_mas -= 2;
		}
		LCD_DrawLine(lcd, x_min, y, x_max, y, color);
	}
}

/* Рисует окружность с заданным центром и радиусом */
void LCD_DrawCircle(LCD_Handler* lcd, int16_t x0, int16_t y0, int16_t r, uint32_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	LCD_DrawPixel(lcd, x0, y0 + r, color);
	LCD_DrawPixel(lcd, x0, y0 - r, color);
	LCD_DrawPixel(lcd, x0 + r, y0, color);
	LCD_DrawPixel(lcd, x0 - r, y0, color);

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		LCD_DrawPixel(lcd, x0 + x, y0 + y, color);
		LCD_DrawPixel(lcd, x0 - x, y0 + y, color);
		LCD_DrawPixel(lcd, x0 + x, y0 - y, color);
		LCD_DrawPixel(lcd, x0 - x, y0 - y, color);

		LCD_DrawPixel(lcd, x0 + y, y0 + x, color);
		LCD_DrawPixel(lcd, x0 - y, y0 + x, color);
		LCD_DrawPixel(lcd, x0 + y, y0 - x, color);
		LCD_DrawPixel(lcd, x0 - y, y0 - x, color);
	}
}

/* Рисует закрашенную окружность с заданным центром и радиусом */
void LCD_DrawFilledCircle(LCD_Handler* lcd, int16_t x0, int16_t y0, int16_t r, uint32_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	LCD_DrawLine(lcd, x0 - r, y0, x0 + r, y0, color);

	while (x < y) {
		if (f >= 0)	{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		LCD_DrawLine(lcd, x0 - x, y0 + y, x0 + x, y0 + y, color);
		LCD_DrawLine(lcd, x0 + x, y0 - y, x0 - x, y0 - y, color);

		LCD_DrawLine(lcd, x0 + y, y0 + x, x0 - y, y0 + x, color);
		LCD_DrawLine(lcd, x0 + y, y0 - x, x0 - y, y0 - x, color);
	}
}

/*
 * Вывод на дисплей символа с кодом в ch, с начальными координатами координатам (x, y), шрифтом font, цветом color,
 * цветом окружения bgcolor.
 * modesym - определяет, как выводить символ:
 *    LCD_SYMBOL_PRINT_FAST - быстрый вывод с полным затиранием знакоместа;
 *    LCD_SYMBOL_PRINT_PSETBYPSET - вывод символа по точкам, при этом цвет окружения bgcolor игнорируется (режим наложения).
 * Ширина символа до 32 пикселей (4 байта на строку). Высота символа библиотекой не ограничивается.
 */
void LCD_WriteChar(LCD_Handler* lcd, uint16_t x, uint16_t y, char ch, FontDef *font, uint32_t txcolor, uint32_t bgcolor, LCD_PrintSymbolMode modesym)
{
	int i, j, k;
	uint32_t tmp = 0;
	const uint8_t *b = font->data;
	uint16_t color;
	uint16_t txcolor16 = LCD_Color_24b_to_16b(lcd, txcolor);
	uint16_t bgcolor16 = LCD_Color_24b_to_16b(lcd, bgcolor);
	ch = ch < font->firstcode || ch > font->lastcode ? 0: ch - font->firstcode;
	int bytes_per_line = ((font->width - 1) >> 3) + 1;
	if (bytes_per_line > 4) { //Поддержка ширины символов до 32 пикселей (4 байта на строку)
		return;
	}
	k = 1 << ((bytes_per_line << 3) - 1);
	b += ch * bytes_per_line * font->height;
	//Заполнение блока данными символа без сохранения фона
	if (modesym == LCD_SYMBOL_PRINT_FAST) {
		LCD_SetActiveWindow(lcd, x, y, x + font->width - 1, y + font->height - 1);
		for (i = 0; i < font->height; i++) {
			if (bytes_per_line == 1)      { tmp = *((uint8_t*)b);  }
			else if (bytes_per_line == 2) { tmp = *((uint16_t*)b); }
			else if (bytes_per_line == 3) { tmp = (*((uint8_t*)b)) | ((*((uint8_t*)(b + 1))) << 8) |  ((*((uint8_t*)(b + 2))) << 16); }
			else { tmp = *((uint32_t*)b); }
			b += bytes_per_line;
			for (j = 0; j < font->width; j++) {
				color = (tmp << j) & k ? txcolor16: bgcolor16;
				*((volatile uint16_t *)lcd->fsmc_data.data_addr) = color;
			}
		}
	}
	else { //По точкам с сохранением фона
		for (i = 0; i < font->height; i++) {
			if (bytes_per_line == 1) { tmp = *((uint8_t*)b); }
			else if (bytes_per_line == 2) { tmp = *((uint16_t*)b); }
			else if (bytes_per_line == 3) { tmp = (*((uint8_t*)b)) | ((*((uint8_t*)(b + 1))) << 8) |  ((*((uint8_t*)(b + 2))) << 16); }
			else if (bytes_per_line == 4) { tmp = *((uint32_t*)b); }
			b += bytes_per_line;
			for (j = 0; j < font->width; j++) {
				if ((tmp << j) & k) {
					LCD_DrawPixel(lcd, x + j, y + i, txcolor);
				}
			}
		}
	}
}

/*
 * Вывод на дисплей lcd строки с позиции x, y текста str, шрифтом font, цветом букв color, цветом окружения bgcolor и
 * в режиме modesym - определяет, как выводить текст:
 * LCD_SYMBOL_PRINT_FAST - быстрый вывод с полным затиранием знакоместа
 * LCD_SYMBOL_PRINT_PSETBYPSET - вывод по точкам, при этом цвет окружения bgcolor игнорируется (позволяет накладывать надписи поверх изображений на дисплее)
 */
void LCD_WriteString(LCD_Handler* lcd, uint16_t x, uint16_t y, const char *str, FontDef *font, uint32_t color, uint32_t bgcolor, LCD_PrintSymbolMode modesym)
{
	while (*str) {
		if (x + font->width > lcd->Width) {
			x = 0;
			y += font->height;
			if (y + font->height > lcd->Height) {
				break;
			}
		}
		LCD_WriteChar(lcd, x, y, *str, font, color, bgcolor, modesym);
		x += font->width;
		str++;
	}
	lcd->AtPos.x = x;
	lcd->AtPos.y = y;
}

inline uint16_t LCD_Color (LCD_Handler *lcd, uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
#ifdef LCD_SWAP_BYTES
	color = (color >> 8) | (color << 8);
#endif
	return color;
}

inline uint16_t LCD_Color_24b_to_16b(LCD_Handler *lcd, uint32_t color)
{
	uint8_t r = (color >> 16) & 0xff;
	uint8_t g = (color >> 8) & 0xff;
	uint8_t b = color & 0xff;
	return LCD_Color(lcd, r, g, b);
}
