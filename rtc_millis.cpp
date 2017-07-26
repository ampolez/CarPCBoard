/*
 * TestTimer_AVRCpp.cpp
 * Author: ampolez
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "rtc_millis.h"

extern "C"
{
	#include "rtc.h"
	#include "twi.h"
}

static volatile rtc_millis_t msCount = 0;


/*
 * Настройка портов и прерываний. 
 * Подготовка перефирийного оборудования
 */
void rtc_millis_init(void) {

	// Настройки порта для подключения сигнала SQW таймера
	RTC_DDR &= ~(1 << RTC_SQW_PIN);		// Порт таймера является входом
	RTC_PORT |= (1 << RTC_SQW_PIN);		// Включен подтягивающий резистор
	
	// Настройка прерываний
	EICRA |= (1 << ISC01);    // Прерывание INT0 срабатывает при спаде сигнала
	EIMSK |= (1 << INT0);     // Активация прерывания INT0
	
	sei();

	twi_init_master();				// Инициализации шины I2c
	rtc_SQW_enable(true);			// Запуск генератора	
	rtc_SQW_set_freq(FREQ_1024);	// Установка частоты меандра 1024Гц
}

/*
 * Функция, возвращающая количество миллисекунд с момента запуска счетчика
 */
rtc_millis_t rtc_millis(void) {
	rtc_millis_t ms;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ms = msCount;
	}
	return ms;
}

/*
 * Функция для сброса счетчика миллисекунд
 */
void rtc_millis_reset(void) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		msCount = 0;
	}
}

/*
 *	Обработка прерывания по счетчику тиков от часов
 */
ISR(INT0_vect) 
{
	++msCount;		// Увеличение счетчика миллисекунд
}