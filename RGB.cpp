#include "RGB.h"
#include <util/atomic.h>
#include "millis.h"


// Constructor. Start with leds off.
RGBController::RGBController()
{
  currentRGBColor.red = 0;
  currentRGBColor.green = 0;
  currentRGBColor.blue = 0;
  fading_max_steps_ = 500;
  fading_step_time_ = 100;
  holding_color_ = 1000;
  fading_ = false;
  last_update_ = millis();
}

/*
Change instantly the LED colors.
@param r The red (0..255)
@param g The green (0..255)
@param b The blue (0..255)
*/
void RGBController::setRGB(uint16_t r, uint16_t g, uint16_t b) {
  currentRGBColor.red = constrain(r, 0, 255);
  currentRGBColor.green = constrain(g, 0, 255);
  currentRGBColor.blue = constrain(b, 0, 255);
  fading_ = false;
}

void RGBController::setRGB(uint32_t color) {
  setRGB((color & 0xFF0000) >> 16, (color & 0x00FF00) >> 8, color & 0x0000FF);
}

void RGBController::setRGB(RGBColor color) {
	setRGB(color.red, color.green, color.blue);	
}

/*
Fade from current color to the one provided.
@param r The red (0..255)
@param g The green (0..255)
@param b The blue (0..255)
*/
void RGBController::fadeRGB(uint16_t r, uint16_t g, uint16_t b) {
  initialRGBColor.red = currentRGBColor.red;
  initialRGBColor.green = currentRGBColor.green;
  initialRGBColor.blue = currentRGBColor.blue;
  targetRGBColor.red = r;
  targetRGBColor.green = g;
  targetRGBColor.blue = b;
  fading_ = true;
  fading_step_ = 0;
}

void RGBController::fadeRGB(uint32_t color) {
  fadeRGB((color & 0xFF0000) >> 16, (color & 0x00FF00) >> 8, color & 0x0000FF);
}

void RGBController::fadeRGB(RGBColor color) {
  fadeRGB(color.red, color.green, color.blue);
}

/*
This function needs to be called in the loop function.
*/
void RGBController::tick() {
  unsigned long current_millis = millis();
  if (fading_) {
    // Enough time since the last step ?
    if (current_millis - last_update_ >= fading_step_time_) {
      fading_step_++;
      fade();
      if (fading_step_ >= fading_max_steps_) {
        fading_ = false;
      }
      last_update_ = current_millis;
    }
  }
  set(currentRGBColor);
}

/*  Private functions
------------------------------------------------------------ */

/*
This function is used internaly to do the fading between colors.
*/
void RGBController::fade()
{
	currentRGBColor.red = (uint16_t)(initialRGBColor.red - (fading_step_*((initialRGBColor.red-(float)targetRGBColor.red)/fading_max_steps_)));
	currentRGBColor.green = (uint16_t)(initialRGBColor.green - (fading_step_*((initialRGBColor.green-(float)targetRGBColor.green)/fading_max_steps_)));
	currentRGBColor.blue = (uint16_t)(initialRGBColor.blue - (fading_step_*((initialRGBColor.blue-(float)targetRGBColor.blue)/fading_max_steps_)));		
}

/* 
* Инициализация каналов RGB. Настройка таймеров для ШИМ
* Red - PD5, 8bit OC0B
* Green - PD3, 8bit OC2B
* Blue - PB3, 8bit OC2A
*/
void RGBController::init(void) {

	/*
	 * Таймер 0 (8 бит) - Канал А - для millis, канал B - ШИМ красного цвета
	 * Таймер 1 (16 бит) - Для ИК-приемника. См. IRemoteInt.h
	 * Таймер 2 (8 бит) - Канал А - ШИМ синего цвета, канал B - ШИМ зеленого цвета
	 */

	TCCR0A = 1<<COM0A1 | 1<<COM0B1 | 1<<WGM01 | 1<<WGM00;	   // Неинвертирующий режим на OC0A и OC0B, Режим 3, СКОРОСТНОЙ ШИМ
	//TCCR0B = (1<<CS02) | (1<<CS00);							   // Делитель частоты 1024
	TCCR0B = 1<<CS00;                                          

  	TCCR2A = 1<<COM2A1 | 1<<COM2B1 | 1<<WGM21 | 1<<WGM20;      // Неинвертирующий режим на OC2A и OC2B, Режим 3, СКОРОСТНОЙ ШИМ
  	TCCR2B = 1<<CS20;                                          // Без делителя частоты
}

/*
* Разбор цветового массива, полученного от управляющей программы на цветовые составляющие
* Возвращает структуру SColorRGB
*/
RGBColor RGBController::parse(char rgb_command[5]) {
	RGBColor currentColor; 
	
	currentColor.red = rgb_command[2];
	currentColor.green = rgb_command[3];
	currentColor.blue = rgb_command[4];

	return currentColor;
}

/*
* Установка цветов на каналах RGB. В случае, если цвет канала равен 0,
* канал отключается через регистр DDRx из-за особенностей генератора ШИМ
*/	
void RGBController::set(RGBColor rgb_color) {
//RGBColorыключаем красный канал, если цввет 0x00
	if(rgb_color.red == 0)
		RGB_RED_DDR &= ~(1 << RGB_RED_PIN);
	else
		RGB_RED_DDR |= (1 << RGB_RED_PIN);

	// Выключаем зеленый канал, если цввет 0x00
	if(rgb_color.green == 0)
		RGB_GREEN_DDR &= ~(1 << RGB_GREEN_PIN);
	else
		RGB_GREEN_DDR |= (1 << RGB_GREEN_PIN);

	// Выключаем синий канал, если цввет 0x00
	if(rgb_color.blue == 0)
		RGB_BLUE_DDR &= ~(1 << RGB_BLUE_PIN);
	else
		RGB_BLUE_DDR |= (1 << RGB_BLUE_PIN);
	
	RGB_RED_OCR = rgb_color.red;
	RGB_GREEN_OCR = rgb_color.green;
	RGB_BLUE_OCR = rgb_color.blue;
}
