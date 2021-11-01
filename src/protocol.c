/*
Copyright 2021, Franco Bucafusco
All rights reserved.

licence: see LICENCE file
*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "protocol.h"
#include "semphr.h"
#include "task.h"




/* private objects */
uartMap_t         uart_used;
SemaphoreHandle_t new_frame_signal;


char* buffer;
uint16_t max_idx;
uint16_t index;                         //<-- index of the buffer. It points to the next free position

/**
   @brief RX handler for SAPI UART Driver
   @param not_used
 */
void protocol_rx_event( void *not_used )
{
    ( void* ) not_used;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    bool signal_app = false;

    /* read the received byte */
    char c = uartRxRead( UART_USB );

    uint32_t mask =  taskENTER_CRITICAL_FROM_ISR();

    /* do something with if the app is waiting for a frame. If not, do not nothing. */
    if( buffer != NULL )
    {
        if( max_idx - 1 == index )
        {
            /* restarts the frame */
            index = 0;
        }

        if( c=='>' )
        {
            if( index==0 )
            {
                /* 1st byte */
            }
            else
            {
                /* restarts the frame (discarding the current progress)*/
                index = 0;
            }

            buffer[index] = c;
            index++;
        }
        else if( c=='<' )
        {
            /*  close the frame just if at least one byte was received */
            if( index >=1 )
            {
                /* frame ended - store the byte */
                buffer[index] = c;
                index++;

                /* mark to signal the app */
                signal_app = true;
            }
            else
            {
                /* byte discarded */
            }
        }
        else
        {
            /* close the frame just if at least one byte was received */
            if( index>=1 )
            {
                /* keep the data */
                buffer[index] = c;
                index++;
            }
            else
            {
                /* byte discarded */
            }
        }
    }

    taskEXIT_CRITICAL_FROM_ISR( mask );

    if( signal_app )
    {
        /* signal the app */
        xSemaphoreGiveFromISR( new_frame_signal, &xHigherPriorityTaskWoken );
    }


    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/**
   @brief Inicializa el stack

   @param uart
   @param baudRate
 */
void procotol_x_init( uartMap_t uart, uint32_t baudRate )
{
    taskENTER_CRITICAL();

    uart_used = uart;

    /* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
    uartConfig( uart, baudRate );

    /* Habilito todas las interrupciones de UART_USB */
    uartInterrupt( uart, true );

    max_idx = 0;
    buffer  = NULL;
    index   = 0;
    taskEXIT_CRITICAL();

    new_frame_signal = xSemaphoreCreateBinary();
}

/**
   @brief Waits for a new frame to be received. Blocks the caller task.
 */
uint16_t protocol_wait_frame( char* data, uint16_t max_size )
{
    /* configure the private data to allo the RX processs. */
    taskENTER_CRITICAL();
    max_idx = max_size;
    buffer  = data;
    index   = 0;

    /* limpio cualquier interrupcion que hay ocurrido durante el procesamiento */
    uartClearPendingInterrupt( uart_used );

    /* habilito isr rx  de UART_USB */
    uartCallbackSet( uart_used, UART_RECEIVE, protocol_rx_event, NULL );

    taskEXIT_CRITICAL();

    /* waits for the isr signalling */
    xSemaphoreTake( new_frame_signal, portMAX_DELAY );

    taskENTER_CRITICAL();
    /* disable the isr until the app releases it again waiting for a new frame */
    uartCallbackClr( uart_used, UART_RECEIVE );

    /* prepare the return value */
    uint16_t rv = index;

    max_idx = 0;
    buffer  = NULL;
    index   = 0;

    taskEXIT_CRITICAL();

    return rv;
}

