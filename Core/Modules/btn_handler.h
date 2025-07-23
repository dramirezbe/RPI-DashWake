/**
 * @file btn_handler.h
 * @author Leandro
 */
#ifndef BTN_HANDLER_H
#define BTN_HANDLER_H

#include <wiringPi.h>
#include <stdio.h>

//btn_flag as global extern var
extern volatile int button_flag;

// Function to initialize the button and interrupt
void button_init(uint8_t btn_pin);

// Interrupt Service Routine (ISR) to be called when the button is pressed
void button_isr(void);

#endif // BTN_HANDLER_H