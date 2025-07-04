/**
 * @file dht11Driver.h
 */

#include <wiringPi.h>
#include <stdio.h>

#define MAXTIMINGS  85

// Function prototype for read_dht11_dat
void read_dht11_dat(int *dht11_dat, int DHTPIN);