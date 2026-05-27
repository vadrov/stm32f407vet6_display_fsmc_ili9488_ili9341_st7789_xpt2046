/*
 *	Драйвер управления дисплеями
 *  Author: VadRov
 *  Copyright (C) 2019 - 2024, VadRov, all right reserved.
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

#ifndef INC_DISPLAY_H_
#define INC_DISPLAY_H_

#include "main.h"
#include "fonts.h"

//некоторые предопределенные цвета
//формат 0xRRGGBB
#define	COLOR_BLACK			0x000000
#define	COLOR_BLUE			0x0000FF
#define	COLOR_RED			0xFF0000
#define	COLOR_GREEN			0x00FF00
#define COLOR_CYAN			0x00FFFF
#define COLOR_MAGENTA		0xFF00FF
#define COLOR_YELLOW		0xFFFF00
#define COLOR_WHITE			0xFFFFFF
#define COLOR_NAVY			0x000080
#define COLOR_DARKGREEN		0x2F4F2F
#define COLOR_DARKCYAN		0x008B8B
#define COLOR_MAROON		0xB03060
#define COLOR_PURPLE		0x800080
#define COLOR_OLIVE			0x808000
#define COLOR_LIGHTGREY		0xD3D3D3
#define COLOR_DARKGREY		0xA9A9A9
#define COLOR_ORANGE		0xFFA500
#define COLOR_GREENYELLOW	0xADFF2F

//Статусы дисплея
typedef enum {
	LCD_STATE_READY,
	LCD_STATE_BUSY,
	LCD_STATE_ERROR,
	LCD_STATE_UNKNOW
} LCD_State;

//Ориентация страницы дисплея
typedef enum {
	PAGE_ORIENTATION_PORTRAIT,			//портрет - по умолчанию
	PAGE_ORIENTATION_LANDSCAPE,			//пейзаж
	PAGE_ORIENTATION_PORTRAIT_MIRROR,	//портрет перевернуто
	PAGE_ORIENTATION_LANDSCAPE_MIRROR	//пейзаж перевернуто
} LCD_PageOrientation;

//Режимы печати символов
typedef enum {
	LCD_SYMBOL_PRINT_FAST,		//быстрый с затиранием фона
	LCD_SYMBOL_PRINT_PSETBYPSET	//медленный, по точкам, без затирания фона
} LCD_PrintSymbolMode;

//Данные FSMC подключения
typedef struct {
	uint32_t data_addr;
	uint32_t cmd_addr;
	GPIO_TypeDef *reset_port;
	uint32_t reset_pin;
	DMA_TypeDef *dma;
	uint32_t dma_stream;
} LCD_FSMC_Connected_data;

//Подсветка
typedef struct {
	TIM_TypeDef *tim_bk;		//------- для подсветки с PWM:- таймер
	uint32_t channel_tim_bk;	//----------------------------- канал таймера

	GPIO_TypeDef *blk_port;		//просто для включения и выключения подсветки, если htim_bk = 0 (без PWM, определен порт вывода)
	uint32_t blk_pin;			//----------------------------------------------------------------------- пин порта

	uint8_t bk_percent;			//яркость подсветки для режима PWM, %
								//либо 0 - подсветка отключена, > 0 подсветка включена, если если htim_bk = 0 (без PWM, определен порт вывода)
} LCD_BackLight_data;

//Коллбэки
typedef const uint8_t* (*DisplayInitCallback)(void);
typedef const uint8_t* (*DisplaySetWindowCallback)(uint16_t, uint16_t, uint16_t, uint16_t);
typedef const uint8_t* (*DisplaySleepInCallback)(void);
typedef const uint8_t* (*DisplaySleepOutCallback)(void);
typedef const uint8_t* (*DisplaySetOrientationCallback)(LCD_PageOrientation);

//позиция печати
typedef struct {
	uint16_t x;
	uint16_t y;
} LCD_xy_pos;

//Обработчик дисплея
typedef struct {
	uint16_t Width_Controller;    		//Максимальная ширина матрицы, поддерживаемая контроллером дисплея, пиксели
	uint16_t Height_Controller;			//Максимальная высота матрицы, поддерживаемая контроллером дисплея, пиксели
	uint16_t Width;						//Фактическая ширина матрицы используемого дисплея, пиксели
	uint16_t Height;					//Фактическая высота матрицы используемого дисплея, пиксели
	LCD_PageOrientation Orientation;	//Ориентация дисплея
	int x_offs;							//Смещение по x
	int y_offs;							//Смещение по y
	int w_offs, h_offs;					//Внутреннние переменные для
	uint16_t w_cntrl, h_cntrl;			//нужд драйвера (изменение ориентации страницы)
	LCD_xy_pos AtPos;					//Текущая позиция печати символа
	DisplayInitCallback Init_callback;						//Коллбэк инициализации
	DisplaySetWindowCallback SetActiveWindow_callback;		//Коллбэк установки окна вывода
	DisplaySleepInCallback SleepIn_callback;				//Коллбэк "входа в сон"
	DisplaySleepOutCallback SleepOut_callback;				//Коллбэк "выхода из сна"
	DisplaySetOrientationCallback SetOrientation_callback;	//Коллбэк установки ориентации страницы
	LCD_FSMC_Connected_data fsmc_data;	//Данные подключения по FSMC
	LCD_BackLight_data bkl_data;		//Данные подсветки
	uint8_t display_number;				//Номер данного дисплея
	uint16_t fill_color;				//Переменная хранения цвета R5G6B5 при выполнении заливки с DMA
	volatile uint8_t dma_complete;		//Флаг, сигнализирующий, что передача по DMA завершена
	volatile uint32_t size_mem;
	void *prev;							//Указатель на предыдующий дисплей
	void *next;							//Указатель на следующий дисплей
} LCD_Handler;

extern LCD_Handler *LCD;		//указатель на список дисплеев (первый дисплей в списке)

void LCD_SetCS(LCD_Handler *lcd);
void LCD_ResCS(LCD_Handler *lcd);
void LCD_SetDC(LCD_Handler *lcd);
void LCD_ResDC(LCD_Handler *lcd);

void Display_TC_Callback(DMA_TypeDef *dma_x, uint32_t stream);

//создает обработчик дисплея и добавляет его в список дисплеев
//возвращает указатель на созданный дисплей либо 0 при неудаче
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
							LCD_BackLight_data bkl_data);

//Удаляет обработчик дисплея
void LCD_Delete(LCD_Handler* lcd);
//Аппаратный сброс дисплея
void LCD_HardWareReset(LCD_Handler* lcd);
//Инициализация дисплея
void LCD_Init(LCD_Handler* lcd);
//Интерпретатор командных строк
void LCD_String_Interpretator(LCD_Handler* lcd, const uint8_t *str);
//Установка яркости дисплея
void LCD_SetBackLight(LCD_Handler* lcd, uint8_t bk_percent);
//Возвращает текущую яркость дисплея
uint8_t LCD_GetBackLight(LCD_Handler* lcd);
//Возвращает ширину дисплея, пиксели
uint16_t LCD_GetWidth(LCD_Handler* lcd);
//Возвращает высоту дисплея, пиксели
uint16_t LCD_GetHeight(LCD_Handler* lcd);
//Возвращает статус дисплея
LCD_State LCD_GetState(LCD_Handler* lcd);
//Переводит дисплей в режим сна
void LCD_SleepIn(LCD_Handler* lcd);
//Выводит дисплей из режима сна
void LCD_SleepOut(LCD_Handler* lcd);
//Устанавливает окно вывода на дисплее
void LCD_SetActiveWindow(LCD_Handler* lcd, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
//Задает ориентацию изображения на дисплее
void LCD_SetOrientation(LCD_Handler* lcd, LCD_PageOrientation orientation);
//Отправляет данные на дисплей без DMA
void LCD_WriteData(LCD_Handler *lcd, uint16_t *data, uint32_t len);
//Отправляет данные на дисплей с использованием DMA
void LCD_WriteDataDMA(LCD_Handler *lcd, uint16_t *data, uint32_t len);
//Заливает окно с заданными координатами заданным цветом
void LCD_FillWindow(LCD_Handler* lcd, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
//Заливает весь экран заданным цветом
void LCD_Fill(LCD_Handler* lcd, uint32_t color);
//Рисует точку в заданных координатах заданным цветом
void LCD_DrawPixel(LCD_Handler* lcd, int16_t x, int16_t y, uint32_t color);
//Рисует линию по заданным координатам заданным цветом
void LCD_DrawLine(LCD_Handler* lcd, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t color);
//Рисует прямоугольник по заданным координатам заданным цветом
void LCD_DrawRectangle(LCD_Handler* lcd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color);
//Рисует закрашенный прямоугольник по заданным координатам заданным цветом
void LCD_DrawFilledRectangle(LCD_Handler* lcd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint32_t color);
//Рисует треугольник
void LCD_DrawTriangle(LCD_Handler* lcd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint32_t color);
//Рисует закрашенный треугольник
void LCD_DrawFilledTriangle(LCD_Handler* lcd, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint32_t color);
//Рисует окружность с заданным центром и радиусом с заданным цветом
void LCD_DrawCircle(LCD_Handler* lcd, int16_t x0, int16_t y0, int16_t r, uint32_t color);
//Рисует закрашенную окружность
void LCD_DrawFilledCircle(LCD_Handler* lcd, int16_t x0, int16_t y0, int16_t r, uint32_t color);
//Пересылает на дисплей блок памяти (например, изображение или его часть)
void LCD_DrawImage(LCD_Handler* lcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *data, uint8_t dma_use_flag);
//Выводит символ в указанной позиции
void LCD_WriteChar(LCD_Handler* lcd, uint16_t x, uint16_t y, char ch, FontDef *font, uint32_t txcolor, uint32_t bgcolor, LCD_PrintSymbolMode modesym);
//Выводит строку символов с указанной позиции
void LCD_WriteString(LCD_Handler* lcd, uint16_t x, uint16_t y, const char *str, FontDef *font, uint32_t color, uint32_t bgcolor, LCD_PrintSymbolMode modesym);
//Преобразует цвет в формате R8G8B8 (24 бита) в 16 битовый R5G6B5:
//заданный покомпонентно
uint16_t LCD_Color(LCD_Handler *lcd, uint8_t r, uint8_t g, uint8_t b);
//заданный словом
uint16_t LCD_Color_24b_to_16b(LCD_Handler *lcd, uint32_t color);
//задержка в миллисекундах (основана на системном таймере Systick)
void LCD_Delay(uint32_t Delay);

#endif /* INC_DISPLAY_H_ */
