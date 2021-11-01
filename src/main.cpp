/*
Copyright 2021, Franco Bucafusco
All rights reserved.

licence: see LICENCE file
*/

#include <stdbool.h>
#include "protocol.h"

#define FRAME_MAX_SIZE  200 // max size of a frame


/**
   @brief Application code entry point.
 */
void app( void* pvParameters )
{
    protocol p( UART_USB , 115200 );

    char data[FRAME_MAX_SIZE+1];    //<-- storage for the received data (+1 for null termination)
    uint16_t size;                  //<-- size of the received data

    uint16_t frame_counter = 0;

    while( true )
    {
        size = p.wait_frame( data , FRAME_MAX_SIZE );

        /* just to convert to NULL terminated for the printf */
        data[size] = '\0';
        printf( "Received frame (%3u): %s\n", frame_counter, data );

        /* blick just to show to user that a frame was received */
        gpioToggle( LEDB );
        vTaskDelay( 100/portTICK_RATE_MS );
        gpioToggle( LEDB );

        frame_counter++;
    }
}


int main( void )
{
    /* board initialization */
    boardConfig();

    /* create the task that acts as an applicatio layer */
    xTaskCreate(
        app,
        ( const char * ) "app",
        configMINIMAL_STACK_SIZE*2,
        0,
        tskIDLE_PRIORITY+1,
        0
    );

    /* starts the scheduler */
    vTaskStartScheduler();

    return 0;
}
