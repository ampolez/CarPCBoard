#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "millis.h"

static volatile millis_t msCount;

void millis_init()
{
	// �������� TCCR ������������� � ������ RGB.h, ����� Init 
	TIMSK0 = 1<<OCIE0A;			// �������� ���������� �� ������������ �������� A
	OCR0A = TIKS_1MS;			// �������� ��� ��������� - 1��
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
