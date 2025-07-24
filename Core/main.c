/**
 * @file main.c
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wiringSerial.h>
#include <wiringPi.h>

#include "Modules/btn_handler.h"

#define BAUD_RATE 9600

#define BTN_PIN 0


int main( void )
{
    int fd; // File descriptor para el puerto serial
    char serialData[64]; // Buffer para almacenar los datos leídos de la UART
    int dataIndex = 0;   // Índice para el buffer de datos seriales
    

    printf("Iniciando la escucha UART en Raspberry Pi Zero 2 W.\n");
    printf("Esperando datos del ESP32 en el formato '|humedad|temperatura|ADC|'...\n");


    if ( wiringPiSetup() == -1 )
    {
        fprintf(stderr, "Failed to initialize WiringPi. Exiting.\n");
        return 1; // Use return 1 for error exit
    }

    fd = serialOpen(SERIAL_PORT, BAUD_RATE);
    if (fd == -1) {
        // Muestra un error si no se puede abrir el puerto serial
        fprintf(stderr, "Error al abrir el puerto serial %s: %s\n", SERIAL_PORT, strerror(errno));
        return 1; // Sale con error
    }
    printf("Puerto serial %s abierto exitosamente a %d baudios.\n", SERIAL_PORT, BAUD_RATE);

    button_init(BTN_PIN);

    while ( 1 )
    {

        if (button_flag == 1)
        {
            printf("Button Pressed......................................");
            button_flag = 0;
        }
        // 3. Verifica si hay datos disponibles para leer en el buffer de la UART
        if (serialDataAvail(fd)) {
            char byte = serialGetchar(fd); // Lee un solo byte del puerto serial

            // 4. Lógica de acumulación de bytes y detección de fin de cadena
            // Si el byte leído es un retorno de carro o una nueva línea, se considera el final de la cadena
            if (byte == '\n' || byte == '\r') {
                serialData[dataIndex] = '\0'; // Agrega el terminador nulo para que sea una cadena válida en C

                // 5. Imprime la cadena completa sin parsear, solo si se ha recibido algo
                if (dataIndex > 0) {
                    printf("Cadena recibida: '%s'\n", serialData);
                }

                // Reinicia el índice del buffer y limpia el buffer para la siguiente cadena
                dataIndex = 0; 
                memset(serialData, 0, sizeof(serialData)); // Llena el buffer con ceros
            } else {
                // Si no es un retorno de carro/nueva línea, añade el byte al buffer
                // Evita el desbordamiento del buffer
                if (dataIndex < sizeof(serialData) - 1) { 
                    serialData[dataIndex++] = byte;
                } else {
                    // Si el buffer se llena antes de una nueva línea, indica una posible pérdida de datos
                    fprintf(stderr, "Advertencia: Buffer de lectura lleno. Posible pérdida de datos. Reiniciando buffer.\n");
                    dataIndex = 0; // Reinicia el índice
                    memset(serialData, 0, sizeof(serialData)); // Limpia el buffer
                }
            }
        }

        delay(1);
    }
    
    serialClose(fd);
    return 0;
}