/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Handling of the node configuration protocol

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis, Gregory Cristian and Matthieu Verdy
*/
#include "sx1280-hal.h"

/*!
 * \brief Helper macro to create Interrupt objects only if the pin name is
 *        different from NC
 */
#define CreateDioPin( pinName, dio )                 \
            if( pinName == NC )                      \
            {                                        \
                dio = NULL;                          \
            }                                        \
            else                                     \
            {                                        \
                dio = new InterruptIn( pinName );    \
            }

/*!
 * \brief Helper macro to avoid duplicating code for setting dio pins parameters
 */
#if defined( TARGET_NUCLEO_L476RG )
#define DioAssignCallback( dio, pinMode, callback )                    \
            if( dio != NULL )                                          \
            {                                                          \
                dio->mode( pinMode );                                  \
                dio->rise( this, static_cast <Trigger>( callback ) );  \
            }
#else
#define DioAssignCallback( dio, pinMode, callback )                    \
            if( dio != NULL )                                          \
            {                                                          \
                dio->rise( this, static_cast <Trigger>( callback ) );  \
            }
#endif
/*!
 * \brief Used to block execution waiting for low state on radio busy pin.
 *        Essentially used in SPI communications
 */
#define WaitOnBusy( )          while( BUSY == 1 ){ }

/*!
 * \brief Blocking routine for waiting the UART to be writeable
 *
 */
#define WaitUartWritable( )  while( RadioUart->writeable( ) == false ){ }

/*!
 * \brief Blocking routine for waiting the UART to be readable
 *
 */
#define WaitUartReadable( )  while( RadioUart->readable( ) == false ){ }

// This code handles cases where assert_param is undefined
#ifndef assert_param
#define assert_param( ... )
#endif

SX1280Hal::SX1280Hal( PinName mosi, PinName miso, PinName sclk, PinName nss,
                      PinName busy, PinName dio1, PinName dio2, PinName dio3, PinName rst,
                      RadioCallbacks_t *callbacks )
        :   SX1280( callbacks ),
            RadioNss( nss ),
            RadioReset( rst ),
            RadioCtsn( NC ),
            BUSY( busy )
{
    CreateDioPin( dio1, DIO1 );
    CreateDioPin( dio2, DIO2 );
    CreateDioPin( dio3, DIO3 );
    RadioSpi = new SPI( mosi, miso, sclk );
    RadioUart = NULL;

    RadioNss = 1;
    RadioReset = 1;
}

SX1280Hal::SX1280Hal( PinName tx, PinName rx, PinName ctsn,
                      PinName busy, PinName dio1, PinName dio2, PinName dio3, PinName rst,
                      RadioCallbacks_t *callbacks )
        :   SX1280( callbacks ),
            RadioNss( NC ),
            RadioReset( rst ),
            RadioCtsn( ctsn ),
            BUSY( busy )
{
    CreateDioPin( dio1, DIO1 );
    CreateDioPin( dio2, DIO2 );
    CreateDioPin( dio3, DIO3 );
    RadioSpi = NULL;
    RadioUart = new Serial( tx, rx );
    RadioCtsn = 0;
    RadioReset = 1;
}

SX1280Hal::~SX1280Hal( void )
{
    if( this->RadioSpi != NULL )
    {
        delete RadioSpi;
    }
    if( this->RadioUart != NULL )
    {
        delete RadioUart;
    }
    if( DIO1 != NULL )
    {
        delete DIO1;
    }
    if( DIO2 != NULL )
    {
        delete DIO2;
    }
    if( DIO3 != NULL )
    {
        delete DIO3;
    }
};

void SX1280Hal::SpiInit( void )
{
    RadioNss = 1;
    RadioSpi->format( 8, 0 );
#if defined( TARGET_KL25Z )
    RadioSpi->frequency( 4000000 );
#elif defined( TARGET_NUCLEO_L476RG )
    RadioSpi->frequency( 8000000 );
#else
    RadioSpi->frequency( 8000000 );
#endif

    wait( 0.1 );
}

void SX1280Hal::UartInit( void )
{
    RadioUart->format( 9, SerialBase::Even, 1 ); // 8 data bits + 1 even parity bit + 1 stop bit
    RadioUart->baud( 115200 );

    // By default the SX1280 UART is setup to handle bytes MSB first.
    // In order to setup the radio to use the UART standard way we first send
    // the equivalent of a WriteRegister with reversed bit order in order to
    // change the endianness.
    uint8_t regVal = 0;
    RadioUart->putc( 0x98 );            // Reversed opcode for read register (0x19)
    RadioUart->putc( 0x10 );            // Reversed MSB register address (0x08)
    RadioUart->putc( 0x18 );            // Reversed LSB register address (0x18)
    RadioUart->putc( 0x80 );            // Reversed value for reading only 1 byte (0x01)
    regVal = RadioUart->getc( )& 0xF3;  // Read reversed value and mask it

    RadioUart->putc( 0x18 );            // Reversed opcode for read register (0x18)
    RadioUart->putc( 0x10 );            // Reversed MSB register address (0x08)
    RadioUart->putc( 0x18 );            // Reversed LSB register address (0x18)
    RadioUart->putc( 0x80 );            // Reversed value for writing only 1 byte (0x01)
    RadioUart->putc( regVal );          // The new value of the register

    // After this point, the UART is running standard mode: 8 data bit, 1 even
    // parity bit, 1 stop bit, 115200 baud, LSB first
    wait_us( 10 );
}

void SX1280Hal::IoIrqInit( DioIrqHandler irqHandler )
{
    assert_param( RadioSpi != NULL || RadioUart != NULL );
    if( RadioSpi != NULL )
    {
        SpiInit( );
    }
    if( RadioUart != NULL )
    {
        UartInit( );
    }

    BUSY.mode( PullNone );

    DioAssignCallback( DIO1, PullNone, irqHandler );
    DioAssignCallback( DIO2, PullNone, irqHandler );
    DioAssignCallback( DIO3, PullNone, irqHandler );
}

void SX1280Hal::Reset( void )
{
    __disable_irq( );
    wait_ms( 20 );
    RadioReset.output( );
    RadioReset = 0;
    wait_ms( 50 );
    RadioReset = 1;
    RadioReset.input( ); // Using the internal pull-up
    wait_ms( 20 );
    __enable_irq( );
}

void SX1280Hal::Wakeup( void )
{
    __disable_irq( );

    //Don't wait for BUSY here

    if( RadioSpi != NULL )
    {
        RadioNss = 0;
        RadioSpi->write( RADIO_GET_STATUS );
        RadioSpi->write( 0 );
        RadioNss = 1;
    }
    if( RadioUart != NULL )
    {
        RadioUart->putc( RADIO_GET_STATUS );
        WaitUartReadable( );
        RadioUart->getc( );
    }

    // Wait for chip to be ready.
    WaitOnBusy( );

    __enable_irq( );
}

void SX1280Hal::WriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    WaitOnBusy( );

    if( RadioSpi != NULL )
    {
        RadioNss = 0;
        RadioSpi->write( ( uint8_t )command );
        for( uint16_t i = 0; i < size; i++ )
        {
            RadioSpi->write( buffer[i] );
        }
        RadioNss = 1;
    }
    if( RadioUart != NULL )
    {
        RadioUart->putc( command );
        if( size > 0 )
        {
            RadioUart->putc( size );
            for( uint16_t i = 0; i < size; i++ )
            {
                RadioUart->putc( buffer[i] );
            }
        }
    }

    if( command != RADIO_SET_SLEEP )
    {
        WaitOnBusy( );
    }
}

void SX1280Hal::ReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    WaitOnBusy( );

    if( RadioSpi != NULL )
    {
        RadioNss = 0;
        if( command == RADIO_GET_STATUS )
        {
            buffer[0] = RadioSpi->write( ( uint8_t )command );
            RadioSpi->write( 0 );
            RadioSpi->write( 0 );
        }
        else
        {
            RadioSpi->write( ( uint8_t )command );
            RadioSpi->write( 0 );
            for( uint16_t i = 0; i < size; i++ )
            {
                 buffer[i] = RadioSpi->write( 0 );
            }
        }
        RadioNss = 1;
    }
    if( RadioUart != NULL )
    {
        RadioUart->putc( command );

        // Behavior on the UART is different depending of the opcode command
        if( ( command == RADIO_GET_PACKETTYPE ) ||
            ( command == RADIO_GET_RXBUFFERSTATUS ) ||
            ( command == RADIO_GET_RSSIINST ) ||
            ( command == RADIO_GET_PACKETSTATUS ) ||
            ( command == RADIO_GET_IRQSTATUS ) )
        {
            /*
             * TODO : Check size size in UART (uint8_t in putc)
             */
            RadioUart->putc( size );
        }

        WaitUartReadable( );
        for( uint16_t i = 0; i < size; i++ )
        {
             buffer[i] = RadioUart->getc( );
        }
    }

    WaitOnBusy( );
}

void SX1280Hal::WriteRegister( uint16_t address, uint8_t *buffer, uint16_t size )
{
    WaitOnBusy( );

    if( RadioSpi != NULL )
    {
        RadioNss = 0;
        RadioSpi->write( RADIO_WRITE_REGISTER );
        RadioSpi->write( ( address & 0xFF00 ) >> 8 );
        RadioSpi->write( address & 0x00FF );
        for( uint16_t i = 0; i < size; i++ )
        {
            RadioSpi->write( buffer[i] );
        }
        RadioNss = 1;
    }
    if( RadioUart != NULL )
    {
        uint16_t addr = address;
        uint16_t i = 0;
        for( addr = address; ( addr + 255 ) < ( address + size ); )
        {
            RadioUart->putc( RADIO_WRITE_REGISTER );
            RadioUart->putc( ( addr & 0xFF00 ) >> 8 );
            RadioUart->putc( addr & 0x00FF );
            RadioUart->putc( 255 );
            for( uint16_t lastAddr = addr + 255 ; addr < lastAddr; i++, addr++ )
            {
                RadioUart->putc( buffer[i] );
            }
        }
        RadioUart->putc( RADIO_WRITE_REGISTER );
        RadioUart->putc( ( addr & 0xFF00 ) >> 8 );
        RadioUart->putc( addr & 0x00FF );
        RadioUart->putc( address + size - addr );

        for( ; addr < ( address + size ); addr++, i++ )
        {
            RadioUart->putc( buffer[i] );
        }
    }

    WaitOnBusy( );
}

void SX1280Hal::WriteRegister( uint16_t address, uint8_t value )
{
    WriteRegister( address, &value, 1 );
}

void SX1280Hal::ReadRegister( uint16_t address, uint8_t *buffer, uint16_t size )
{
    WaitOnBusy( );

    if( RadioSpi != NULL )
    {
        RadioNss = 0;
        RadioSpi->write( RADIO_READ_REGISTER );
        RadioSpi->write( ( address & 0xFF00 ) >> 8 );
        RadioSpi->write( address & 0x00FF );
        RadioSpi->write( 0 );
        for( uint16_t i = 0; i < size; i++ )
        {
            buffer[i] = RadioSpi->write( 0 );
        }
        RadioNss = 1;
    }
    if( RadioUart != NULL )
    {
        uint16_t addr = address;
        uint16_t i = 0;
        for( addr = address; ( addr + 255 ) < ( address + size ); )
        {
            RadioUart->putc( RADIO_READ_REGISTER );
            RadioUart->putc( ( addr & 0xFF00 ) >> 8 );
            RadioUart->putc( addr & 0x00FF );
            RadioUart->putc( 255 );
            WaitUartReadable( );
            for( uint16_t lastAddr = addr + 255 ; addr < lastAddr; i++, addr++ )
            {
                buffer[i] = RadioUart->getc( );
            }
        }
        RadioUart->putc( RADIO_READ_REGISTER );
        RadioUart->putc( ( addr & 0xFF00 ) >> 8 );
        RadioUart->putc( addr & 0x00FF );
        RadioUart->putc( address + size - addr );
        WaitUartReadable( );
        for( ; addr < ( address + size ); addr++, i++ )
        {
            buffer[i] = RadioUart->getc( );
        }
    }

    WaitOnBusy( );
}

uint8_t SX1280Hal::ReadRegister( uint16_t address )
{
    uint8_t data;

    ReadRegister( address, &data, 1 );
    return data;
}

void SX1280Hal::WriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    WaitOnBusy( );

    if( RadioSpi != NULL )
    {
        RadioNss = 0;
        RadioSpi->write( RADIO_WRITE_BUFFER );
        RadioSpi->write( offset );
        for( uint16_t i = 0; i < size; i++ )
        {
            RadioSpi->write( buffer[i] );
        }
        RadioNss = 1;
    }
    if( RadioUart != NULL )
    {
        RadioUart->putc( RADIO_WRITE_BUFFER );
        RadioUart->putc( offset );
        RadioUart->putc( size );
        for( uint16_t i = 0; i < size; i++ )
        {
            RadioUart->putc( buffer[i] );
        }
    }

    WaitOnBusy( );
}

void SX1280Hal::ReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    WaitOnBusy( );

    if( RadioSpi != NULL )
    {
        RadioNss = 0;
        RadioSpi->write( RADIO_READ_BUFFER );
        RadioSpi->write( offset );
        RadioSpi->write( 0 );
        for( uint16_t i = 0; i < size; i++ )
        {
            buffer[i] = RadioSpi->write( 0 );
        }
        RadioNss = 1;
    }
    if( RadioUart != NULL )
    {
        RadioUart->putc( RADIO_READ_BUFFER );
        RadioUart->putc( offset );
        RadioUart->putc( size );
        WaitUartReadable( );
        for( uint16_t i = 0; i < size; i++ )
        {
            buffer[i] = RadioUart->getc( );
        }
    }

    WaitOnBusy( );
}

uint8_t SX1280Hal::GetDioStatus( void )
{
    return ( *DIO3 << 3 ) | ( *DIO2 << 2 ) | ( *DIO1 << 1 ) | ( BUSY << 0 );
}
