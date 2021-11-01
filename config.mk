# Copyright 2021, Franco Bucafusco
# All rights reserved.

# licence: see LICENCE file


# Compile options
VERBOSE=n
OPT=g
USE_NANO=y
SEMIHOST=n
USE_FPU=y

# Libraries
USE_LPCOPEN=y
USE_SAPI=y
DEFINES+=SAPI_USE_INTERRUPTS

# Use FreeRTOS
USE_FREERTOS=y
FREERTOS_HEAP_TYPE=1

# Tell SAPI to use FreeRTOS SYSTICK
DEFINES+=USE_FREERTOS