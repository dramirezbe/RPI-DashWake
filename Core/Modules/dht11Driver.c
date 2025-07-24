/**
 * @file dht11Driver.c  FAILED
 */

#include "dht11Driver.h"
#include <stdint.h> // Required for uint8_t

void read_dht11_dat(int *dht11_dat, int DHTPIN)
{
    uint8_t laststate   = HIGH;
    uint8_t counter     = 0;
    uint8_t j           = 0, i;

    // Initialize all data bytes to 0
    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    // Sensor communication start sequence
    pinMode( DHTPIN, OUTPUT );
    digitalWrite( DHTPIN, LOW );
    delay( 18 );
    digitalWrite( DHTPIN, HIGH );
    delayMicroseconds( 40 );
    pinMode( DHTPIN, INPUT );

    // Data collection loop
    for ( i = 0; i < MAXTIMINGS; i++ )
    {
        counter = 0;
        while ( digitalRead( DHTPIN ) == laststate )
        {
            counter++;
            delayMicroseconds( 1 );
            if ( counter == 255 ) // Timeout for current state
            {
                break;
            }
        }
        laststate = digitalRead( DHTPIN );

        if ( counter == 255 ) // If timeout occurred, break from the main loop
            break;

        // Store data
        if ( (i >= 4) && (i % 2 == 0) )
        {
            dht11_dat[j / 8] <<= 1;
            if ( counter > 16 ) // This threshold (16) determines if the bit is a '1' or '0'
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }

    // Error checking and data validation
    // If less than 40 bits were received or checksum is incorrect, invalidate data.
    if ( (j < 40) ||
         (dht11_dat[4] != ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
    {
        // Set all data bytes to a known error value -2000
        dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = -2000;
    }
    
}