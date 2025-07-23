/**
 * @file btn_handler.c
 * @author Leandro
 */

#include "btn_handler.h"

// Define the global flag
volatile int button_flag = 0;

// Interrupt handler function
void button_isr(void) {
    button_flag = 1;
    printf("[ISR] Button pressed. Flag set to 1.\n");
}

// Initialization function for button input and interrupt setup
void button_init(uint8_t btn_pin) {
    // Initialize the wiringPi system
    if (wiringPiSetup() == -1) {
        printf("Error initializing wiringPi.\n");
        return;
    }

    // Configure the pin as input and enable pull-up resistor
    pinMode(btn_pin, INPUT);
    pullUpDnControl(btn_pin, PUD_UP);

    // Attach interrupt to the button pin (falling edge)
    if (wiringPiISR(btn_pin, INT_EDGE_FALLING, &button_isr) < 0) {
        printf("Error setting up interrupt for button.\n");
        return;
    }

    printf("[Init] Button initialized on pin %lu.\n", btn_pin);
}
