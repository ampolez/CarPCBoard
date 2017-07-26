/*
 * common.h
 *
 * Created: 20.01.2015 0:27:09
 *  Author: Андрей
 */ 


#ifndef COMMON_H_
#define COMMON_H_

/* 
 * Структура, описывающая датчики температуры
 */
typedef struct {
	unsigned char hash[8];						// Уникальный идентификатор датчика  
	unsigned char val[2];						// Последнее значение температуры
} ds18x20_sensor;



#endif /* COMMON_H_ */