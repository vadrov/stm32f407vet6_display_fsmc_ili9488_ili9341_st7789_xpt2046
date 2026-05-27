/*
 *	Драйвер управления дисплеями
 *  Версия: 2.0 F4/FSMC
 *  Author: VadRov
 *  Copyright (C) 2019 - 2022, VadRov, all right reserved.
 *
 *  Драйвер контроллера XPT2046 (HR2046 и т.п.)
 *  Версия: 1.1 F4
 *  Copyright (C) 2019, VadRov, all right reserved.
 *  Author: VadRov
 *
 *  Графическая библиотека для работы с объектами-примитивами в двумерном пространстве microGL2D
 *  Author: VadRov
 *  Copyright (C) 2022, VadRov, all right reserved.
 *
 *  TJpgDec - Tiny JPEG Decompressor R0.03                      (C)ChaN, 2021
 *  Copyright (C) 2021, ChaN, all right reserved.
 *  Portions сopyright (C) 2022-2023, VadRov, all right reserved.
 *
 *  Допускается свободное распространение.
 *  При любом способе распространения указание автора ОБЯЗАТЕЛЬНО.
 *  В случае внесения изменений и распространения модификаций указание первоначального автора ОБЯЗАТЕЛЬНО.
 *  Распространяется по типу "как есть", то есть использование осуществляется на свой страх и риск.
 *  Автор не предоставляет никаких гарантий.
 *
 *  Демо-проект собран для отладочной платы STM32F4VE ver 2.1 (JL-32_F4VE v.2.0) на базе м/к stm32f407vet6
 *  Схемы отладочной платы и дисплейного модуля прилагаются к проекту
 *
 *  https://www.youtube.com/@VadRov
 *  https://dzen.ru/vadrov
 *  https://vk.com/vadrov
 *  https://t.me/vadrov_channel
 *
 */
#include "main.h"
#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "ili9488.h"
#include "microgl2d.h"
#include "textures.h"
#include "jpeg_chan.h"
#include "xpt2046.h"
#include "calibrate_touch.h"
#include "demo.h"
#include <math.h>

#define RENDER_BUFFER_LINES	8

uint32_t SystemCoreClock = 16000000;
volatile uint32_t millis = 0;
uint16_t *render_buf1, *render_buf2;
extern iPicture_jpg picture_jpg_1;
extern iPicture_jpg picture_jpg_0;
extern iPicture_jpg picture_jpg_test;

//Выводы портов
#define GPIO_PIN_0				(1UL << 0U)
#define GPIO_PIN_1				(1UL << 1U)
#define GPIO_PIN_2				(1UL << 2U)
#define GPIO_PIN_3				(1UL << 3U)
#define GPIO_PIN_4				(1UL << 4U)
#define GPIO_PIN_5				(1UL << 5U)
#define GPIO_PIN_6				(1UL << 6U)
#define GPIO_PIN_7				(1UL << 7U)
#define GPIO_PIN_8				(1UL << 8U)
#define GPIO_PIN_9				(1UL << 9U)
#define GPIO_PIN_10				(1UL << 10U)
#define GPIO_PIN_11				(1UL << 11U)
#define GPIO_PIN_12				(1UL << 12U)
#define GPIO_PIN_13				(1UL << 13U)
#define GPIO_PIN_14				(1UL << 14U)
#define GPIO_PIN_15				(1UL << 15U)

//Типы выходов порта
#define GPIO_OUTPUT_PUSHPULL	0 //Output push-pull
#define GPIO_OUTPUT_OPENDRAIN	1 //Output open-drain

//Типы подтяжки вывода порта
#define GPIO_PULL_NO			0 //No pull-up, pull-down
#define GPIO_PULL_UP			1 //Pull-up
#define GPIO_PULL_DOWN			2 //Pull-down

//Скорости вывода порта
#define GPIO_SPEED_LOW 			0 //Low speed
#define GPIO_SPEED_MEDIUM		1 //Medium speed
#define GPIO_SPEED_HIGH			2 //High speed
#define GPIO_SPEED_VERYHIGH		3 //Very high speed

//Режимы вывода порта
#define GPIO_MODE_INPUT			0 //Input
#define GPIO_MODE_OUTPUT		1 //General purpose output mode
#define GPIO_MODE_ALTERNATE		2 //Alternate function mode
#define GPIO_MODE_ANALOG		3 //Analog mode

//Альтернативные функции выводов портов семейства микроконтроллеров STM32F405xx/07xx и STM32F415xx/17xx
//См. Схему 26. Выбор альтернативной функции, RM0090
#define GPIO_ALTERNATE_AF0		0 //AF0 (system)
#define GPIO_ALTERNATE_AF1		1 //AF1 (TIM1/TIM2)
#define GPIO_ALTERNATE_AF2		2 //AF2 (TIM3..5)
#define GPIO_ALTERNATE_AF3		3 //AF3 (TIM8..11)
#define GPIO_ALTERNATE_AF4		4 //AF4 (I2C1..3)
#define GPIO_ALTERNATE_AF5		5 //AF5 (SPI1/SPI2)
#define GPIO_ALTERNATE_AF6		6 //AF6 (SPI3)
#define GPIO_ALTERNATE_AF7		7 //AF7 (USART1..3)
#define GPIO_ALTERNATE_AF8		8 //AF8 (USART4..6)
#define GPIO_ALTERNATE_AF9		9 //AF9 (CAN1/CAN2, TIM12..14)
#define GPIO_ALTERNATE_AF10		10 //AF10 (OTG_FS, OTG_HS)
#define GPIO_ALTERNATE_AF11		11 //AF11 (ETH)
#define GPIO_ALTERNATE_AF12		12 //AF12 (FSMC, SDIO, OTG_HS)
#define GPIO_ALTERNATE_AF13		13 //AF13 (DCMI)
#define GPIO_ALTERNATE_AF14		14 //AF14
#define GPIO_ALTERNATE_AF15		15 //AF15 (EVENTOUT)

static void GPIO_Init(GPIO_TypeDef *port, uint32_t pins, uint32_t mode, uint32_t alternate, uint32_t speed, uint32_t pull, uint32_t output_type)
{
	uint32_t tmp;
	for (uint32_t pinnum = 0; pinnum <= 15 && (pins >> pinnum); pinnum++) {
		uint32_t pin = pins & (1UL << pinnum);
		if (pin) {
			if (mode == GPIO_MODE_OUTPUT || mode == GPIO_MODE_ALTERNATE) {
				//Скорость вывода
				tmp = (port->OSPEEDR & (~(3UL << (pinnum * 2U)))) | (speed << (pinnum * 2U));
				port->OSPEEDR = tmp;
				//Тип вывода порта
				if (output_type == GPIO_OUTPUT_PUSHPULL)
					port->OTYPER &= ~pin; //двухтактный (push_pull)
				else
					port->OTYPER |= pin; //открытый сток (open_drain)
				//Выбор альтернативной функции
				if (mode == GPIO_MODE_ALTERNATE) {
					if (pinnum < 8) { //младший регистр AFR (AFRL)
						tmp = (port->AFR[0] & (~(15UL << (pinnum * 4U)))) | (alternate << (pinnum * 4U));
						port->AFR[0] = tmp;
					}
					else { //старший регистр AFR (AFRH)
						tmp = (port->AFR[1] & (~(15UL << ((pinnum - 8) * 4U)))) | (alternate << ((pinnum - 8) * 4U));
						port->AFR[1] = tmp;
					}
				}
			}

			//Подтяжка вывода (pull_no, pull_up, pull_down)
			tmp = (port->PUPDR & (~(3UL << (pinnum * 2U)))) | (pull << (pinnum * 2U));
			port->PUPDR = tmp;

			//Режим вывода порта
			tmp = (port->MODER & (~(3UL << (pinnum * 2U)))) | (mode << (pinnum * 2U));
			port->MODER = tmp;
		}
	}
}

static void IO_Init(void)
{
	//Включение тактирования портов: B, A, H, C
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOHEN | RCC_AHB1ENR_GPIOCEN;
	(void)RCC->AHB1ENR;
	//Настройка вывода PB12 ---> T_CS
	GPIO_Init( GPIOB,
			   GPIO_PIN_12,
			   GPIO_MODE_OUTPUT,
			   GPIO_ALTERNATE_AF0,
			   GPIO_SPEED_HIGH,
			   GPIO_PULL_UP,
			   GPIO_OUTPUT_PUSHPULL);
	GPIOB->BSRR = GPIO_PIN_12; //на выходе высокий уровень
	//Настройка вывода PC5 ---> T_PEN
	GPIO_Init( GPIOC,
			   GPIO_PIN_5,
			   GPIO_MODE_INPUT,
			   GPIO_ALTERNATE_AF0,
			   GPIO_SPEED_HIGH,
			   GPIO_PULL_UP,
			   GPIO_OUTPUT_PUSHPULL);

 	//настройка внешних прерываний на линии EXTI5 для 5 вывода порта С
	SYSCFG->EXTICR[1] &= ~SYSCFG_EXTICR2_EXTI5_Msk; //регистр SYSCFG_EXTICR2, EXTI5, порт С
	SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI5_PC;

	EXTI->EMR &= ~EXTI_EMR_EM5;  //откл. Event
	EXTI->IMR |= EXTI_IMR_IM5;   //вкл. IT
	//EXTI->RTSR |= EXTI_RTSR_TR5; //вкл. Rising Trigger
	EXTI->FTSR |= EXTI_FTSR_TR5; //вкл. Falling Trigger

	NVIC_SetPriority(EXTI9_5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
	NVIC_EnableIRQ(EXTI9_5_IRQn);
}

static void SPI2_Init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN; //Включение тактирования SPI2
	(void)RCC->APB1ENR;

	RCC->APB1ENR |= RCC_AHB1ENR_GPIOBEN;//Включение тактирования порта B
	(void)RCC->APB1ENR;

	//Настройка выводов PB13 ---> SPI2_SCK, PB14 ---> SPI2_MISO, PB15 ---> SPI2_MOSI
	GPIO_Init( GPIOB,
			   GPIO_PIN_13  | GPIO_PIN_14  | GPIO_PIN_15,
			   GPIO_MODE_ALTERNATE,
			   GPIO_ALTERNATE_AF5,
			   GPIO_SPEED_VERYHIGH,
			   GPIO_PULL_NO,
			   GPIO_OUTPUT_PUSHPULL);
	SPI2->CR1 &= ~SPI_CR1_SPE;

	SPI2->CR1 = (0UL << SPI_CR1_CPHA_Pos)     |		// Clock phase - 1 edge
				(1UL << SPI_CR1_CPOL_Pos)     | 	// Clock polarity - high
				(1UL << SPI_CR1_MSTR_Pos)     | 	// Master configuration
				(1UL << SPI_CR1_SSI_Pos)	  |		// Internal slave select
				(0UL << SPI_CR1_BR_Pos)       | 	// Baud rate = Fpclk / 2
				(0UL << SPI_CR1_LSBFIRST_Pos) |		// Frame format - MSB transmitted first
				(0UL << SPI_CR1_DFF_Pos) 	  | 	// Data frame format - 8 bit
				(1UL << SPI_CR1_SSM_Pos)	  |     // Software slave management
				(1UL << SPI_CR1_BIDIOE_Pos)   |		// Output enabled (transmit-only mode)
				(0UL << SPI_CR1_BIDIMODE_Pos);		// 2-line bidirectional data mode selected

	SPI2->CR2 &= ~SPI_CR2_FRF; // protocol MOTOROLA
	SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SMOD;
}

static void TIM3_CH4_PWM_Init(int inv_pol)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; //Включение тактирования таймера TIM3
	(void)RCC->APB1ENR;

	TIM3->PSC = 83; //Делитель для тактовой частоты счетчика таймера

	TIM3->CR1 &= ~(TIM_CR1_CMS | TIM_CR1_DIR | //Суммирующий счетчик с выравниванием по краю
			       TIM_CR1_CKD | 			   //
				   TIM_CR1_ARPE); 			   //

	TIM3->ARR = 99; //Период счетчика (значение регистра автоматической перезагрузки). Определяет период импульса.
					//соответствует частоте 10 кГц

	TIM3->EGR |= TIM_EGR_UG; //Включение обновления регистров и переинициализации счетчика.

	TIM3->CCMR2 |= TIM_CCMR2_OC4PE; //Включение предварительной загрузки для канала сравнения

	TIM3->CCER &= ~TIM_CCER_CC4E; //Выключение канала 4 (вдруг включен)

	//Канал СС4 настраивается как выход с режимом PWM mode 1 (при восходящем счете канал 1 активен,
	//пока TIM3_CNT<TIM3_CCR4, иначе неактивен. При обратном счете канал 1 неактивен (OC4REF=’0),
	//пока TIM3_CNT>TIM3_CCR4, иначе активен (OC4REF=1)).
	//
	uint32_t temp = (TIM3->CCMR2 & (~(TIM_CCMR2_CC4S | TIM_CCMR2_OC4M | TIM_CCMR2_OC4FE))) | (6UL << TIM_CCMR2_OC4M_Pos);
	TIM3->CCMR2 = temp;
	//Полярность выходного канала захвата/сравнения - активным выбран высокий уровень.
	TIM3->CCER &= ~TIM_CCER_CC4P;
	if (inv_pol) {
		//Полярность выходного канала захвата/сравнения - активным выбран низкий уровень.
		TIM3->CCER |= TIM_CCER_CC4P;
	}
	//Значение регистра 4 канала захвата/сравнения (длительность импульса -> pulse).
	//Определяет величину скважности, рассчитываемую как отношение периода импульсов
	//к их длительности.
	TIM3->CCR4 = 49;

	//Триггерный выход TRGO, используемый для синхронизации таймера.
	//Сброс – бит UG регистра TIM3_EGR используется в качестве TRGO.
	TIM3->CR2 &= ~TIM_CR2_MMS;

	TIM3->SMCR &= ~TIM_SMCR_MSM; //отключение режима мастер/ведомый

	//Настройка вывода PB1, как выход 4 канала 3 таймера  (Timer 3 channel 4)
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;//включение тактирования порта B
	(void)RCC->AHB1ENR;
	GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_ALTERNATE, GPIO_ALTERNATE_AF2, GPIO_SPEED_LOW, GPIO_PULL_NO, GPIO_OUTPUT_PUSHPULL);
}

static void FSMC_Init(void)
{
	//Включаем тактирование FSMC
	RCC->AHB3ENR |= RCC_AHB3ENR_FSMCEN;
	(void)RCC->AHB3ENR;
	//Включаем тактирование портов D и E
	RCC->AHB1ENR |= (RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN);
	(void)RCC->AHB1ENR;

	//Настройка выводов PD0, PD1, PD4, PD5, PD7 - PD10, PD13 - PD15
	GPIO_Init( GPIOD,
			   GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_4  |
			   GPIO_PIN_5  | GPIO_PIN_7  | GPIO_PIN_8  |
			   GPIO_PIN_9  | GPIO_PIN_10 | GPIO_PIN_13 |
			   GPIO_PIN_14 | GPIO_PIN_15,
			   GPIO_MODE_ALTERNATE,
			   GPIO_ALTERNATE_AF12,
			   GPIO_SPEED_VERYHIGH,
			   GPIO_PULL_NO,
			   GPIO_OUTPUT_PUSHPULL);

	//Настройка выводов с PE7 по PE15
	GPIO_Init( GPIOE,
			   GPIO_PIN_7  | GPIO_PIN_8  | GPIO_PIN_9  |
			   GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
			   GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
			   GPIO_MODE_ALTERNATE,
			   GPIO_ALTERNATE_AF12,
			   GPIO_SPEED_VERYHIGH,
			   GPIO_PULL_NO,
			   GPIO_OUTPUT_PUSHPULL);

	//Отключаем банк Bank1 для настройки
	FSMC_Bank1->BTCR[0] &= ~FSMC_BCR1_MBKEN;

	//Конфигурирование банка для доступа к устройству памяти
	FSMC_Bank1->BTCR[0] = 0UL << FSMC_BCR1_MUXEN_Pos	 |  //Мультиплексирование адреса/данных (Отключено).
	  	  	  	  	  	  0UL << FSMC_BCR1_MTYP_Pos		 |	//Тип устройства памяти: 0 - SRAM, 1 - PSRAM (CRAM), 2 - NOR Flash/OneNAND Flash (SRAM).
	  					  1UL << FSMC_BCR1_MWID_Pos		 |	//Ширина шины данных: 0 - 8 бит, 1 - 16 бит (16 бит).
						  0UL << FSMC_BCR1_BURSTEN_Pos	 |	//Включает или отключает пакетный режим доступа к флэш-памяти.
						  	  	  	  	  	  	  	  	  	//Действителен только для синхронной пакетной флэш-памяти (Отключено).
						  0UL << FSMC_BCR1_WRAPMOD_Pos	 |	//Включает или отключает режим пакетного доступа (Отключено).
						  	  	  	  	  	  	  	  	  	//Примечание. Не имеет эффекта, поскольку ЦП и DMA не могут генерировать пакетные передачи.
						  0UL << FSMC_BCR1_WAITEN_Pos	 |	//Включает или отключает состояние ожидания посредством формирования сигнала ожидания.
						  	  	  	  	  	  	  	  	  	//Действительно для доступа к флэш-памяти в пакетном режиме (Отключено).
						  0UL << FSMC_BCR1_WAITCFG_Pos	 |	//Указывает, когда подается сигнал ожидания: 0 - за один такт до состояния ожидания,
						  	  	  	  	  	  	  	  	  	//1 - во время состояния ожидания. Действует только при доступе к памяти в пакетном режиме.
						  0UL << FSMC_BCR1_ASYNCWAIT_Pos |	//Включает или отключает сигнал ожидания при асинхронной передаче.
						  	  	  	  	  	  	  	  	  	//Действителен только для асинхронной флэш-памяти (Отключено).
						  1UL << FSMC_BCR1_WREN_Pos		 |	//Разрешает или запрещает операции записи на выбранном устройстве памяти (Разрешено).
						  0UL << FSMC_BCR1_CBURSTRW_Pos	 |	//Разрешает или запрещает операции пакетной записи (Запрещено).
						  0UL << FSMC_BCR1_WAITPOL_Pos	 |	//Определяет полярность для сигнала ожидания: 0 - активный при низком уровне, 1 - активный при высоком уровне.
						  	  	  	  	  	  	  	  	  	//Действительно только при доступе к флэш-памяти в пакетном режиме.
						  0UL << FSMC_BCR1_FACCEN_Pos	 |	//Доступ к флеш. Отключено для SRAM, для NOR включено (Отключен).
						  1UL << FSMC_BCR1_EXTMOD_Pos;		//Раздельные тайминги для чтения/записи -> расширенный режим (Включен).
	//Тайминги чтения
	//См. Таблицу 220. Программируемые параметры доступа к NOR/PSRAM, RM0090
	FSMC_Bank1->BTCR[1] = 10UL << FSMC_BTR1_ADDSET_Pos	 |	//Определяет циклы HCLK для настройки продолжительности времени установки адреса: 0...15 HCLK.
															//Этот параметр не используется с синхронной флэш-памятью NOR.
						  0UL << FSMC_BTR1_ADDHLD_Pos	 |	//Определяет циклы HCLK для настройки продолжительности времени удержания адреса: 1...15 HCLK.
						  	  	  	  	  	  	  	  	  	//Этот параметр не используется с синхронной флэш-памятью NOR.
						  10UL << FSMC_BTR1_DATAST_Pos	 |	//Определяет циклы HCLK для настройки продолжительности времени установки данных: 1...256 HCLK.
						  	  	  	  	  	  	  	  	  	//Этот параметр используется для SRAM, ROM и асинхронной мультиплексированной флэш-памяти NOR.
						  0UL << FSMC_BTR1_BUSTURN_Pos	 |	//Определяет циклы HCLK для настройки продолжительности переключения шины: 0...15 HCLK.
						  	  	  	  	  	  	  	  	    //Этот параметр используется только для мультиплексированной флэш-памяти NOR.
						  0UL << FSMC_BTR1_CLKDIV_Pos	 |	//Определяет период выходного сигнала синхронизации CLK, выраженный в количестве циклов HCLK: 2...16 HCLK.
						  	  	  	  	  	  	  	  	    //Этот параметр не используется для асинхронного доступа NOR Flash, SRAM или ROM.
						  0UL << FSMC_BTR1_DATLAT_Pos	 |	//Определяет количество тактов памяти, которые необходимо выполнить перед получением первых данных.
						  	  	  	  	  	  	  	  	  	//Значение параметра зависит от типа памяти:
						  	  	  	  	  	  	  	  	  	//1. В случае CRAM он должен быть установлен на 0.
						  	  	  	  	  	  	  	  	  	//2. Не имеет значения при асинхронный доступе NOR, SRAM или ROM.
						  	  	  	  	  	  	  	  	  	//3. От 2 до 17 CLK для флэш-памяти NOR с включенным синхронным пакетным режимом.
						  0 << FSMC_BTR1_ACCMOD_Pos;		//Определяет режим асинхронного доступа: 0 - A, 1 - B, 2 - C, 3 - D. Учитывается только
															//в том случае, если включен активный режим (установлен бит EXTMOD в регистре BCRx).

	//Тайминги записи
	FSMC_Bank1E->BWTR[0] = (0UL << FSMC_BTR4_ADDSET_Pos) |
						   (1UL << FSMC_BTR4_DATAST_Pos) |
						   (0UL << FSMC_BTR4_ACCMOD_Pos);

	//Включаем банк
	FSMC_Bank1->BTCR[0] |= FSMC_BCR1_MBKEN;

	//--------------------------------- Настройка DMA и прерываний ----------------------------
	//TODO В соответствии с документацией, транзакции DMA в режиме память-память доступны только контроллеру DMA2.
	//TODO При использовании режима «память-память» кольцевой и прямой режимы не допускаются.
	//Включаем тактирование контроллера DMA2
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
	(void)RCC->AHB1ENR;
	//Настраиваем приоритет прерываний для событий потока DMA
	NVIC_SetPriority(DMA2_Stream0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 0));
	//Включаем прерывания от потока
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);

	DMA2_Stream0->CR =  (1UL << DMA_SxCR_CHSEL_Pos)  |	//1 канал (свободный)
					    (2UL << DMA_SxCR_DIR_Pos)	 |	//направление передачи память-память (memory-to-memory)
					    (1UL << DMA_SxCR_PL_Pos)	 |	//приоритет DMA потока - средний (medium)
					    (0UL << DMA_SxCR_CIRC_Pos)	 |	//кольцевой режим отключен (режим нормальный, normal)
					    (1UL << DMA_SxCR_PINC_Pos)	 |	//адрес источника-памяти увеличивается после передачи очередного элемента
					    (0UL << DMA_SxCR_MINC_Pos)	 |	//адрес приемника-памяти фиксирован и не изменяется после передачи очередного элемента
					    (1UL << DMA_SxCR_PSIZE_Pos)	 |	//размер (ширина) данных источника 16 бит - полуслово
					    (1UL << DMA_SxCR_MSIZE_Pos)	 |	//размер (ширина) данных приемника 16 бит - полуслово
					    (1UL << DMA_SxCR_PBURST_Pos) |	//Выбираем пакетный режим. inc4 *
					    (1UL << DMA_SxCR_MBURST_Pos) |   //*
						(1UL << DMA_SxCR_TCIE_Pos);		//Разрешаем генерацию прерываний (выставление флага) по окончанию передачи данных.
	DMA2_Stream0->FCR = (1UL << DMA_SxFCR_DMDIS_Pos) |  //передача с использованием буфера FIFO *
						(1UL << DMA_SxFCR_FTH_Pos) ;    //порог FIFO 1/2 *
	//* - Cм. Таблицу 49. Конфигурации порогов FIFO (RM0090)
}

void SystemInit(void)
{
    SCB->CPACR |= (3UL << 20) | (3UL << 22);  //включение FPU
}

static void SetSystemClock(void)
{
	//Установка задержки flash
	uint32_t temp = (FLASH->ACR & (~FLASH_ACR_LATENCY)) | FLASH_ACR_LATENCY_5WS;
	FLASH->ACR = temp;
	while((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_5WS) { ; }

	PWR->CR |= PWR_CR_VOS; //режим питания: Scale 1 mode (на выключенном ФАПЧ!)
	//Включение внешнего осциллятора HSE
	RCC->CR |= RCC_CR_HSEON;

	//Ожидание готовности HSE
	while(!(RCC->CR & RCC_CR_HSERDY)) { ; }
	//Включение системы защиты тактирования
	RCC->CR |= RCC_CR_CSSON;

	//Настройка ФАПЧ из расчета, что внешний осциллятор HSE на 8 MHz
	temp = RCC->PLLCFGR;
	temp &= ~(RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP);
	temp |= RCC_PLLCFGR_PLLSRC_HSE          |
			(4UL << RCC_PLLCFGR_PLLM_Pos)   |
			(168UL << RCC_PLLCFGR_PLLN_Pos) |
			(0UL << RCC_PLLCFGR_PLLP_Pos);
	RCC->PLLCFGR = temp;

	//Включение модуля ФАПЧ (PLL)
	RCC->CR |= RCC_CR_PLLON;

	//Ожидание готовности (стабилизации частоты) модуля ФАПЧ
	while(!(RCC->CR & RCC_CR_PLLRDY)) { ; }
	//Ожидание готовности VOS (стабилизация напряжения)
	while (!(PWR->CSR & PWR_CSR_VOSRDY)) { ; }

	//Установка делителей частоты для шин AHB, APB1, APB2,
	//Выбор ФАПЧ в роли источника синхронизации системы
	temp = RCC->CFGR;
	temp &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | RCC_CFGR_SW);
	temp |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2 | RCC_CFGR_SW_PLL;
	RCC->CFGR = temp;

	//Ожидание смены источника синхронизации системы на ФАПЧ
	while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { ; }

	SystemCoreClock = 168000000;

	//Настройка SysTick
	SysTick->LOAD  = (uint32_t)((SystemCoreClock / 1000) - 1UL);
	NVIC_SetPriority (SysTick_IRQn, 15);
	NVIC_EnableIRQ(SysTick_IRQn);
	SysTick->VAL = 0UL;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

static void demo_jpg (LCD_Handler *lcd, iPicture_jpg *file)
{
	uint32_t tick;
	const char str1[] = "decoding time = ";
	const char str2[] = " cycles";
	char str[50];
	uint32_t repeat = 10;
	while (repeat--) {
		DWT->CYCCNT = 0;
		LCD_Load_JPG_chan(lcd, 0, 0, lcd->Width, lcd->Height, file, PICTURE_IN_MEMORY);
		tick = DWT->CYCCNT;
		strcpy(str, str1);
		utoa(tick, &str[strlen(str)], 10);
		strcat(str, str2);
		LCD_WriteString(lcd, 0, 0, str, &Font_8x13, COLOR_BLACK, COLOR_CYAN, LCD_SYMBOL_PRINT_FAST);
		LCD_Delay(1000);
	}
}

static void Render2D (LCD_Handler *lcd, MGL_OBJ *obj, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int x_c, int y_c)
{
	int lines = y1 - y0 + 1;
	uint32_t w = x1 - x0 + 1;
	uint16_t *render_ptr = render_buf1;
	uint8_t use_dma = (lcd->fsmc_data.dma) ? 1 : 0;
	LCD_SetActiveWindow(lcd, x0, y0, x1, y1);
	uint32_t r_lines;
	while (lines) {
		r_lines = lines < RENDER_BUFFER_LINES ? lines : RENDER_BUFFER_LINES;
		MGL_RenderObjects(obj, x_c, y_c, x_c + w - 1, y_c + r_lines - 1, render_ptr);
		if (use_dma) {
			LCD_WriteDataDMA(lcd, render_ptr, r_lines * w);
			render_ptr = (render_ptr == render_buf1) ? render_buf2 : render_buf1;
		}
		else {
			LCD_WriteData(lcd, render_ptr, r_lines * w);
		}
		lines -= r_lines;
		y_c += r_lines;
	}
}

void demo_gl(LCD_Handler *lcd)
{
	MGL_GRADIENT *grad1 = MGL_GradientCreate(MGL_GRADIENT_LINEAR);
	MGL_GradientAddColor(grad1, 0,   COLOR_WHITE,    1);
	MGL_GradientAddColor(grad1, 50,  COLOR_DARKGREY, 1);
	MGL_GradientAddColor(grad1, 100, COLOR_WHITE,    1);
	MGL_GradientSetDeg(grad1, 0);

	MGL_GRADIENT *grad2 = MGL_GradientCreate(MGL_GRADIENT_LINEAR);
	MGL_GradientAddColor(grad2, 0, COLOR_BLUE, 1);
	MGL_GradientAddColor(grad2, 100, COLOR_WHITE, 1);
	MGL_GradientSetDeg(grad2, 0);

	MGL_GRADIENT *gradt = MGL_GradientCreate(MGL_GRADIENT_LINEAR);
	MGL_GradientAddColor(gradt, 0, COLOR_BLUE, 1);
	MGL_GradientAddColor(gradt, 100, COLOR_CYAN, 1);
	MGL_GradientSetDeg(gradt, 0);

	// фон
	MGL_GRADIENT *grad_fon = MGL_GradientCreate(MGL_GRADIENT_LINEAR);
	MGL_GradientSetDeg(grad_fon, 0);
	MGL_GradientAddColor(grad_fon, 0,   0xFF0000, 1);
	MGL_GradientAddColor(grad_fon, 25,  0x00FFFF, 1);
	MGL_GradientAddColor(grad_fon, 40,  0xFFFFFF, 1);
	MGL_GradientAddColor(grad_fon, 50,  0x00FF00, 1);
	MGL_GradientAddColor(grad_fon, 60,  COLOR_ORANGE, 1);
	MGL_GradientAddColor(grad_fon, 75,  0xFFFF00, 1);
	MGL_GradientAddColor(grad_fon, 100, 0x0000FF, 1);
	MGL_OBJ *rect = MGL_ObjectAdd(0, MGL_OBJ_TYPE_FILLRECTANGLE);
	MGL_SetRectangle(rect, 0, 0, lcd->Width-1, lcd->Height-1, COLOR_WHITE);
	//MGL_ObjectSetGradient(rect, grad_fon);
	MGL_TEXTURE texture_youtube = {(MGL_IMAGE *)&image_youtube, 0, 0/*MGL_TEXTURE_REPEAT_X | MGL_TEXTURE_REPEAT_Y*/};
	MGL_ObjectSetTexture(rect, &texture_youtube);

	// информационное окно
	MGL_OBJ *obj1 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_FILLRECTANGLE);
	MGL_SetRectangle(obj1, 0, 0, 200, 14, COLOR_BLUE);
	MGL_ObjectSetGradient(obj1, grad2);
	MGL_OBJ *obj2 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_FILLRECTANGLE);
	MGL_SetRectangle(obj2, 0, 14, 200, 100, COLOR_WHITE);
	MGL_ObjectSetGradient(obj2, grad1);
	MGL_ObjectSetTransparency(obj2, 50);
	MGL_OBJ *obj3 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_FILLRECTANGLE);
	MGL_SetRectangle(obj3, 200-12, 2, 200-4, 12, COLOR_RED);
	MGL_OBJ *obj4 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_TEXT);
	MGL_SetText(obj4, 200-12, 1, "X", &Font_8x13, 1, COLOR_WHITE);
	MGL_OBJ *obj5 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_TEXT);
	MGL_SetText(obj5, 4, 1, "Message", &Font_8x13, 0, COLOR_WHITE);
	MGL_OBJ *obj6 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_TEXT);
	MGL_SetText(obj6, 8, 20, "Hello, YouTube!", &Font_12x20, 1, COLOR_BLACK);
	MGL_ObjectSetGradient(obj6, gradt);
	MGL_OBJ *rect1 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_RECTANGLE);
	MGL_SetRectangle(rect1, 0, 0, 200, 100, COLOR_BLACK);
	MGL_OBJ *img_obj = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_FILLCIRCLE);
	MGL_SetCircle(img_obj, 30, 70, 20, COLOR_WHITE);
	MGL_TEXTURE texture = {(MGL_IMAGE *)&image_avatar, 0, 0};
	MGL_ObjectSetTexture(img_obj, &texture);

	// горизонтальный ползунок
	MGL_GRADIENT *grad3 = MGL_GradientCreate(MGL_GRADIENT_RADIAL);
	MGL_GradientAddColor(grad3, 0, COLOR_WHITE, 1);
	MGL_GradientAddColor(grad3, 100, COLOR_BLUE, 1);
	MGL_GRADIENT *grad4 = MGL_GradientCreate(MGL_GRADIENT_LINEAR);
	MGL_GradientAddColor(grad4, 0, COLOR_CYAN, 1);
	MGL_GradientAddColor(grad4, 100, COLOR_RED, 1);
	MGL_GradientSetDeg(grad4, 90);
	MGL_GRADIENT *grad5 = MGL_GradientCreate(MGL_GRADIENT_LINEAR);
	MGL_GradientAddColor(grad5, 0, COLOR_CYAN, 1);
	MGL_GradientAddColor(grad5, 100, COLOR_RED, 1);
	MGL_OBJ *slider = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_SLIDER);
	MGL_SetSlider(slider, MGL_SLIDER_HORIZONTAL, 0, lcd->Height/2, lcd->Width - 20, lcd->Height/2 + 19, COLOR_LIGHTGREY, 0, 100, 50, "");
	MGL_SetRectangle(((MGL_OBJ_SLIDER*)slider->object)->obj_rectangle1, 0, 0, 0, 0, COLOR_CYAN);
	MGL_ObjectSetGradient(((MGL_OBJ_SLIDER*)slider->object)->obj_rectangle1, grad4);
	MGL_SetRectangle(((MGL_OBJ_SLIDER*)slider->object)->obj_rectangle2, 0, 0, 0, 0, COLOR_DARKGREY);
	MGL_ObjectSetGradient(((MGL_OBJ_SLIDER*)slider->object)->obj_circle, grad3);
	// вертикальный ползунок
	MGL_OBJ *slider1 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_SLIDER);
	MGL_SetSlider(slider1, MGL_SLIDER_VERTICAL, lcd->Width - 20, 0, lcd->Width - 1, lcd->Height - 1, COLOR_LIGHTGREY, 0, 100, 100, "");
	MGL_SetRectangle(((MGL_OBJ_SLIDER*)slider1->object)->obj_rectangle1, 0, 0, 0, 0, COLOR_CYAN);
	MGL_ObjectSetGradient(((MGL_OBJ_SLIDER*)slider1->object)->obj_rectangle1, grad5);
	MGL_SetRectangle(((MGL_OBJ_SLIDER*)slider1->object)->obj_rectangle2, 0, 0, 0, 0, COLOR_DARKGREY);
	MGL_ObjectSetGradient(((MGL_OBJ_SLIDER*)slider1->object)->obj_circle, grad3);

/*
	// стрелочный индикатор
	MGL_OBJ *strelka = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_ARROWINDICATOR);
	MGL_SetArrowIndicator(strelka, lcd->Width - 50, lcd->Height - 50, 50, COLOR_WHITE, 0, 100, 50, "ms");
	MGL_ObjectSetTransparency(((MGL_OBJ_ARROWINDICATOR*)strelka->object)->obj_circle, 120);
	MGL_SetText(((MGL_OBJ_ARROWINDICATOR*)strelka->object)->obj_text, 0, 0, "", &Font_8x13, 1, COLOR_RED);
	MGL_SetTriangle(((MGL_OBJ_ARROWINDICATOR*)strelka->object)->obj_triangle, 0, 0, 0, 0, 0, 0, COLOR_BLUE);
	MGL_TEXTURE texture_arwind = {(MGL_IMAGE *)&image_shkala, 0};
	MGL_ObjectSetTexture(((MGL_OBJ_ARROWINDICATOR*)strelka->object)->obj_circle, &texture_arwind);
	//MGL_GRADIENT grad_uk = {COLOR_RED, COLOR_BLUE, MGL_GRADIENT_SKEW, 0};
	//MGL_SetGradient(((MGL_OBJ_ARROWINDICATOR*)strelka->object)->obj_triangle, &grad_uk);

	// круговой индикатор
	MGL_OBJ *krug = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_CIRCLEINDICATOR);
	MGL_SetCircleIndicator(krug, 90, 200, 30, 25, COLOR_WHITE, 0, 100, 50, "ms");
	MGL_GRADIENT *gradient_crlind = MGL_GradientCreate(MGL_GRADIENT_LINEAR);
	MGL_GradientAddColor(gradient_crlind, 0,   COLOR_GREEN, 1);
	MGL_GradientAddColor(gradient_crlind, 100, COLOR_RED, 1);
	MGL_ObjectSetGradient(krug, gradient_crlind);
	MGL_SetText(((MGL_OBJ_CIRCLEINDICATOR*)krug->object)->obj_text, 0, 0, "", &Font_8x13, 1, COLOR_WHITE);
*/
	// мельница
	MGL_TEXTURE melnica_tex = {(MGL_IMAGE *)&image_melnica, 0, 0};
	MGL_OBJ *melnica = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_FILLRECTANGLE);
	MGL_SetRectangle(melnica, 0, 0, 60, 90, COLOR_RED);
	MGL_ObjectSetTransparency(melnica, 0);
	MGL_ObjectSetTexture(melnica, &melnica_tex);

	// лошадь
	MGL_TEXTURE loshad_tex = {(MGL_IMAGE *)&image_loshad, 0, MGL_TEXTURE_FLIP_X};
	MGL_OBJ *loshad = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_FILLCIRCLE);
	MGL_SetCircle(loshad, 100, 60, 30, COLOR_RED);
	MGL_ObjectSetTexture(loshad, &loshad_tex);

	// надпись - копирайт
	MGL_GRADIENT *grad_cprt = MGL_GradientCreate(MGL_GRADIENT_LINEAR);
	MGL_GradientAddColor(grad_cprt, 0, COLOR_WHITE, 1);
	MGL_GradientAddColor(grad_cprt, 100, COLOR_BLUE, 1);
	MGL_GradientSetDeg(grad_cprt, 45);
	MGL_OBJ *text = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_TEXT);
	MGL_SetText(text, (lcd->Width - 14*12)/2 - 1, (lcd->Height - 20)/2 - 1, "(c)2022 VadRov", &Font_12x20, 1, COLOR_RED);
	MGL_ObjectSetGradient(text, grad_cprt);
	MGL_OBJ *text1 = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_TEXT);
	MGL_SetText(text1, (lcd->Width - 8*12)/2 - 1, (lcd->Height - 20)/2 + 20 - 1, "mGL Demo", &Font_12x20, 1, COLOR_WHITE);

	// надпись - счетчик кадров
	char fps_s[10] = "FPS = ";
	MGL_OBJ *fps = MGL_ObjectAdd(rect, MGL_OBJ_TYPE_TEXT);
	MGL_SetText(fps, 0, lcd->Height - 21, fps_s, &Font_12x20, 0, COLOR_WHITE);

	int z = 1, z1 = 1, z2 = 1, z3 = 1, z4 = -2, z5 = -1, z6 = -1, step_z6 = 0, zz1 = 1, zz2 = 1;
	uint32_t frame = 0, counter = 0;
	int x0 = 0, y0 = 0, x1 = 2, y1 = 2, x_c = 0, y_c = 0;
	int f1 = 0;
	DWT->CYCCNT = 0;
	while (1)  {
/*
		if (!f1) {
			//LCD_DrawRectangle(lcd, x0, y0, x1, y1, COLOR_WHITE);
			Render2D(lcd, rect, x0+1, y0+1, x1-1, y1-1, x_c, y_c);
			//LCD_DrawRectangle(lcd, x0, y0, x1, y1, 0x319bb1);
			x1++;
			y1++;
			if (x1 == lcd->Width - 1 && y1 == lcd->Height - 1) f1 = 1;
		}
		else {
			Render2D(lcd, rect, x0+1, y0+1, x1-1, y1-1, x_c, y_c);
		}
*/
		Render2D(lcd, rect, 0, 0, lcd->Width - 1, lcd->Height - 1, 0, 0);

		MGL_ObjectListMove(obj1, z, z1);
		MGL_ObjectListMove(slider, -z, -z1);

		if (((MGL_OBJ_RECTANGLE*)obj1->object)->x1 <= 0)   z = -z;
		if (((MGL_OBJ_RECTANGLE*)obj1->object)->x2 >= lcd->Width - 1) z = -z;

		if (((MGL_OBJ_RECTANGLE*)obj1->object)->y1 <= 0)   z1 = -z1;
		if (((MGL_OBJ_RECTANGLE*)obj1->object)->y2 >= lcd->Height - 1 - 86) z1 = -z1;

		MGL_ObjectMove(text, 0, -1);
		MGL_ObjectMove(text1, 0, -1);
		if (((MGL_OBJ_TEXT*)text1->object)->y < -30) {
			((MGL_OBJ_TEXT*)text->object)->y = lcd->Height + 30;
			((MGL_OBJ_TEXT*)text1->object)->y = lcd->Height + 50;
		}

		((MGL_OBJ_CIRCLE*)img_obj->object)->r += z2;
		if (((MGL_OBJ_CIRCLE*)img_obj->object)->r < 15 ||
			((MGL_OBJ_CIRCLE*)img_obj->object)->r > 25)
			z2 = -z2;

		img_obj->texture->alpha += 10;
		if (img_obj->texture->alpha > 360) img_obj->texture->alpha %= 360;

		((MGL_OBJ_SLIDER*)slider->object)->value += z3;
		if (((MGL_OBJ_SLIDER*)slider->object)->value <= 0 ||
			((MGL_OBJ_SLIDER*)slider->object)->value >= 100)
			z3 = -z3;
/*
		((MGL_OBJ_ARROWINDICATOR*)strelka->object)->value += z6;
		((MGL_OBJ_CIRCLEINDICATOR*)krug->object)->value += z6;
		if (((MGL_OBJ_ARROWINDICATOR*)strelka->object)->value <= 0 ||
		    ((MGL_OBJ_ARROWINDICATOR*)strelka->object)->value >= 100)
			z6 = -z6;
*/
		((MGL_OBJ_SLIDER*)slider1->object)->value += z4;
		if (((MGL_OBJ_SLIDER*)slider1->object)->value <= 0 ||
			((MGL_OBJ_SLIDER*)slider1->object)->value >= 100)
			z4 = -z4;

		loshad_tex.alpha += z5;
		if (!(counter % 5))  {
			MGL_ObjectMove(loshad, -z5, 0);
		}
		if (loshad_tex.alpha == 20 ||
			loshad_tex.alpha == -20) z5 = -z5;

		MGL_ObjectMove(melnica, zz1, zz2);
		if (((MGL_OBJ_RECTANGLE*)melnica->object)->x1 < 0 ||
			((MGL_OBJ_RECTANGLE*)melnica->object)->x1 > lcd->Width - 60)
			zz1 = -zz1;
		if (((MGL_OBJ_RECTANGLE*)melnica->object)->y1 < 0 ||
			((MGL_OBJ_RECTANGLE*)melnica->object)->y1 > lcd->Height - 90)
			zz2 = -zz2;

		//grad_fon->deg += 2;
		texture_youtube.alpha += 2;

		counter++;

		/* подсчет количества кадров в секунду (fps) */
		frame++;
		if (DWT->CYCCNT >= SystemCoreClock) {
			utoa(frame, &fps_s[6], 10);
			frame = 0;
			DWT->CYCCNT = 0;
		}
		MGL_GRADIENT_POINT *points_list = grad_fon->points_list;
		int f = 0;
		while (points_list) {
			if (f) {
				if (points_list->next) {
					points_list->offset += z6;
				}
			}
			else {
				f = 1;
			}
			points_list = (MGL_GRADIENT_POINT*)points_list->next;
		}
		step_z6++;
		if (step_z6 > 20) {
			step_z6 = 0;
			z6 = -z6;
		}
	}
}

static void DWT_init (void)
{
	if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk))	{
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	#ifdef __CORE_CM7_H_GENERIC
	        DWT->LAR = 0xC5ACCE55;  //разблокирование доступа к DWT регистру
	#endif
	   DWT->CYCCNT = 0;
	   DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	}
}


#define fabs_(x)	(x < 0 ? -x : x)

//таблица синусов от 0 до 90 градусов (значения умножены на 32768)
const uint16_t sin1_tbl_int[91] = {	    0,   572,  1144,  1715,  2286,  2856,  3425,  3993,  4560,  5126,
									 5690,  6252,  6813,  7371,  7927,  8481,  9032,  9580, 10126, 10668,
									11207, 11743, 12275, 12803, 13328, 13848, 14365, 14876, 15384, 15886,
									16384, 16877, 17364, 17847, 18324, 18795, 19261, 19720, 20174, 20622,
									21063, 21498, 21926, 22348, 22763, 23170, 23571, 23965, 24351, 24730,
									25102, 25466, 25822, 26170, 26510, 26842, 27166, 27482, 27789, 28088,
									28378, 28660, 28932, 29197, 29452, 29698, 29935, 30163, 30382, 30592,
									30792, 30983, 31164, 31336, 31499, 31651, 31795, 31928, 32052, 32166,
									32270, 32365, 32449, 32524, 32588, 32643, 32688, 32723, 32748, 32763,
									32768 };

//возвращает синус угла умноженный на 32768.
//angle - угол в градусах
static int sin_int(int angle)
{
	if (fabs_(angle) > 360) angle %= 360;
	if (angle < 0) angle += 360;
	if (angle >=   0 && angle <=  90) return (int)sin1_tbl_int[angle];
	if (angle >=  91 && angle <= 180) return (int)sin1_tbl_int[180 - angle];
	if (angle >= 181 && angle <= 270) return -(int)sin1_tbl_int[angle - 181];
	return -(int)sin1_tbl_int[360 - angle]; //271...360
}

static uint32_t demo_fill(LCD_Handler *lcd)
{
	uint32_t frames;
	char buff[10];
	uint8_t r = 0, g = 0, b = 0;
	uint32_t cycles = 3;
	while (cycles--) {
		LCD_SetActiveWindow(lcd, 0, 0, lcd->Width - 1, lcd->Height - 1);
		frames = 0;
		DWT->CYCCNT = 0;
		while (DWT->CYCCNT < SystemCoreClock) {
			LCD_Fill(lcd, (r << 16) | (g << 8) | b);
			r++;
			g += 2;
			b += 4;
			frames++;
		}
		utoa(frames, buff, 10);
		LCD_WriteString(lcd, 0, 0, buff, &Font_12x20, COLOR_YELLOW, COLOR_BLUE, LCD_SYMBOL_PRINT_FAST);
		LCD_Delay(1000);
	}
	return frames;
}

int main(void)
{
	FLASH->ACR |= (1UL << FLASH_ACR_PRFTEN_Pos) | //вкл. системы предварительной выборки инструкций
				  (1UL << FLASH_ACR_ICEN_Pos)   | //вкл. кеширования инструкций
				  (1UL << FLASH_ACR_DCEN_Pos); 	  //вкл. кеширования данных

	//Включение тактирования контроллера конфигурации системы
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	(void)RCC->APB2ENR;

	//Включение тактирования интерфейса питания
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	(void)RCC->APB1ENR;

	//Инициализация системных прерываний по схеме:
	//4 бита для приоритета вытеснения и 0 бит для субприоритета
	NVIC_SetPriorityGrouping(3);

	SetSystemClock();

	IO_Init();
  	SPI2_Init();
  	TIM3_CH4_PWM_Init(1); //сигнал ШИМ для подсветки дисплея:
  						  //1 - с активным низким уровнем канала захвата/сравнения
  						  //0 - с активным высоким уровнем канала захвата/сравнения
	FSMC_Init();
    DWT_init();

	//1 подбанк: 0x60000000
	//2 подбанк: 0x64000000
	//3 подбанк: 0x68000000
	//4 подбанк: 0x6c000000
  	LCD_FSMC_Connected_data fsmc_data = { .cmd_addr  = 0x60000000, //адрес для записи команд
  										  .data_addr = 0x60080000, //адрес для записи данных
										  .reset_port = 0,		   //порт для вывода Reset (0, если вывод Reset не используется)
										  .reset_pin = 0,		   //пин порта вывода Reset
										  //DMA доллжно быть настроено в режиме память-память
										  .dma = DMA2,			   //контроллер DMA (0, если DMA не используется)
										  .dma_stream = 0 };	   //номер потока DMA

  	LCD_BackLight_data bklt_data = { .tim_bk = TIM3,		//Таймер и его канал, настроенный на генерацию ШИМ,
  									 .channel_tim_bk = 4,	//для плавного управления яркостью подсветки.
									 .bk_percent = 100,		//Яркость подсветки 0...100, %
									 .blk_port = 0,			//Порт и пин этого порта для случая управления
									 .blk_pin = 0 };		//подсветкой по типу вкл./выкл.

	//Для дисплея на контроллере ILI9488
	LCD = LCD_DisplayAdd( LCD,
						  480,
						  320,
						  ILI9488_CONTROLLER_WIDTH,
						  ILI9488_CONTROLLER_HEIGHT,
						  0,
						  0,
						  ILI9488_Init,
						  ILI9488_SetWindow,
						  ILI9488_SleepIn,
						  ILI9488_SleepOut,
						  ILI9488_SetOrientation,
						  &fsmc_data,
						  bklt_data );
	LCD_Handler *lcd = LCD; //указатель на первый дисплей в списке
	LCD_Init(lcd);
	LCD_SetOrientation(lcd, PAGE_ORIENTATION_LANDSCAPE);
	LCD_Fill(lcd, 0x319bb1);
	LCD_WriteString(lcd, 0, 0, "Hello, world!", &Font_15x25, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
	LCD_Delay(2000);

	//тесты заливки дисплея с DMA и без DMA
	uint32_t fps_dma = 0, fps_nodma = 0, n = 2;
	while (n--)
	{
		LCD_Fill(LCD, 0x319bb1);
		LCD_WriteString(LCD, 0, 0, "Example FSMC LCD -> Start", &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		LCD_WriteString(LCD, 0, 20, "DMA mode - ", &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		if (LCD->fsmc_data.dma)
			LCD_WriteString(LCD, LCD->AtPos.x, LCD->AtPos.y, "Enabled", &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		else
			LCD_WriteString(LCD, LCD->AtPos.x, LCD->AtPos.y, "Disabled", &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		LCD_Delay(2000);
		uint32_t fps = demo_fill(LCD);
		LCD_WriteString(LCD, 0, 0, "Example FSMC LCD -> End", &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		if (LCD->fsmc_data.dma) {
			fps_dma = fps;
			LCD->fsmc_data.dma = 0;
		}
		else {
			fps_nodma = fps;
			LCD->fsmc_data.dma = DMA2;
		}
		char buff[10];
		utoa(fps_dma, buff, 10);
		LCD_WriteString(LCD, 0, 20, "fps_dma_on = ", &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		LCD_WriteString(LCD, LCD->AtPos.x, LCD->AtPos.y, buff, &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		utoa(fps_nodma, buff, 10);
		LCD_WriteString(LCD, 0, 40, "fps_dma_off = ", &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		LCD_WriteString(LCD, LCD->AtPos.x, LCD->AtPos.y, buff, &Font_12x20, COLOR_YELLOW, 0x319bb1, LCD_SYMBOL_PRINT_FAST);
		if (fps_dma && fps_nodma) {
			if (fps_dma > fps_nodma) {
				LCD_WriteString(LCD, 0, 60, "DMA Winner :)", &Font_12x20, COLOR_RED, COLOR_WHITE, LCD_SYMBOL_PRINT_FAST);
			}
			else if (fps_dma < fps_nodma){
				LCD_WriteString(LCD, 0, 60, "DMA Loser :(", &Font_12x20, COLOR_RED, COLOR_WHITE, LCD_SYMBOL_PRINT_FAST);
			}
			else {
				LCD_WriteString(LCD, 0, 60, "DMA Parity", &Font_12x20, COLOR_RED, COLOR_WHITE, LCD_SYMBOL_PRINT_FAST);
			}
		}
		LCD_Delay(3000);
	}
	LCD->fsmc_data.dma = DMA2;

	//демо декодера jpeg
	demo_jpg(lcd, &picture_jpg_test);

#if 1
	/* ----------------------------------- Настройка тачскрина ------------------------------------------*/
	//Будем обмениваться данными с XPT2046 на скорости 1.3125 Мбит/с (по спецификации максимум 2.0 Мбит/с).
	XPT2046_ConnectionData cnt_touch = { .spi 	 = SPI2,			//Используемый spi
	  	  	  	  	  	  	  	   		 .speed 	 = 4,			//Скорость spi 0...7 (0 - clk/2, 1 - clk/4, ..., 7 - clk/256)
										 .cs_port  = GPIOB,			//Порт для управления T_CS
										 .cs_pin 	 = GPIO_PIN_12,	//Вывод порта для управления T_CS
										 .irq_port = GPIOC,			//Порт для управления T_PEN
										 .irq_pin  = GPIO_PIN_5,	//Вывод порта для управления T_PEN
										 .exti_irq = EXTI9_5_IRQn  	//Канал внешнего прерывания для T_PEN
		  	  	  	  	  	  	  	   };
	//Инициализация обработчика XPT2046
	XPT2046_Handler touch1;
	XPT2046_InitTouch(&touch1, 20, &cnt_touch);

	/* Калибровка тачскрина
	 * Если коэффициенты калибровки выше определены, то эту строку надо
	 * закомментировать */
	XPT2046_CalibrateTouch(&touch1, lcd); //Запускаем процедуру калибровки

	//----------------------------------------- Запуск демок --------------------------------------------*/

	//Демка для рисования на экране с помощью тачскрина.
	Draw_TouchPenDemo(&touch1, lcd);

	//Демка рисует примитивы, отображает температуру и позволяет перемещать круг по дисплею.
	//При удержании касания окрашивает дисплей случайным цветом.
	RoadCircleDemo(&touch1, lcd);

	/* --------------------------------------------------------------------------------------------------*/
#endif

	//демо построчного рендеринга объектов
	render_buf1 = malloc(RENDER_BUFFER_LINES * lcd->Width * sizeof(uint16_t));
	if (lcd->fsmc_data.dma) {
		render_buf2 = malloc(RENDER_BUFFER_LINES * lcd->Width * sizeof(uint16_t));
	}

	demo_gl(lcd);

  while (1)
  {
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) { ; }
}

//Non maskable interrupt
void NMI_Handler(void)
{
   while (1) { ; }
}

//Hard fault interrupt
void HardFault_Handler(void)
{
  while (1) { ; }
}

//Memory management fault
void MemManage_Handler(void)
{
  while (1) { ; }
}

//Pre-fetch fault, memory access fault
void BusFault_Handler(void)
{
  while (1) { ; }
}

//Undefined instruction or illegal state
void UsageFault_Handler(void)
{
  while (1)  { ; }
}

//System service call via SWI instruction
void SVC_Handler(void)
{
}

//Debug monitor
void DebugMon_Handler(void)
{
}

//Pendable request for system service
void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
	XPT2046_TIMCallback(touch);
	millis++;
}

void EXTI9_5_IRQHandler(void)
{
	if (EXTI->PR & EXTI_PR_PR5)
	{
		EXTI->PR = EXTI_PR_PR5;
		XPT2046_EXTICallback(touch);
	}
}

void DMA2_Stream0_IRQHandler(void)
{
#if 0
	Display_TC_Callback(DMA2, 0);
#else
	DMA2->LIFCR = 0x3d;
	if (LCD->size_mem > 65535) {
		DMA2_Stream0->NDTR = 65535;
		LCD->size_mem -= 65535;
	}
	else if (LCD->size_mem) {
		DMA2_Stream0->NDTR = LCD->size_mem;
		LCD->size_mem = 0;
	}
	else {
		LCD->dma_complete = 1;
		return;
	}
	DMA2_Stream0->CR |= DMA_SxCR_EN;
#endif
}

