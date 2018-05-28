#include "Sx1280Hal.h"

/*!
 * \brief Used to block execution waiting for low state on radio busy pin.
 *        Essentially used in SPI communications
 */
#define WaitOnBusy(busyPin)              \
    while (digitalRead(busyPin) == HIGH) \
    {                                    \
    }

/*!
 * \brief Helper macro to avoid duplicating code for setting dio pins parameters
 */
#define DioInit(pin)             \
    {                            \
        if (pin != NC)           \
        {                        \
            pinMode(pin, INPUT); \
        }                        \
    }

SX1280Hal::SX1280Hal(int nss,
                     int busy, int dio1, int dio2, int dio3, int rst,
                     RadioCallbacks_t *callbacks) : SX1280(callbacks)
{
    // DIO
    this->DIO1 = dio1;
    DioInit(dio1);

    this->DIO2 = dio2;
    DioInit(dio2);

    this->DIO3 = dio3;
    DioInit(dio3);

    // SPI
    pinMode(nss, OUTPUT);
    // SPI.begin();

    // SS
    this->RadioNss = nss;
    pinMode(nss, OUTPUT);
    digitalWrite(nss, HIGH);

    // RESET
    this->RadioReset = rst;
    pinMode(rst, OUTPUT);
    digitalWrite(rst, HIGH);

    // BUSY
    this->BUSY = busy;
    pinMode(busy, INPUT);
}

SX1280Hal::~SX1280Hal(void)
{
}

void SX1280Hal::SpiInit(void)
{
    digitalWrite(RadioNss, HIGH);
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    delay(100);
}

void SX1280Hal::IoIrqInit(DioIrqHandler irqHandler)
{
    // Call SX1280Hall::OnDioIrq on DioInterrupt
}

void SX1280Hal::Reset(void)
{
    // __disable_irq( );
    delay(20);
    digitalWrite(RadioReset, LOW);
    delay(50);
    digitalWrite(RadioReset, HIGH);
    delay(20);
    // __enable_irq( );
}

void SX1280Hal::Wakeup(void)
{
    // __disable_irq( );

    //Don't wait for BUSY here

    digitalWrite(RadioNss, LOW);    // RadioNss = 0;
    SPI.transfer(RADIO_GET_STATUS); // RadioSpi->write(RADIO_GET_STATUS);
    SPI.transfer(0);                // RadioSpi->write(0);
    digitalWrite(RadioNss, HIGH);   // RadioNss = 1;

    // Wait for chip to be ready.
    WaitOnBusy(BUSY);

    // __enable_irq( );
}

void SX1280Hal::WriteCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{
    WaitOnBusy(BUSY);

    digitalWrite(RadioNss, LOW);    // RadioNss = 0;
    SPI.transfer((uint8_t)command); // RadioSpi->write((uint8_t)command);
    for (uint16_t i = 0; i < size; i++)
    {
        SPI.transfer(buffer[i]); // RadioSpi->write(buffer[i]);
    }
    digitalWrite(RadioNss, HIGH); // RadioNss = 1;

    if (command != RADIO_SET_SLEEP)
    {
        WaitOnBusy(BUSY);
    }
}

void SX1280Hal::ReadCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{
    WaitOnBusy(BUSY);

    digitalWrite(RadioNss, LOW); // RadioNss = 0;
    if (command == RADIO_GET_STATUS)
    {
        buffer[0] = SPI.transfer((uint8_t)command); // buffer[0] = RadioSpi->write((uint8_t)command);
        SPI.transfer(0);                            // RadioSpi->write(0);
        SPI.transfer(0);                            // RadioSpi->write(0);
    }
    else
    {
        SPI.transfer((uint8_t)command); // RadioSpi->write((uint8_t)command);
        SPI.transfer(0);                // RadioSpi->write(0);
        for (uint16_t i = 0; i < size; i++)
        {
            buffer[i] = SPI.transfer(0); // buffer[i] = RadioSpi->write(0);
        }
    }
    digitalWrite(RadioNss, HIGH); // RadioNss = 1;

    WaitOnBusy(BUSY);
}

void SX1280Hal::WriteRegister(uint16_t address, uint8_t *buffer, uint16_t size)
{
    WaitOnBusy(BUSY);

    digitalWrite(RadioNss, LOW);           // RadioNss = 0;
    SPI.transfer(RADIO_WRITE_REGISTER);    // RadioSpi->write(RADIO_WRITE_REGISTER);
    SPI.transfer((address & 0xFF00) >> 8); // RadioSpi->write((address & 0xFF00) >> 8);
    SPI.transfer(address & 0x00FF);        // RadioSpi->write(address & 0x00FF);
    for (uint16_t i = 0; i < size; i++)
    {
        SPI.transfer(buffer[i]); // RadioSpi->write(buffer[i]);
    }
    digitalWrite(RadioNss, HIGH); // RadioNss = 1;

    WaitOnBusy(BUSY);
}

void SX1280Hal::WriteRegister(uint16_t address, uint8_t value)
{
    WriteRegister(address, &value, 1);
}

void SX1280Hal::ReadRegister(uint16_t address, uint8_t *buffer, uint16_t size)
{
    WaitOnBusy(BUSY);

    digitalWrite(RadioNss, LOW);           // RadioNss = 0;
    SPI.transfer(RADIO_READ_REGISTER);     // RadioSpi->write(RADIO_READ_REGISTER);
    SPI.transfer((address & 0xFF00) >> 8); // RadioSpi->write((address & 0xFF00) >> 8);
    SPI.transfer(address & 0x00FF);        // RadioSpi->write(address & 0x00FF);
    SPI.transfer(0);                       // RadioSpi->write(0);
    for (uint16_t i = 0; i < size; i++)
    {
        buffer[i] = SPI.transfer(0); // buffer[i] = RadioSpi->write(0);
    }
    digitalWrite(RadioNss, HIGH); // RadioNss = 1;

    WaitOnBusy(BUSY);
}

uint8_t SX1280Hal::ReadRegister(uint16_t address)
{
    uint8_t data;

    ReadRegister(address, &data, 1);
    return data;
}

void SX1280Hal::WriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
    WaitOnBusy(BUSY);

    digitalWrite(RadioNss, LOW);      // RadioNss = 0;
    SPI.transfer(RADIO_WRITE_BUFFER); // RadioSpi->write(RADIO_WRITE_BUFFER);
    SPI.transfer(offset);             // RadioSpi->write(offset);
    for (uint16_t i = 0; i < size; i++)
    {
        SPI.transfer(buffer[i]); // RadioSpi->write(buffer[i]);
    }
    digitalWrite(RadioNss, HIGH); // RadioNss = 1;

    WaitOnBusy(BUSY);
}

void SX1280Hal::ReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
    WaitOnBusy(BUSY);

    digitalWrite(RadioNss, LOW);     // RadioNss = 0;
    SPI.transfer(RADIO_READ_BUFFER); // RadioSpi->write(RADIO_READ_BUFFER);
    SPI.transfer(offset);            // RadioSpi->write(offset);
    SPI.transfer(0);                 // RadioSpi->write(0);
    for (uint16_t i = 0; i < size; i++)
    {
        buffer[i] = SPI.transfer(0); // buffer[i] = RadioSpi->write(0);
    }
    digitalWrite(RadioNss, HIGH); // RadioNss = 1;

    WaitOnBusy(BUSY);
}

uint8_t SX1280Hal::GetDioStatus(void)
{
    uint8_t result = 0;
    result = (digitalRead(DIO3) << 3) | (digitalRead(DIO2) << 2) | (digitalRead(DIO1) << 1) | (digitalRead(BUSY) << 0);
    return result;
}
