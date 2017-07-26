#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "millis.h"

static volatile millis_t msCount;

void millis_init()
{
	// Регистры TCCR настраиваются в модуле RGB.h, метод Init 
	TIMSK0 = 1<<OCIE0A;			// Включаем прерывание по переполнению регистра A
	OCR0A = TIKS_1MS;			// Значение для сравнения - 1мс
}

millis_t millis_get()
{
	millis_t ms;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ms = msCount;
	}
	return ms;
}

ISR(TIMER0_COMPA_vect)
{
	++msCount;
	//do { __asm__ __volatile__ ("nop"); } while (0);
}
