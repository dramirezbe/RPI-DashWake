/**
 * @file dht11Driver.h FAILED
 */
#ifndef DHT11DRIVER_H
#define DHT11DRIVER_H


#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAXTIMINGS  85

// Function prototype for read_dht11_dat
void read_dht11_dat(int *dht11_dat, int DHTPIN);

#endif
// DHT11DRIVER_H