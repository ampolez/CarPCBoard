/*
 * rgb.c
 *
 * Created: 16.01.2015 14:03:12
 *  Author: Андрей
 */ 

#include "rgb.h"

/* 
 * Инициализация каналов RGB. Настройка таймеров для ШИМ
 */
void rgb_init(void) {
  	TCCR2A = 1<<COM2A1 | 1<<COM2B1 | 1<<WGM21 | 1<<WGM20;      // Неинвертирующий режим на OC2A и OC2B, Режим 3, СКОРОСТНОЙ ШИМ
  	TCCR2B = 1<<CS20;                                          // Без делителя частоты
  	TCCR0A = 1<<COM0B1 | 1<<WGM01 | 1<<WGM00;				   // Неинвертирующий режим на OC0B, Режим 3, СКОРОСТНОЙ ШИМ
  	TCCR0B = 1<<CS00;                                          // Без делителя частоты	
}

SColorRGB rgb_parse(char rgb_command[5]) {
	SColorRGB currentColor; 
	
	currentColor.red = rgb_command[2];
	currentColor.green = rgb_command[3];
	currentColor.blue = rgb_command[4];

	return currentColor;
}

/*
 * Установка цветов на каналах RGB. В случае, если цвет канала равен 0,
 * канал отключается через регистр DDRx из-за особенностей генератора ШИМ
 */
void rgb_set_color(SColorRGB rgb_color) {
	
	// Выключаем красный канал, если цввет 0x00
	if(rgb_color.red == 0)
		DDRD &= ~(1 << RGB_RED);
	else
		DDRD |= (1 << RGB_RED);

	// Выключаем зеленый канал, если цввет 0x00
	if(rgb_color.green == 0)
		DDRD &= ~(1 << RGB_GREEN);
	else
		DDRD |= (1 << RGB_GREEN);

	// Выключаем синий канал, если цввет 0x00
	if(rgb_color.blue == 0)
		DDRB &= ~(1 << RGB_BLUE);
	else
		DDRB |= (1 << RGB_BLUE);
	
	OCR0B = rgb_color.red;
	OCR2B = rgb_color.green;
	OCR2A = rgb_color.blue;
}
