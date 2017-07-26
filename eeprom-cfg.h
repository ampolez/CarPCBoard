/*
 * eeprom-cfg.h
 *
 * Created: 06.07.2015 18:40:15
 *  Author: Андрей
 */ 
#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <string.h>
#include "RGB.h"

#ifndef EEPROM_CFG_H_
#define EEPROM_CFG_H_

/*
 * Режимы работы RGB-подстветки. 
 */
enum RGB_modes {
	OFF = 0x00,			// Подсветка выключена
	COLOR = 0x01,		// Подсветка одним цветом
	TRANSITION = 0x02,	// Цветовой переход
	AUTOMATIC = 0x03	// Автоматический режим
};

/*
 * Открытая структура данных, описывающая параметры RGB-подсветки
 */
typedef struct {
	uint8_t Mode;												// Режим подствеки
	RGBColor Color;												// Цвет по-умолчанию в режиме одного цвета
	//uint8_t StepsPerSecond;									// Число шагов перехода за секунду
	uint8_t TransitionsNumber;									// Число переходов в режиме смены цветов
	char colorSequence[RGB_TRANSITION_MAX_BYTES - RGB_TRANSITION_SERVICE_BYTES];			// Массив цветовых переходов
} RGBsettings;

class EEPROMConfig {
	public:
		RGBsettings RGBConfig;
		
		EEPROMConfig();
		void RGBSetMode(RGB_modes mode);	
		void RGBSetColor(char colorData[5]);		
		void RGBSetTransition(char colorData[RGB_TRANSITION_MAX_BYTES]);

	private:
		void RGBGetMode();
		void RGBGetColor();
		void RGBGetTransition();
};



#endif /* EEPROM_CFG_H_ */