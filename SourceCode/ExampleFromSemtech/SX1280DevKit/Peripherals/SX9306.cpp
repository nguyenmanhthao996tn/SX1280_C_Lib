/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: SX9306 Proximity sensor

Maintainer: Gregory Cristian & Gilbert Menth
*/

#include "mbed.h"
#include <stdio.h>
#include "DmTftBase.h"
#include "SX9306.h"
#include "Timers.h"
#include "Eeprom.h"

union ProximityData_t
{
    ProximityStruct ThisAntenna;
    char Buffer[sizeof( ProximityStruct )];
};

typedef enum
{
    PROXIMITY_STATE_START_RESET,
    PROXIMITY_STATE_WAIT_RESET,
    PROXIMITY_STATE_INIT,
    PROXIMITY_STATE_READ,
    PROXIMITY_STATE_WAIT
}ProximityAppState_t;


DigitalOut ProximityIcReset( PA_1 );
I2C ProxI2C( I2C_SDA, I2C_SCL );

static uint32_t ProximityTimer;
static char ReadDataBuffer[2];
static char WriteDataBuffer[2];
static ProximityAppState_t ProximityAppState;
static ProximityData_t ProximityReading;
static ProximityStruct Antenna[2];


static bool SX9306ProximityReadRegister( char thisRegAddress, char *value );
static bool SX9306ProximityWriteRegister( char thisRegAddress, char value );
static bool SX9306ProximityReadAntennaValues( void );


void SX9306ProximityInit( void )
{
    int count;
    
    ProximityAppState = PROXIMITY_STATE_START_RESET;
    for( count = 0; count < 2; count++ )
    {
        Antenna[count].Averaged = 0;
        Antenna[count].Instantaneous = 0;
    }
}

void SX9306ProximityHandle( void )
{
    bool i2cResult;
    
    switch( ProximityAppState )
    {
        case PROXIMITY_STATE_START_RESET:
            ProximityIcReset = false;
            TimersSetTimer( &ProximityTimer, 10 * TIM_MSEC );
            ProximityAppState = PROXIMITY_STATE_WAIT_RESET;
            break;

        case PROXIMITY_STATE_WAIT_RESET:
            if( TimersTimerHasExpired( &ProximityTimer ) )
            {
                ProximityIcReset = true;
                TimersSetTimer( &ProximityTimer, 10 * TIM_MSEC );
                ProximityAppState = PROXIMITY_STATE_INIT;
            }
            break;

        case PROXIMITY_STATE_INIT:
            if( TimersTimerHasExpired( &ProximityTimer ) )
            {
                TimersSetTimer( &ProximityTimer, 1 * TIM_SEC );
                //Clear interrupts after power on
                i2cResult = SX9306ProximityReadRegister( REG_IRQ_SRC, \
                                                         &ReadDataBuffer[0] );
                if( !i2cResult )
                {
                    //Enable antennas 1 & 2
                    i2cResult = SX9306ProximityWriteRegister( REG_CONTROL_0, \
                                                              SENSOR_ENABLE_23 );
                    if( !i2cResult )
                    {
                        //Stop doze mode
                        i2cResult = SX9306ProximityWriteRegister( REG_CONTROL_3, \
                                                                  SENSOR_DOZE_OFF );
                        if( !i2cResult )
                        {
                            //Set max gain and granularity
                            i2cResult = SX9306ProximityWriteRegister( REG_CONTROL_2, \
                                                                      MAX_GAIN );
                        }
                    }
                }
                if( !i2cResult )
                {
                    ProximityAppState = PROXIMITY_STATE_READ;
                }
                else
                {
                    ProximityAppState = PROXIMITY_STATE_START_RESET;
                }
            }
            break;

        case PROXIMITY_STATE_READ:
            TimersSetTimer( &ProximityTimer, 1 * TIM_SEC );
            //Select antenna 1
            i2cResult = SX9306ProximityWriteRegister( REG_SENSORSEL, SENSOR_SEL_1 );
            if( i2cResult )
            {
                ProximityAppState = PROXIMITY_STATE_START_RESET;
                break;
            }
            i2cResult = SX9306ProximityReadAntennaValues( ); //Read values
            if( i2cResult )
            {
                ProximityAppState = PROXIMITY_STATE_START_RESET;
                break;
            }
            Antenna[ANTENNA_1] = ProximityReading.ThisAntenna;
            //Select antenna 2
            i2cResult = SX9306ProximityWriteRegister( REG_SENSORSEL, SENSOR_SEL_2 );
            if( i2cResult )
            {
                ProximityAppState = PROXIMITY_STATE_START_RESET;
                break;
            }
            i2cResult = SX9306ProximityReadAntennaValues( ); //Read values
            if( i2cResult )
            {
                ProximityAppState = PROXIMITY_STATE_START_RESET;
                break;
            }
            Antenna[ANTENNA_2] = ProximityReading.ThisAntenna;
            ProximityAppState = PROXIMITY_STATE_WAIT;
            break;

        case PROXIMITY_STATE_WAIT:
            if( TimersTimerHasExpired( &ProximityTimer ) )
            {
                ProximityAppState = PROXIMITY_STATE_READ;
            }
            break;
    }
}

static bool SX9306ProximityReadRegister( char thisRegAddress, char *value )
{
    WriteDataBuffer[0] = thisRegAddress;
    // Write the register address
    if( ProxI2C.write( PROXIMITY_I2C_ADDR, &WriteDataBuffer[0], 1, 0 ) )
    {
        return true;
    }
    // Read data from the address
    if( ProxI2C.read( PROXIMITY_I2C_ADDR, value, 1, 0 ) )
    {
        return true;
    }
    return false;
}

static bool SX9306ProximityWriteRegister( char thisRegAddress, char value )
{
    WriteDataBuffer[0] = thisRegAddress;
    WriteDataBuffer[1] = value;
    
    // Write the register address and data byte
    if( ProxI2C.write( PROXIMITY_I2C_ADDR, &WriteDataBuffer[0], 2, 0 ) )
    {
        return true;
    }
    return false;
}

static bool SX9306ProximityReadAntennaValues( void )
{
    bool i2cResult;
    
    i2cResult = SX9306ProximityReadRegister( REG_PROXUSEFUL, \
                                             &ProximityReading.Buffer[1] );
    if( i2cResult )
    {
        return true;
    }
    i2cResult = SX9306ProximityReadRegister( REG_PROXUSEFUL + 1, \
                                             &ProximityReading.Buffer[0] );
    if( i2cResult )
    {
        return true;
    }
    i2cResult = SX9306ProximityReadRegister( REG_PROXAVG, \
                                             &ProximityReading.Buffer[3] );
    if( i2cResult )
    {
        return true;
    }
    i2cResult = SX9306ProximityReadRegister( REG_PROXAVG + 1, \
                                             &ProximityReading.Buffer[2] );
    if( i2cResult )
    {
        return true;
    }
    return false;
}

uint8_t SX9306proximitySerialCommand( uint8_t writeNotRead, uint8_t address, \
                                      uint8_t writeValue, uint8_t *readValue )
{
    char valueRead = 0;
    bool i2cResult;
    
    if( writeNotRead )
    {
        i2cResult = SX9306ProximityWriteRegister( address, writeValue );
    }
    else
    {
        i2cResult = SX9306ProximityReadRegister( address, &valueRead );
        *readValue = ( uint8_t )valueRead;
    }
    if( i2cResult )
    {
        return 0;
    }
    return 1;
}

uint16_t SX9306proximityGetReadValue( uint32_t thisAntenna )
{
    uint16_t retVal;
    
    switch( thisAntenna )
    {
        case 0:
        case 1:
            retVal = Antenna[thisAntenna].Instantaneous;
            break;

        default:
            retVal = Antenna[0].Instantaneous;
            break;
    }
    return retVal;
}
