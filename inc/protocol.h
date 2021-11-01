/*
Copyright 2021, Franco Bucafusco
All rights reserved.

licence: see LICENCE file
*/

#ifndef PROTOCOL_H_
#define PROTOCOL_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "sapi.h"

void procotol_x_init( uartMap_t uart, uint32_t baudRate );
uint16_t protocol_wait_frame( char* data, uint16_t max_size );


#ifdef __cplusplus
}
#endif

#endif