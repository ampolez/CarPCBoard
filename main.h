/*
 * main.h
 *
 * Created: 14.01.2015 2:02:17
 *  Author: Андрей
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

	#define SIZEOF_ARRAY(a) (sizeof(a) / sizeof(a[0]))
	#define UART_BAUD_RATE 57600			// Скорость передачи UART
	#define UART_RX_COMMAND_SIZE 512		// Максимальный размер входящей UART-команды в байтах
	
	#define STATUS_RED 2					// нога МК для карсного статусного диода
	#define STATUS_GREEN 4					// нога МК для зеленого статусного диода

	#define MAXSENSORS 5					// Max число датчиков а шине 1-Wire
	#define IR_DATA_LENGTH 4				// размер блока данных кадра ИК-приемника
	#define RGB_DATE_LENGTH 4				// размер блока данных кадра RGB

	#define TRUE 1
	#define FALSE 0
	#define CHAR_NEWLINE '\n'
	#define CHAR_RETURN '\r'
	#define NEWLINESTR "\r\n"	
	#define FRAME_END 0xF0

	/*
	 * Перечислитель типов кадров
	 */
	enum frame_marker {
        UNDEFINED = 0x00,
		ERROR = 0xFF,
        RESET = 0xFC,
        INFRARED = 0x10,
        RESISTIVE = 0x20,
        TEMPERATURE_SENSOR = 0x30,
        TEMPERATURE_ENUM = 0x32,
        RGB_COLOR = 0x40,
        RGB_MODE = 0x41
	};
	
	/*
	 * Возможные ошибки при работе с датчиками температуры
	 */
	enum error_codes {
		ERROR_NONE = 0x00,						// Ошибок нет
		ERROR_UNKNOWN = 0xFF,					// Неизвестная ошибка
		ERROR_TEMP_NO_SENSORS = 0x31,			// Датчики температуры не обнаружены
		ERROR_TEMP_CRC = 0x32,					// Ошибка контрольной суммы при обращении к датчику, возможно потеряно соединение
		ERROR_TEMP_MEASURE_FAILED = 0x33,		// Ошибка измерения температуры, возможно короткое замыкание
		ERROR_TEMP_BUS_NO_SENSOR = 0x34,		// Ошибка шины 1-Wire - нет датчиков
		ERROR_TEMP_BUS_DATA = 0x35,				// Ошибка шины 1-Wire - ошибка в полученных данных
		ERROR_RGB_INCORRECT_COLOR_FRAME = 0x41	// Неверный пакет с цветовыми данными	
	};

	/* 
	 * Структура, описывающая датчики температуры
	 */
	typedef struct {
		unsigned char hash[8];						// Уникальный идентификатор датчика  
		unsigned char val[2];						// Последнее значение температуры
	} ds18x20_sensor;


	template <typename T, size_t size>				// Шаблон типа для данных кадра
	void uart_send_frame (frame_marker marker, T (&data)[size]);
	void uart_send_frame(frame_marker marker);
	void uart_put_int(uint8_t value);
	void uart_print_value (char *id, int value);
	void uart_copy_command ();
	void uart_process_command();
	void uart_process();
	int uart_parse_assignment (char input[16]);
	uint8_t uart_put_temp_maxres(int32_t tval);
	static uint8_t ds18b20_Enumerate(void);
	void ds18b20_GetTemp(uint8_t sensorID);
	void timerDelayMs(unsigned int ms);
	void blink_Red(void);
	
	


#endif /* MAIN_H_ */