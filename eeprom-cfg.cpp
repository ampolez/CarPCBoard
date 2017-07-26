/*
 * eeprom-cfg.cpp
 *
 * Created: 06.07.2015 18:39:51
 *  Author: Андрей
 */ 

#include "eeprom-cfg.h"

/*
 * Переменные в EEPROM для хранения настроек RGB-подсветки
 */
uint8_t EEMEM eeprom_RGB_mode;					// Режим работы подсветки
//uint8_t EEMEM eeprom_RGB_defaultStepsPerSecond; // Количество шагов в секунду для RGB перехода (шт)
uint8_t EEMEM eeprom_RGB_TransitionsNumber;		// Количество цветовых переходов в эффекте (шт)
uint8_t EEMEM eeprom_RGB_defaultColor[3];		// Цвет подсветки по-умолчанию в режиме одного цвета
uint8_t EEMEM eeprom_RGB_defaultColors[RGB_TRANSITION_MAX_TRANSITIONS * RGB_TRANSITION_BYTES_PER_STEP];		// Набор цветовых переходов по-умолчанию. До 16 переходов: 3 байта - цвета RGB, 2 байта - длительность, 1 байт - число шагов

/*
 * Конструктор. Получает режим подсветки. 
 * В зависимости от режима, получает остальные параметры.
 */
EEPROMConfig::EEPROMConfig() {
	this->RGBGetMode();
	switch(this->RGBConfig.Mode)	{
		case COLOR:
			this->RGBGetColor();
		break;
		
		case TRANSITION:
			this->RGBGetTransition();
		break;
			
		case AUTOMATIC:
		case OFF:
		break;
	}
}

/*
 * Установка режима RGB подстветки
 */
void EEPROMConfig::RGBSetMode(RGB_modes mode) {
	eeprom_update_byte(&eeprom_RGB_mode, mode);	
	this->RGBConfig.Mode = mode;
}

/*
 * Установка цвета подстветки по-умолчанию
 * Данный цвет устанавливается при включении контроллера до получения команд от хоста
 */
void EEPROMConfig::RGBSetColor(char colorData[5]) {
	eeprom_update_block(&colorData[2], &eeprom_RGB_defaultColor[0], 3);		
	this->RGBConfig.Color.red = colorData[2];
	this->RGBConfig.Color.green = colorData[3];
	this->RGBConfig.Color.blue = colorData[4];
}

/*
 * Установка параметров цветового перехода по-умолчанию
 */
void EEPROMConfig::RGBSetTransition(char colorData[RGB_TRANSITION_MAX_BYTES]) {
	int frameDataSize = colorData[1]; 
	this->RGBConfig.TransitionsNumber = colorData[frameDataSize + 1];	// ??!
	//this->RGBConfig.StepsPerSecond = colorData[frameDataSize + 1];
	
	for(uint8_t i = 2; i < RGB_TRANSITION_MAX_BYTES - 1; i++) {
		this->RGBConfig.colorSequence[i-2] = colorData[i];
	}

	eeprom_update_byte(&eeprom_RGB_TransitionsNumber, this->RGBConfig.TransitionsNumber);	
	//eeprom_update_byte(&eeprom_RGB_defaultStepsPerSecond, this->RGBConfig.StepsPerSecond);	
	eeprom_update_block(&colorData[2], &eeprom_RGB_defaultColors[0], this->RGBConfig.TransitionsNumber * RGB_TRANSITION_BYTES_PER_STEP);
}

/*
 * Получение режима подстветки по-умолчанию
 */
void EEPROMConfig::RGBGetMode() {
	this->RGBConfig.Mode = eeprom_read_byte(&eeprom_RGB_mode);
}

/*
 * Получение цвета подстветки по-умолчанию
 */
void EEPROMConfig::RGBGetColor() {
	uint8_t ccolor[3];
	eeprom_read_block(ccolor, eeprom_RGB_defaultColor, 3);
	this->RGBConfig.Color.red = ccolor[0];
	this->RGBConfig.Color.green = ccolor[1];
	this->RGBConfig.Color.blue = ccolor[2];		
}

/*
 * Получение параметров цветового перехода по-умолчанию
 */
void EEPROMConfig::RGBGetTransition() {
	this->RGBConfig.TransitionsNumber = eeprom_read_byte(&eeprom_RGB_TransitionsNumber);
	//this->RGBConfig.StepsPerSecond = eeprom_read_byte(&eeprom_RGB_defaultStepsPerSecond);
	eeprom_read_block(this->RGBConfig.colorSequence, &eeprom_RGB_defaultColors[0], this->RGBConfig.TransitionsNumber * RGB_TRANSITION_BYTES_PER_STEP);
}