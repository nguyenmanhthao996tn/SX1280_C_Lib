/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Main program

Maintainer: Gregory Cristian & Gilbert Menth
*/

#include "mbed.h"
#include "Timers.h"
#include "Menu.h"
#include "Eeprom.h"
#include "GpsMax7.h"
#include "SX9306.h"
#include "sx1280-hal.h"
#include "main.h"


/*!
 * \brief Define IO for Unused Pin
 */
DigitalOut F_CS( D6 );      // MBED description of pin
DigitalOut SD_CS( D8 );     // MBED description of pin
DigitalIn userButton( USER_BUTTON );


/*!
 * \brief Specify serial datarate for UART debug output
 */
void baud( int baudrate )
{
    Serial s( USBTX, USBRX );
    s.baud( baudrate );
}

extern SX1280Hal Radio;

int main( )
{
    uint8_t currentPage = START_PAGE;
    uint8_t demoStatusUpdate = 0;   // used for screen display status

    baud( 115200 );

    F_CS = 1;
    SD_CS = 1;

    printf( "Starting SX1280DevKit : %s\n\r", FIRMWARE_VERSION );

    EepromInit( );

    if( userButton == 0 )
    {
        FactoryReset( );
    }

    InitDemoApplication( );
    MenuInit( );
    TimersInit( );
    Max7GpsInit( );
    SX9306ProximityInit( );

    printf( "Radio version: 0x%x\n\r", Radio.GetFirmwareVersion( ) );

    while( 1 )
    {
        currentPage = MenuHandler( demoStatusUpdate );

        switch( currentPage )
        {
            case START_PAGE:
                break;

            case PAGE_PING_PONG:
                demoStatusUpdate = RunDemoApplicationPingPong( );
                break;

            case PAGE_PER:
                demoStatusUpdate = RunDemoApplicationPer( );
                break;

            case PAGE_RANGING_MASTER:
            case PAGE_RANGING_SLAVE:
                demoStatusUpdate = RunDemoApplicationRanging( );
                break;

            case PAGE_SLEEP_MODE:
                demoStatusUpdate = RunDemoSleepMode( );
                break;

            case PAGE_STBY_RC_MODE:
                demoStatusUpdate = RunDemoStandbyRcMode( );
                break;

            case PAGE_STBY_XOSC_MODE:
                demoStatusUpdate = RunDemoStandbyXoscMode( );
                break;

            case PAGE_TX_CW:
                demoStatusUpdate = RunDemoTxCw( );
                break;

            case PAGE_CONT_MODULATION:
                demoStatusUpdate = RunDemoTxContinuousModulation( );
                break;

            case PAGE_UTILITIES:
                // Extracts time and position information from the GPS module
                Max7GpsHandle( );
                SX9306ProximityHandle( ); //Deals with the proximity IC readings
                break;

            default:    // Any page not running a demo
                break;
        }
    }
}

void FactoryReset( void )
{
    EepromFactoryReset( );
    HAL_NVIC_SystemReset( );
}
