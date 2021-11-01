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

#define PROTOCOL_SOF '>'
#define PROTOCOL_EOF '<'

/**
   @brief RX handler for SAPI UART Driver
   @param not_used
 */
void protocol_rx_event( void *instance_ )
{
    protocol *instance = (protocol *) instance_;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    bool signal_app = false;

    /* read the received byte */
    char c = uartRxRead( instance->get_uart() );

    uint32_t mask = taskENTER_CRITICAL_FROM_ISR();

    /* do something with if the app is waiting for a frame. If not, do not nothing. */
    if( instance->receive_started() )
    {
        if( instance->buffer_overrun()  )
        {
            /* restarts the frame */
            instance->restart_frame();
        }

        if( c == PROTOCOL_SOF )
        {
            if( instance->just_started() )
            {
                /* 1st byte */
            }
            else
            {
                /* restarts the frame (discarding the current progress)*/
                instance->restart_frame();
            }

            instance->add_to_buffer( c );
        }
        else if( c == PROTOCOL_EOF )
        {
            /*  close the frame just if at least one byte was received */
            if( !instance->just_started() )
            {
                /* frame ended - store the byte */
                instance->add_to_buffer( c );

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
            if( !instance->just_started() )
            {
                /* keep the data */
                instance->add_to_buffer( c );
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
        xHigherPriorityTaskWoken = instance->send_signal_from_isr();
    }


    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}


/**
   @brief Initializes the protocol

   @param uart
   @param baudRate
 */
protocol::protocol( uartMap_t uart, uint32_t baudRate )
{
    taskENTER_CRITICAL();

    this->uart_used = uart;

    /* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
    uartConfig( this->uart_used , baudRate );

    /* Habilito todas las interrupciones de UART_USB */
    uartInterrupt( this->uart_used , true );

    this->max_idx = 0;
    this->buffer  = NULL;
    this->index   = 0;
    taskEXIT_CRITICAL();

    this->new_frame_signal = xSemaphoreCreateBinary();
}

/**
   @brief Waits for a new frame to be received. Blocks the caller task.
 */
uint16_t protocol::wait_frame( char* data, uint16_t max_size )
{
    /* configure the private data to allo the RX processs. */
    taskENTER_CRITICAL();
    this->max_idx = max_size;
    this->buffer  = data;
    this->index   = 0;

    /* limpio cualquier interrupcion que hay ocurrido durante el procesamiento */
    uartClearPendingInterrupt( this->uart_used );

    /* habilito isr rx  de UART_USB */
    uartCallbackSet( this->uart_used, UART_RECEIVE, protocol_rx_event , (void*) this );

    taskEXIT_CRITICAL();

    /* waits for the isr signalling */
    xSemaphoreTake( this->new_frame_signal , portMAX_DELAY );

    taskENTER_CRITICAL();
    /* disable the isr until the app releases it again waiting for a new frame */
    uartCallbackClr( this->uart_used, UART_RECEIVE );

    /* prepare the return value */
    uint16_t rv = this->index;

    this->max_idx = 0;
    this->buffer  = NULL;
    this->index   = 0;

    taskEXIT_CRITICAL();

    return rv;
}

/**
   @brief Returns the configured UART

   @return uartMap_t
 */
uartMap_t protocol::get_uart( void )
{
    return this->uart_used;
}

/**
   @brief Returns if a frame has started

   @return true
   @return false
 */
bool  protocol::receive_started( void )
{
    return this->buffer != NULL;
}

/**
   @brief Returns if the reception overrun the given buffer

   @return true
   @return false
 */
bool protocol::buffer_overrun( void )
{
    return this->max_idx - 1 == this->index;
}

/**
   @brief Returns if the reception just started

   @return true
   @return false
 */
bool protocol::just_started( void )
{
    return  this->index  ==0;
}

/**
   @brief Restart the frame

   @return true
   @return false
 */
void protocol::restart_frame()
{
    this->index = 0;
}

/**
   @brief Adds a byte to the buffer

   @param c
 */
void protocol::add_to_buffer( char c )
{
    this->buffer[this->index] = c;
    this->index++;
}

/**
   @brief Sends a signl to the application from RX isr

   @return BaseType_t
 */
BaseType_t protocol::send_signal_from_isr()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR( this->new_frame_signal, &xHigherPriorityTaskWoken );
    return xHigherPriorityTaskWoken;
}