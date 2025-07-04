/**
 * @file main.c
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "Modules/dht11Driver.h"
#include <wiringPi.h>

#define DHTPIN      7

int dht11_dat[5] = { 0, 0, 0, 0, 0 };
float   f;

int main( void )
{
    printf( "Raspberry Pi wiringPi DHT11 Temperature test program\n" );

    if ( wiringPiSetup() == -1 )
    {
        fprintf(stderr, "Failed to initialize WiringPi. Exiting.\n");
        return 1; // Use return 1 for error exit
    }

    while ( 1 )
    {
        read_dht11_dat(dht11_dat, DHTPIN);

        // Check for error condition from dht11Driver
        if (dht11_dat[0] == -2000) //Known error value
        {
            fprintf(stderr, "Error DHT11 data...\n");
        }
        else
        {
            
            f = dht11_dat[2] * 9. / 5. + 32;
            printf( "Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",
                    dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f );
        }

        delay( 1000 );
    }

    return 0; // Should not be reached in an infinite loop, but good practice
}