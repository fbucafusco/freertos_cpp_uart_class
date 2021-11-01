/*
Copyright 2021, Franco Bucafusco
All rights reserved.

licence: see LICENCE file
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "sapi.h"
#include "semphr.h"


class protocol
{
    public:
        protocol( uartMap_t uart, uint32_t baudRate );

        uint16_t wait_frame( char* data, uint16_t max_size );
        uartMap_t get_uart( void );
        bool receive_started( void );
        bool buffer_overrun( void );
        bool just_started( void );
        void restart_frame();
        void add_to_buffer( char c );
        BaseType_t send_signal_from_isr();

    private:
        /* private objects */
        uartMap_t         uart_used;
        SemaphoreHandle_t new_frame_signal;

        char* buffer;
        uint16_t max_idx;
        uint16_t index;                         //<-- index of the buffer. It points to the next free position
};

