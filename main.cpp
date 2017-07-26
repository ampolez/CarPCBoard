/*
 * main.cpp
 *
 * Created: 13.01.2015 12:34:33
 *  Author: Андрей
 */ 

//#define DEBUG_MODE

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "IRremote.h"

extern "C"
{
	#include "uart.h"
	#include "millis.h"
}
#include "RGB.h"
#include "onewire.h"
#include "ds18x20.h"
#include "main.h"


uint8_t gSensorIDs[MAXSENSORS][OW_ROMCODE_SIZE];	// идентификаторы всех устройств на шине 1-Wire
int32_t temp_eminus4[MAXSENSORS];					// массив температуры со всех датчиков
uint8_t nSensors = 0;								// Количество найденных датчиков
bool isError = false;								// Флаг общей ошибки

IRrecv irrecv(4, &DDRB, &PINB);						// ИК-приемник на 4 ноге порта B
decode_results results;								// объект с декодированным результатом ИК-сигнала

RGBController lightControl;


/*
 * Функция мигания красным диодом
 * Куда же без неё =)
 */
void blink_Red(void) 
{
	PORTD |= 1<<STATUS_RED;
	_delay_ms(50);
	PORTD &= ~(1<<STATUS_RED);
}

void(* resetFunc) (void) = 0;

#pragma region UART

unsigned char data_count = 0;						// длина текущей команды в буфере приёма UART
unsigned char uart_data_rx[UART_RX_COMMAND_SIZE];	// буфер приема UART
char uart_command_rx[UART_RX_COMMAND_SIZE];			// буфер комманды UART. рабочая копия буфера приёма

template <typename frame_data, size_t size>					// Шаблон типа для данных кадра

/*
 * Функция формирует кадр UART ди отправляет его в порт
 * Кадр составляется из:
 * - заголовка-идентификатора типа (1 байт)
 * - размера блока данных (1 байт)
 * - блока данных (размер в байтах указан во втором байте кадра)
 */
void uart_send_frame(frame_marker marker, frame_data (&data)[size]) {
	uint8_t data_size = size;
	uint8_t frame_buffer[data_size+2];

	frame_buffer[0] = marker;
	frame_buffer[1] = data_size;

	for(unsigned int i = 0; i < data_size; i++) {
		frame_buffer[i+2] = data[i];
	}

	for(unsigned int i = 0; i < SIZEOF_ARRAY(frame_buffer); i++){
		uart_putc(frame_buffer[i]);
	}
	uart_putc(FRAME_END);
}

void uart_send_frame(frame_marker marker) {
	uart_putc(marker);
	uart_putc(0x00);
	uart_putc(FRAME_END);
}

void uart_send_frame(error_codes errorCode) {
	uart_putc(ERROR);
	uart_putc(0x01);
	uart_putc(errorCode);
	uart_putc(FRAME_END);
}


/*
 * Функция отправляет по UART INT значение value
 * Отладночная функция для удобства представляения информации
 */
void uart_put_int(uint8_t value) {
	char *strval = NULL;
	itoa(value, strval, 10);
	uart_puts(strval);
}

/*
 * Функция отправляет по UART значение value переменной id
 * Отладночная функция для удобства представляения информации
 */
void uart_print_value (char *id, int value) {
	uart_puts(id);
	uart_putc('=');
	uart_put_int(value);
	uart_puts(NEWLINESTR);
}

/*
 * Функция разбирает строку, принятую по UART и возвращает значение переменной
 * Формат принимаемой строки: переменная=значение
 * Отладочная функция. В продакшене используются опкоды.
 */
int uart_parse_assignment (char input[16]) {
  char *pch;
  char cmdValue[16];
  // Find the position the equals sign is
  // in the string, keep a pointer to it
  pch = strchr(input, '=');
  // Copy everything after that point into
  // the buffer variable
  strcpy(cmdValue, pch+1);
  // Now turn this value into an integer and
  // return it to the caller.
  return atoi(cmdValue);
}

/*
 * Функция копирует данные из буфера UART и освобождает его
 */
void uart_copy_command () {
  memcpy(uart_command_rx, uart_data_rx, UART_RX_COMMAND_SIZE);				// Копируем содержимое UART буфера в uart_command_rx
  memset(uart_data_rx, 0, UART_RX_COMMAND_SIZE);							// Очищаем буфер UART uart_data_rx, чтобы его можно было использовать для следующей команды
}

/*
 * Функция обработки потока входящих данных UART
 * Проверяет переполнения, ошибки формирования кадра UART, соответствие скоростей
 */
void uart_process(){
  /* Get received character from ringbuffer
   * uart_getc() returns in the lower byte the received character and 
   * in the higher byte (bitmask) the last receive error
   * UART_NO_DATA is returned when no data is available.   */
  unsigned int c = uart_getc();
  
  if ( c & UART_NO_DATA ){
    // no data available from UART 
  }
  else {
	#ifdef DEBUG_MODE
		// new data available from UART check for Frame or Overrun error
		if ( c & UART_FRAME_ERROR ) {
		  /* Framing Error detected, i.e no stop bit detected */
		  uart_puts_P("UART Frame Error: ");
		}
		if ( c & UART_OVERRUN_ERROR ) {
		  /* Overrun, a character already present in the UART UDR register was 
		   * not read by the interrupt handler before the next character arrived,
		   * one or more received characters have been dropped */
		  uart_puts_P("UART Overrun Error: ");
		}
		if ( c & UART_BUFFER_OVERFLOW ) {
		  /* We are not reading the receive buffer fast enough,
		   * one or more received character have been dropped  */
		  uart_puts_P("Buffer overflow error: ");
		}
    #endif	

    // Add char to input buffer
    uart_data_rx[data_count] = c;
    
    // Return is signal for end of command input
    if (uart_data_rx[data_count] == CHAR_RETURN) {
      // Reset to 0, ready to go again
      data_count = 0;

	  #ifdef DEBUG_MODE	  
		uart_puts(NEWLINESTR);
	  #endif
      
      uart_copy_command();
      uart_process_command();
    } 
    else {
      data_count++;
    }
	#ifdef DEBUG_MODE    
		uart_putc( (unsigned char)c );
	#endif
  }
}

/*
 * Функция обработки команд, поступающих по UART
 * Отладочная функция
 */
void uart_process_command() {
  
  // В режиме отладки все команды пишутся в терминале в виде команда=значение
  // ответ устройства выводится в чистом текстовом виде
  #ifdef DEBUG_MODE
	  if(strcasestr(uart_command_rx,"LED_R") != NULL) {
		 if(strcasestr(uart_command_rx,"?") != NULL)
			uart_print_value("LED_R", lightControl.red());
		 else {
			lightControl.setRGB(uart_parse_assignment(uart_command_rx), lightControl.green(), lightControl.blue());
		 } 
	  }
	  else if(strcasestr(uart_command_rx,"LED_G") != NULL) {
		 if(strcasestr(uart_command_rx,"?") != NULL)
			 uart_print_value("LED_G", lightControl.green());
		 else {
			lightControl.setRGB(lightControl.red(), uart_parse_assignment(uart_command_rx), lightControl.blue());
		 }
	  }
	  else if(strcasestr(uart_command_rx,"LED_B") != NULL) {
		 if(strcasestr(uart_command_rx,"?") != NULL)
			uart_print_value("LED_B", lightControl.blue());
		 else {
			lightControl.setRGB(lightControl.red(), lightControl.green(), uart_parse_assignment(uart_command_rx));
		 }		
	  } else if(strcasestr(uart_command_rx, "TEMP") != NULL) {
			//uart_send_frame(GENERAL, temp_sensor[0].val);
			ds18b20_GetTemp(uart_parse_assignment(uart_command_rx));
	  } else if (strcasestr(uart_command_rx,"ENUM") != NULL) {
			ds18b20_Enumerate();
	  } else if (strcasestr(uart_command_rx,"FADE") != NULL) {
			lightControl.setRGB(255,255,255);
			lightControl.fadeRGB(uart_parse_assignment(uart_command_rx), 255, 0);
	  }
  #endif
  
  // в рабочем режиме используется uart_send_frame
  // команды принимаются опкодами из соответствующей структуры frame_marker
  #ifndef DEBUG_MODE	
		
		char rx_marker = uart_command_rx[0];
		uint8_t data_size = uart_command_rx[1];
		
		//blink_Red();		
		switch(rx_marker) {	
			case RESET:
				isError = false;
				uart_send_frame(RESET);
				resetFunc();
			break;	
			
			case RGB_COLOR:
				if(data_size == 3) {
					lightControl.setRGB(lightControl.parse(uart_command_rx));
				}
				// else {
				//	uart_send_frame(ERROR_RGB_INCORRECT_COLOR_FRAME);
				//}		
			break;
			
			case TEMPERATURE_ENUM: 
				isError = false;			
				nSensors = ds18b20_Enumerate();
			break;
			
			case TEMPERATURE_SENSOR:
				if(data_size == OW_ROMCODE_SIZE && nSensors > 0) {
					uint8_t sensorID = -1;
					uint8_t ROMcounter = 0;				
					for(int sensor = 0; sensor < nSensors; sensor++) {
						ROMcounter = 0;
						for(int romByte = 0; romByte < OW_ROMCODE_SIZE; romByte++) {
							if(gSensorIDs[sensor][romByte] == uart_command_rx[romByte+2]) {
								ROMcounter++;	
							} else {
								ROMcounter = 0;
								continue;
							}
						}
						if(ROMcounter == OW_ROMCODE_SIZE) {
							sensorID = sensor;
							break;
						}					
					}
					if(sensorID > -1) {
						ds18b20_GetTemp(sensorID);
					}
				}
			break;
			
			default:
			break;
		}

  #endif
}

#pragma endregion UART

#pragma region One-wire Temperature

/*
 * Функция опрашивает шину 1-Wire в поисках датчиков температуры DS18B20
 * Записывает уникальные коды всех датчиков в структуру датчиков
 * Если uart_send_enum = true, отправляется UART кадр TEMPERATURE_ENUM, 
 * содержащий уникальный код датчика темпераутры
 * Возвращает количество найденных датчиков
 */

static uint8_t ds18b20_Enumerate(void) {
	uint8_t i, error = 0;
	uint8_t id[OW_ROMCODE_SIZE];
	uint8_t diff;
	error_codes errorCode;	
	
	#ifdef DEBUG_MODE
		uart_puts( NEWLINESTR "Scanning Bus for DS18X20" NEWLINESTR );
	#endif
	
	ow_reset();
	nSensors = 0;

	diff = OW_SEARCH_FIRST;
	while ( diff != OW_LAST_DEVICE && nSensors < MAXSENSORS ) {
		DS18X20_find_sensor( &diff, &id[0] );

		if( diff == OW_PRESENCE_ERR ) {

			#ifdef DEBUG_MODE
				uart_puts( "No Sensor found" NEWLINESTR );
			#endif

			error++;
			errorCode = ERROR_TEMP_BUS_NO_SENSOR;
			break;
		}
		
		if( diff == OW_DATA_ERR ) {

			#ifdef DEBUG_MODE
				uart_puts( "Bus Error" NEWLINESTR );
			#endif
			
			error++;
			errorCode = ERROR_TEMP_BUS_DATA;
			break;
		}
			
		for ( i=0; i < OW_ROMCODE_SIZE; i++ ) {
			gSensorIDs[nSensors][i] = id[i];
		}
			
		nSensors++;

	}
	
	if(nSensors > 0) {
		
		#ifndef DEBUG_MODE
			uart_putc(TEMPERATURE_ENUM);
			uart_putc(nSensors * OW_ROMCODE_SIZE);
		#endif
	
		for ( i = 0; i < nSensors; i++ )
		{
			#ifdef DEBUG_MODE
				uart_puts( "Sensor# " );
				uart_put_int( (int)i+1 );
				uart_puts( " is a " );
				if ( gSensorIDs[i][0] == DS18S20_FAMILY_CODE )
				{
					uart_puts( "DS18S20/DS1820" );
				}
				else if ( gSensorIDs[i][0] == DS1822_FAMILY_CODE )
				{
					uart_puts( "DS1822" );
				}
				else
				{
					uart_puts( "DS18B20" );
				}
				uart_puts(NEWLINESTR);
			#else
				for(uint8_t j=0; j < OW_ROMCODE_SIZE; j++ ) {
					uart_putc(gSensorIDs[i][j]);
				}
			#endif		
		}
		uart_putc(FRAME_END);		
	} else {
		error++;
		errorCode = ERROR_TEMP_NO_SENSORS;
		uart_send_frame(TEMPERATURE_ENUM);
	}
	
	if( error ) {
		uart_send_frame(errorCode);
		error = 0;
	}	

	return nSensors;
}

/*
 * Функция выводит значение температуры на датчике c номером sensorID в UART
 */
void ds18b20_GetTemp(uint8_t sensorID) {
	#ifdef DEBUG_MODE
		char s[10];
		DS18X20_format_from_maxres( temp_eminus4[sensorID], s, 10 );
		uart_puts( s );
		uart_puts(NEWLINESTR);
	#else
		uint8_t result[OW_ROMCODE_SIZE+4] = {0x00};
		for(uint8_t j=0; j < OW_ROMCODE_SIZE; j++ ) {
			result[j] = gSensorIDs[sensorID][j];
		}			
		result[OW_ROMCODE_SIZE] = (temp_eminus4[sensorID] & 0x000000ff);
		result[OW_ROMCODE_SIZE+1] = (temp_eminus4[sensorID] & 0x0000ff00) >> 8;
		result[OW_ROMCODE_SIZE+2] = (temp_eminus4[sensorID] & 0x00ff0000) >> 16;
		result[OW_ROMCODE_SIZE+3] = (temp_eminus4[sensorID] & 0xff000000) >> 24;
		uart_send_frame(TEMPERATURE_SENSOR, result);
	#endif
	
}

/*
 * Фцнкция проведения измерений температуры. 
 * Считывает показания всех датчиков, найденных на шине
 * Помещает значения в общую структуру temp_eminus4
 * Если возникает ошибка - отправляет пакет с её кодом
 *
 */
void ds18b20_StartMeasure(uint8_t nSensors) {
	uint8_t error = 0, i;
	char errorCode[1] = {0x00};
	
	if ( nSensors == 0 )
	{
		error++;
		errorCode[0] = ERROR_TEMP_NO_SENSORS;
	}

	if ( DS18X20_start_meas( DS18X20_POWER_EXTERN, NULL ) == DS18X20_OK)
	{
		timerDelayMs(DS18B20_TCONV_12BIT);

		for ( i = 0; i < nSensors; i++ )
		{
			if ( DS18X20_read_maxres( &gSensorIDs[i][0], &temp_eminus4[i] ) != DS18X20_OK )
			{
				#ifdef DEBUG_MODE
					uart_puts( "CRC Error (lost connection?)" );
					uart_puts( NEWLINESTR );
				#endif
				
				error++;
				errorCode[0] = ERROR_TEMP_CRC;
			}
		}
	}
	else
	{
		#ifdef DEBUG_MODE
			uart_puts( "Start meas. failed (short circuit?)" );
		#endif
		error++;
		errorCode[0] = ERROR_TEMP_MEASURE_FAILED;
	}

	if ( error )
	{
		isError = true;
		#ifdef DEBUG_MODE
			uart_puts( "*** problems - rescanning bus ..." );	
			nSensors = ds18b20_Enumerate();
			uart_put_int( (int) nSensors );
			uart_puts( " DS18X20 Sensor(s) available" NEWLINESTR );
		#endif
		uart_send_frame(ERROR, errorCode);
		error = 0;
	}
	timerDelayMs(800);	
}

#pragma endregion One-wire Temperature


int main(void)
{	
//	cli();

	// Количество датчиков на шине 1-Wire
	uint8_t nSensors;

	// Статусные диоды
	DDRD |= 1<<STATUS_RED | 1<<STATUS_GREEN;		// красный и зеленый
	PORTD |= 1<<STATUS_GREEN;						// зеленый горит постоянно
	
	// Порт и прерывание для ИК-приемника
	DDRB &= ~(1<<4);								// PORTB4 в качестве входа
	PORTB |= 1<<4;									// подтягивающий резистор
	PCICR |= 1<<PCIE0;								// включаем прерывание при смене состояния пина
	PCMSK0 |= 1<<PCINT4;							// на порте PORTB4 используется прерывание PCINT4

	// Настраиваем шину 1-Wire
	ow_set_bus(&PINB,&PORTB,&DDRB,0);

	// Настраиваем счетчик миллисекунд для цветовых переходов
	millis_init();

	// Включаем ИК-приемник
	irrecv.enableIRIn();

	// Настраиваем UART 
	uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );

	// Настраиваем регистры для ШИМ таймеров RGB
	lightControl.init();
	lightControl.setRGB(255, 255, 255); // Start red.
	lightControl.fadeRGB(255, 0, 0);

	// Разрешаем прерывания
	sei();	

	// Опрашиваем датчики 1-Wire, считаем их данные и отчитываемся хосту
	nSensors = ds18b20_Enumerate();		// Основной бесконечный цикл
	for(;;) { 
		uart_process();							// Обрабатываем данные из UART
		lightControl.tick();					// Меняем состояние подсветки, если это требуется		
				
		if(nSensors > 0 && !isError) {	
			ds18b20_StartMeasure(nSensors);		// Запускаем считывание температуры с датчиков на шине
		}
		
	}
}

/*
 * Функция, заменяющая delay_ms встроенным таймером
 * Позволяет выполнять обработку UART и RGB-подсветку, пока считается задержка
 */
void timerDelayMs(unsigned int ms)
{
	while(ms--)
	{
		//TCNT0 = 0;
		while(TCNT0 < TIKS_1MS)
		{
			uart_process();
			lightControl.tick();
		}
	}

/*
	millis_init();

	unsigned long start = millis();
	unsigned long current = 0;	
	
	do {
		uart_process();
		lightControl.tick();
		current = millis();	
	}
	while(current < start + ms);
*/	
}

/* 
 * Обрабатываем прерывание по ноге ИК-приемника
 * При срабатывании включается функция декодирования ИК-сигнала и отправка его хосту
 */

ISR(PCINT0_vect) {
	if (irrecv.decode(&results))
	{
		if ((results.value > 0) && (results.value < 0xFFFFFFFF))
		{
			#ifdef DEBUG_MODE
				blink_Red();
			#endif		
			
			unsigned long res_dt = results.value;
			unsigned char ir_buffer[IR_DATA_LENGTH];			  // буфер для данных ИК-приемника размер задается дефайном в главном подключаемом файле
			ir_buffer[0] = res_dt & 0xFF;						  // преобразовать в 4-байта
			ir_buffer[1] = (res_dt & 0xFF00) >> 8;
			ir_buffer[2] = (res_dt & 0xFF0000) >> 16;
			ir_buffer[3] = (res_dt & 0xFF000000) >> 24;			
			uart_send_frame(INFRARED, ir_buffer);
		}
		irrecv.resume();
	}
}
