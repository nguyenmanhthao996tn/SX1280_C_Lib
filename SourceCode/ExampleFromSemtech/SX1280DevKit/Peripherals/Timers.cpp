/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Timers

Maintainer: Gregory Cristian & Gilbert Menth
*/

#include "mbed.h"
#include "Timers.h"


Ticker TickTimer;

static uint32_t SoftTimer = 0;
static void TimersIncSoftTimer( void );


void TimersInit( void )
{
    TickTimer.attach_us( &TimersIncSoftTimer, 1000 ); // Ticks every millisecond
}

static void TimersIncSoftTimer( void )
{
    SoftTimer++;
}

void TimersSetTimer( uint32_t *sTimer, uint32_t timeLength )
{
    if( timeLength > MAX_TIMER_VALUE )
    {
        timeLength = MAX_TIMER_VALUE;
    }
    *sTimer = SoftTimer + timeLength;
}

uint32_t TimersTimerHasExpired ( const uint32_t * sTimer )
{
    if( ( SoftTimer - *sTimer ) > 0x7fffffff )
    {
        return false;
    }
    return true;
}

uint32_t TimersTimerValue ( void )
{
    return SoftTimer;
}
