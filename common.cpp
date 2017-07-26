/*
 * common.cpp
 *
 * Created: 20.01.2015 0:26:57
 *  Author: Андрей
 */ 

#include "common.h"

// One-wire Temperature
unsigned char	owDevicesIDs[MAXDEVICES][8];	// идентификаторы всех устройств на шине 1-Wire
ds18x20_sensor	temp_sensor[MAXDEVICES];		// структура датчиков


/*
 * Поиск всех устройств на шине 1-Wire
 * Функция возвращает количество найденных устройств
 */
unsigned char	ow_search_devices(void)
{
	unsigned char	i;
	unsigned char	id[OW_ROMCODE_SIZE];
	unsigned char	diff, sensors_count;

	sensors_count = 0;

	for( diff = OW_SEARCH_FIRST; diff != OW_LAST_DEVICE && sensors_count < MAXDEVICES ; )
	{
		OW_FindROM( &diff, &id[0] );
		if( diff == OW_PRESENCE_ERR ) break;
		if( diff == OW_DATA_ERR )	break;
		for (i=0;i<OW_ROMCODE_SIZE;i++)
		owDevicesIDs[sensors_count][i] = id[i];
		sensors_count++;
	}
	return sensors_count;
}

/*
 * Функция опрашивает датчик с порядковым номером sensor_number на шине 1-Wire
 */
void ds18x20_get_sensor_value(int sensor_number) {
	unsigned char	data[2];											// буфер для значения температуры
	unsigned char	sensor_value[2];									// Символьный буфер для температуры
	DS18x20_StartMeasure(owDevicesIDs[sensor_number]);					// запускаем измерение
	timerDelayMs(800);													// ждем минимум 750 мс, пока конвентируется температура
	DS18x20_ReadData(owDevicesIDs[sensor_number], data);				// считываем данные
	DS18x20_ConvertToThemperature(data, sensor_value);					// преобразовываем температуру в человекопонятный вид
	memcpy(temp_sensor[sensor_number].val, sensor_value, 2);			// записываем значение температуры в структуру, описывающую датчики с указанным порядковым номером
}

/*
 * Функция опрашивает шину 1-Wire в поисках датчиков температуры DS18B20
 * Записывает уникальные коды всех датчиков в структуру датчиков
 * Если uart_send_enum = true отправляется UART кадр типа TEMPERATURE_ENUM, 
 * содержащий уникальный код датчика темпераутры
 * Возвращает количество найденных датчиков
 */
int ds18x20_enumerate(bool uart_send_enum = true)
{
	unsigned char	nDevices;									// количество устройств на шине 1-Wire
	int ds18b20_counter = 0;									// количество датчиков температуры
	
	nDevices = ow_search_devices();								// ищем все устройства
	for (unsigned char i=0; i < nDevices; i++)					// запрашиваем данные
	{
		// узнать устройство можно по его груповому коду, который расположен в первом байте адресса
		switch (owDevicesIDs[i][0])
		{
			case OW_DS18B20_FAMILY_CODE: {						// если найден термодатчик DS18B20
				ds18b20_counter++;
				memcpy(temp_sensor[i].hash, owDevicesIDs[i], 8);
				ds18x20_get_sensor_value(i);
				if(uart_send_enum) 
				{
					uart_send_frame(TEMPERATURE_ENUM,temp_sensor[i].hash);	// печатаем знак переноса строки, затем - адрес
				}
			} break;
		}
	}
	return ds18b20_counter;
}
