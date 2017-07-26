/*
 * rtc_millis.h
 *
 * Created: 25.10.2015 18:33:16
 *  Author: ampolez
 */ 


#ifndef RTC_MILLIS_H_
#define RTC_MILLIS_H_

#define RTC_PORT PORTD
#define RTC_SQW_PIN 2
#define RTC_DDR DDRD

#define TIKS_1MS (F_CPU/64/1000)		// Размер одного тика таймера, заменяющего 1 мс в delay_ms

/**
* Типы для хранения числа миллисекунд
* Тип					- Макс. отрезок времени	- Объем памяти
* unsigned char			- 255 мс				- 1 байт
* unsigned int			- 65.54 с				- 2 байта
* unsigned long			- 49.71 дней			- 4 байта
* unsigned long long	- 584.9 млн лет			- 8 байт
*/
typedef unsigned long rtc_millis_t;

void rtc_millis_init(void);			// Подготовка оборудования и запуск счетчика миллисекунд
void rtc_millis_reset(void);		// Сброс счетчика миллисекунд
rtc_millis_t rtc_millis(void);		// Получение текущего числа миллисекунд с момента запуска счетчика


#endif /* RTC_MILLIS_H_ */