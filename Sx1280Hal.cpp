#include "Sx1280Hal.h"

#define DioInit(pin) {\
    if (pin != NC) {\
        pinMode(pin, INPUT);\
    }\
}

SX1280Hal::SX1280Hal(int mosi, int miso, int sclk, int nss,
                     int busy, int dio1, int dio2, int dio3, int rst,
                     RadioCallbacks_t *callbacks) : SX1280(callbacks)
{
    // DIO
    DioInit(dio1);
    DioInit(dio2);
    DioInit(dio3);

    // SPI
    pinMode(nss, OUTPUT);
    SPI.begin();

    // SS
    pinMode(nss, OUTPUT);
    digitalWrite(nss, HIGH);

    // RESET
    pinMode(rst, OUTPUT);
    digitalWrite(rst, HIGH);

    // BUSY
    pinMode(busy, INPUT);
}

SX1280Hal::~SX1280Hal(void)
{

}

void SX1280Hal::SpiInit( void )
{

}


void SX1280Hal::IoIrqInit( DioIrqHandler irqHandler )
{}


void SX1280Hal::Reset( void )
{}


void SX1280Hal::Wakeup( void )
{}


void SX1280Hal::WriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{}


void SX1280Hal::ReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{}


void SX1280Hal::WriteRegister( uint16_t address, uint8_t *buffer, uint16_t size )
{}


void SX1280Hal::WriteRegister( uint16_t address, uint8_t value )
{
    WriteRegister( address, &value, 1 );
}

void SX1280Hal::ReadRegister( uint16_t address, uint8_t *buffer, uint16_t size )
{}


uint8_t SX1280Hal::ReadRegister( uint16_t address )
{
    uint8_t data;

    ReadRegister( address, &data, 1 );
    return data;
}


void SX1280Hal::WriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{}


void SX1280Hal::ReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{}


uint8_t SX1280Hal::GetDioStatus( void )
{
    uint8_t result = 0;

    return result;
}
