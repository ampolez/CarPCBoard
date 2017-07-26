/*
  Created by Harold Waterkeyn, February 1, 2012
  V0.3 : Feb 4, 2012
  
  Credits:
  Inspired from the Moodlight Library by Kasper Kamperman
    http://www.kasperkamperman.com/blog/arduino-moodlight-library/
*/
#include <stdint.h>
#include <avr/io.h>

// Настройки цветовых каналов
// Регистры направлянеия (каналы подключения цветов)
#define RGB_RED_DDR	DDRD
#define RGB_GREEN_DDR	DDRD
#define RGB_BLUE_DDR	DDRB

// Регистры сравнения таймеров для ШИМ (значения цветов)
#define RGB_RED_OCR	OCR0B
#define RGB_GREEN_OCR	OCR2B
#define RGB_BLUE_OCR	OCR2A

// Пины для подключения
#define RGB_RED_PIN	5	
#define RGB_GREEN_PIN	3 
#define RGB_BLUE_PIN	3

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

#ifndef GUARD_RGB
#define GUARD_RGB

/*
* Структура, описывающая цвета каналов
*/
typedef struct
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} RGBColor;

class RGBController {
  public:
	unsigned long last_update_;							// Последний момент времени, когда что-либо происходило
	
    RGBController();
	
	void init(void);									// Настроить таймеры для работы с ШИМ    
	void tick(void);									// Обновить цвет подсветки, если это необходимо. (Вызывать в основном цикле)	
	RGBColor parse(char rgb_command[5]);				// Разобрать строку UART и преобразовать в структуру цвета
		
	void setRGB(uint16_t, uint16_t, uint16_t);     // Установить значение цвета подстветки в палитре RGB
    void setRGB(uint32_t); 
	void setRGB(RGBColor);
	
    void fadeRGB(uint16_t, uint16_t, uint16_t);    // Плавно перейти к заданному цвету (заданному в палитре RGB).
    void fadeRGB(uint32_t); 
	void fadeRGB(RGBColor);
        
	bool isFading(void) {return fading_;}     // True we are currently fading to a new color.
    bool isStill(void) {return not fading_;}  // True if we are not fading to a new color.
    
	void setHoldingTime(uint16_t t) {holding_color_ = t;}     // How many ms do we keep a color before fading to a new one.
    void setFadingSpeed(uint16_t t) {fading_step_time_ = t;}  // How many ms between each step when fading.
    void setFadingSteps(uint16_t t) {fading_max_steps_ = t;}  // How many steps for fading from a color to another.
  
	uint16_t red(void) {return currentRGBColor.red;}                // The current red color.
    uint16_t green(void) {return currentRGBColor.green;}              // The current green color.
    uint16_t blue(void) {return currentRGBColor.blue;}               // The current blue color.
     
  private:
	RGBColor currentRGBColor;
	RGBColor initialRGBColor;
	RGBColor targetRGBColor;	

	void set(RGBColor rgb_color);	// Записать значение цвета в регистры таймеров ШИМ
    
	uint16_t fading_step_;      // Current step of the fading.
    uint16_t fading_max_steps_; // The total number of steps when fading.
    uint16_t fading_step_time_; // The number of ms between two variation of color when fading.
    uint16_t holding_color_;    // The number of ms to hold color before fading again. (when cycling i.e. mode_ != 0)
    
	bool fading_;               // Are we fading now ?
    void fade();                // Used internaly to fade
};

#endif