/*
 * rgb.h
 *
 * Created: 16.01.2015 14:03:55
 *  Author: Андрей
 */ 


#ifndef RGB_H_
#define RGB_H_

	#include <avr/io.h>

	/*
	 * Определения ног МК, на которых работают каналы RGB
	 */
	#define RGB_RED 5	// DDRD.5
	#define RGB_BLUE 3	// DDRB.3
	#define RGB_GREEN 3 // DDRD.3

	/*
	 * Структура, описывающая цвета каналов
	 */
	typedef struct
	{
		uint8_t red;
		uint8_t green;
		uint8_t blue;
	} SColorRGB;

	/* 
	 * Инициализация каналов RGB. Настройка таймеров для ШИМ
	 * Red - PD5, 8bit OC0B
	 * Green - PD3, 8bit OC2B
	 * Blue - PB3, 8bit OC2A
	 */
	void rgb_init(void);

	/*
	 * Установка цветов на каналах RGB. В случае, если цвет канала равен 0,
	 * канал отключается через регистр DDRx из-за особенностей генератора ШИМ
	 */	
	void rgb_set_color(SColorRGB rgb_color);
	
	/*
	 * Разбор цветового массива, полученного от управляющей программы на цветовые составляющие
	 * Возвращает структуру SColorRGB
	 */
	SColorRGB rgb_parse(char rgb_command[5]);
	

#endif /* RGB_H_ */