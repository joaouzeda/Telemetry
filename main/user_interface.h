#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "stdint.h"
//#include "defines.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "I2CKeyPad.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

void LCDconfigureTWI(TwoWire *twi);
void LCDWriteChars(char* text,int len);
bool setup_lcd(void);
void keypad_setup(TwoWire *twi);
void run_update_screen();

typedef struct
{
    //inputs
    uint8_t screenIndex;
    void (*screenDisplay)();
    void (*onKeyDetectedCB)();
    char* triggerKeys;
} screen_definition_t;



void user_interface_ask_credentials();
void user_interface_run();

#endif // HAL_SIGNAL_INTERFACE_H
