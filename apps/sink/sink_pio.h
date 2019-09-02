/*
Copyright (c) 2004 - 2016 Qualcomm Technologies International, Ltd.
*/

/*!
@file
@ingroup sink_app
@brief
    Part of the ledmanager Module responsible for managing the PIO outputs excluding LEDs

*/

#ifndef SINK_PIO_H
#define SINK_PIO_H

#include "sink_private.h"
#include <pio.h>
#include <pio_common.h>

#define PIO_POWER_ON        (theSink.conf6->PIOIO.pio_outputs.PowerOnPIO)
#define PIO_AMP_MUTE        (theSink.conf6->PIOIO.pio_outputs.AudioMutePIO)
#define PIO_AUDIO_ACTIVE    (theSink.conf6->PIOIO.pio_outputs.AudioActivePIO)

/****************************************************************************
NAME	
	LEDManagerSetPowerOff

DESCRIPTION
    Set / Clear a power pin for the device
    
RETURNS
	void
*/
void PioSetPowerPin ( bool enable ) ;


/****************************************************************************
NAME	
	PioSetPio

DESCRIPTION
    Fn to change a PIO
    Set drive TRUE to drive PIO, FALSE to pull PIO
    Set dir TRUE to set high, FALSE to set low
    
RETURNS
	void
*/
void PioSetPio(uint8 pio , pio_common_dir drive, bool dir);


/****************************************************************************
NAME	
	PioGetPio

DESCRIPTION
    Fn to read a PIO
    
RETURNS
	TRUE if set, FALSE if not
*/
bool PioGetPio(uint8 pio);

/****************************************************************************
NAME	
	PioDrivePio

DESCRIPTION
	Drive a PIO to the passed state if configured. PIOs may be active high or
    active low depending on the pio_invert bitmap.
    
RETURNS
	void
    
*/
void PioDrivePio(uint8 pio, bool state);

/****************************************************************************
NAME	
	PioIsPioConfigured

DESCRIPTION
	Returns whether the passed pio is a valid pio.
    
RETURNS
	TRUE if valid, else FALSE
    
*/
#define PioIsPioConfigured(pio) ((pio == NO_PIO) ? FALSE : TRUE)

#endif
