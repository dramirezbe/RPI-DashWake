/**
 * @file main.c
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "Modules/dht11Driver.h"
#include "Modules/btn_handler.h"

#include <wiringPi.h>

#define DHT_PIN 7
#define BTN_PIN 0

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

    button_init(BTN_PIN);

    while ( 1 )
    {
        read_dht11_dat(dht11_dat, DHT_PIN);

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


        if (button_flag == 1)
        {
            printf("Button Pressed......................................");
            button_flag = 0;
        }
        

        delay( 1000 );
    }

    return 0;
}