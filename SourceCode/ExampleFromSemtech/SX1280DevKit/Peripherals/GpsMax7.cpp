/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: uBlox MAX7 GPS

Maintainer: Gregory Cristian & Gilbert Menth
*/

#include <stdio.h>
#include "mbed.h"
#include "Timers.h"
#include "GpsMax7.h"


#define MAX_NMEA_SENTENCE_LENGTH    100
#define GPS_I2C_ADDR                ( 0x84 ) // GPS IC I2C address
#define NUM_SETUP_COMMANDS          7
#define SETUP_COMMAND_LENGTH        16
#define DATA_STREAM_ADDRESS         0xFF
#define I2C_FREQUENCY               100000  //100 kHz


typedef enum
{
    GPGGA_NMEA_DOLLAR,
    GPGGA_NMEA_G1,
    GPGGA_NMEA_P,
    GPGGA_NMEA_G2,
    GPGGA_NMEA_G3,
    GPGGA_NMEA_A,
    GPGGA_NMEA_CR,
    GPGGA_NMEA_LF
}GpggaNmeaFields_t;

typedef enum
{
    GPZDA_NMEA_DOLLAR,
    GPZDA_NMEA_G1,
    GPZDA_NMEA_P,
    GPZDA_NMEA_G2,
    GPZDA_NMEA_G3,
    GPZDA_NMEA_A,
    GPZDA_NMEA_CR,
    GPZDA_NMEA_LF
}GpzdaNmeaFields_t;

typedef enum
{
    GPS_COMMS_INIT,
    GPS_COMMS_WRITE_SETUP,
    GPS_COMMS_WAIT_WRITE_SETUP,
    GPS_COMMS_READ_DATA_BUFFER,
    GPS_COMMS_WAIT_NEXT_READ
}GpsCommsState_t;

union GpsBufferSize
{
    uint16_t NumBytes;
    char NumBytesBuffer[2];
};


//MAX7 initialisation commands
const char SetupArray[NUM_SETUP_COMMANDS][SETUP_COMMAND_LENGTH] =
{
    { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29 }, // GxGGA on to I2C
    { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x09, 0x62 }, // GxZDA on to I2C
    { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B }, // GxGLL not on the I2C
    { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x40 }, // GxRMC not on the I2C
    { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47 }, // GxVTG not on the I2C
    { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32 }, // GxGSA not on the I2C
    { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39 }  // GxGSV not on the I2C
};

I2C GpsI2C( I2C_SDA, I2C_SCL );

GpsStruct Gps;

static uint8_t  GpggaSentenceBuffer[MAX_NMEA_SENTENCE_LENGTH];
static int      GpggaSentenceBufferPtr = 0;
static uint8_t  GpzdaSentenceBuffer[MAX_NMEA_SENTENCE_LENGTH];
static int      GpzdaSentenceBufferPtr = 0;

static GpggaNmeaFields_t GpggaNmeaField;
static GpzdaNmeaFields_t GpzdaNmeaField;
static GpsCommsState_t   GpsCommsState;
static uint32_t          GpsCommsTimer;


static void GpsCheckCommandBufferForGpgga( uint8_t thisChar );
static void GpsCheckCommandBufferForGpzda( uint8_t thisChar );
static void ParseNmeaGpggaSentence( void );
static void ParseNmeaGpzdaSentence( void );
static bool Max7GpsWriteSetupOK( void );
static bool Max7GpsReadDataBuffer( void );
static uint8_t Max7GpsReadRegister( char thisRegAddress );


void Max7GpsInit( void )
{
    GpsI2C.frequency( I2C_FREQUENCY );
}

void Max7GpsHandle( void )
{
    switch( GpsCommsState )
    {
        case GPS_COMMS_INIT:
            GpsCommsState = GPS_COMMS_WRITE_SETUP;
            break;

        case GPS_COMMS_WRITE_SETUP:
            if( Max7GpsWriteSetupOK( ) )
            {
                GpsCommsState = GPS_COMMS_READ_DATA_BUFFER;
            }
            else
            {
                TimersSetTimer( &GpsCommsTimer, 1 * TIM_SEC );
                GpsCommsState = GPS_COMMS_WAIT_WRITE_SETUP;
            }
            break;

        case GPS_COMMS_WAIT_WRITE_SETUP:
            if( TimersTimerHasExpired( &GpsCommsTimer ) )
            {
                GpsCommsState = GPS_COMMS_WRITE_SETUP;
            }
            break;

        case GPS_COMMS_READ_DATA_BUFFER:
            Max7GpsReadDataBuffer( );
            TimersSetTimer( &GpsCommsTimer, 100 * TIM_MSEC );
            GpsCommsState = GPS_COMMS_WAIT_NEXT_READ;
            break;

        case GPS_COMMS_WAIT_NEXT_READ:
            if( TimersTimerHasExpired( &GpsCommsTimer ) )
            {
                GpsCommsState = GPS_COMMS_READ_DATA_BUFFER;
            }
            break;
    }
}

static bool Max7GpsWriteSetupOK( void )
{
    int lineCount;

    for( lineCount = 0; lineCount < NUM_SETUP_COMMANDS; lineCount++ )
    {
        if( GpsI2C.write( GPS_I2C_ADDR, &SetupArray[lineCount][0], \
                          SETUP_COMMAND_LENGTH, 0 ) )
        {
            return false;
        }
    }
    return true;
}

static bool Max7GpsReadDataBuffer( void )
{
    uint8_t incomingCheck;
    bool contFlag = true;

    while( contFlag )
    {
        incomingCheck = Max7GpsReadRegister( DATA_STREAM_ADDRESS );
        if( incomingCheck == 0xFF )
        {
            contFlag = false;
        }
        else
        {
            GpsCheckCommandBufferForGpgga( incomingCheck );
            GpsCheckCommandBufferForGpzda( incomingCheck );
        }
    }
    return false;
}

static uint8_t Max7GpsReadRegister( char thisRegAddress )
{
    char thisValue;
    uint8_t retVal;

    thisValue = thisRegAddress;
    GpsI2C.write( GPS_I2C_ADDR, &thisValue, 1, 0 );
    GpsI2C.read( GPS_I2C_ADDR, &thisValue, 1, 0 );
    retVal = ( uint8_t )thisValue;
    return retVal;
}

static void GpsCheckCommandBufferForGpgga( uint8_t thisChar )
{
    switch( GpggaNmeaField )
    {
        default:
        case GPGGA_NMEA_DOLLAR:
            if( thisChar == '$' )
            {
                GpggaSentenceBufferPtr = 0;
                GpggaSentenceBuffer[GpggaSentenceBufferPtr++] = thisChar;
                GpggaNmeaField = GPGGA_NMEA_G1;
            }
            break;

        case GPGGA_NMEA_G1:
            if( thisChar == 'G' )
            {
                GpggaSentenceBuffer[GpggaSentenceBufferPtr++] = thisChar;
                GpggaNmeaField = GPGGA_NMEA_P;
            }
            else
            {
                GpggaNmeaField = GPGGA_NMEA_DOLLAR;
            }
            break;

        case GPGGA_NMEA_P:
            if( thisChar == 'P' )
            {
                GpggaSentenceBuffer[GpggaSentenceBufferPtr++] = thisChar;
                GpggaNmeaField = GPGGA_NMEA_G2;
            }
            else
            {
                GpggaNmeaField = GPGGA_NMEA_DOLLAR;
            }
            break;

        case GPGGA_NMEA_G2:
            if( thisChar == 'G' )
            {
                GpggaSentenceBuffer[GpggaSentenceBufferPtr++] = thisChar;
                GpggaNmeaField = GPGGA_NMEA_G3;
            }
            else
            {
                GpggaNmeaField = GPGGA_NMEA_DOLLAR;
            }
            break;

        case GPGGA_NMEA_G3:
            if( thisChar == 'G' )
            {
                GpggaSentenceBuffer[GpggaSentenceBufferPtr++] = thisChar;
                GpggaNmeaField = GPGGA_NMEA_A;
            }
            else
            {
                GpggaNmeaField = GPGGA_NMEA_DOLLAR;
            }
            break;

        case GPGGA_NMEA_A:
            if( thisChar == 'A' )
            {
                GpggaSentenceBuffer[GpggaSentenceBufferPtr++] = thisChar;
                GpggaNmeaField = GPGGA_NMEA_CR;
            }
            else
            {
                GpggaNmeaField = GPGGA_NMEA_DOLLAR;
            }
            break;

        case GPGGA_NMEA_CR:
            GpggaSentenceBuffer[GpggaSentenceBufferPtr++] = thisChar;
            if( GpggaSentenceBufferPtr >= MAX_NMEA_SENTENCE_LENGTH )
            {
                GpggaNmeaField = GPGGA_NMEA_DOLLAR;
            }
            if( thisChar == 0x0A )
            {
                ParseNmeaGpggaSentence( );
            }
            break;

        case GPGGA_NMEA_LF:
            break;
    }
}

static void GpsCheckCommandBufferForGpzda( uint8_t thisChar )
{
    switch( GpzdaNmeaField )
    {
        default:
        case GPZDA_NMEA_DOLLAR:
            if( thisChar == '$' )
            {
                GpzdaSentenceBufferPtr = 0;
                GpzdaSentenceBuffer[GpzdaSentenceBufferPtr++] = thisChar;
                GpzdaNmeaField = GPZDA_NMEA_G1;
            }
            break;

        case GPZDA_NMEA_G1:
            if( thisChar == 'G' )
            {
                GpzdaSentenceBuffer[GpzdaSentenceBufferPtr++] = thisChar;
                GpzdaNmeaField = GPZDA_NMEA_P;
            }
            else
            {
                GpzdaNmeaField = GPZDA_NMEA_DOLLAR;
            }
            break;

        case GPZDA_NMEA_P:
            if( thisChar == 'P' )
            {
                GpzdaSentenceBuffer[GpzdaSentenceBufferPtr++] = thisChar;
                GpzdaNmeaField = GPZDA_NMEA_G2;
            }
            else
            {
                GpzdaNmeaField = GPZDA_NMEA_DOLLAR;
            }
            break;

        case GPZDA_NMEA_G2:
            if( thisChar == 'Z' )
            {
                GpzdaSentenceBuffer[GpzdaSentenceBufferPtr++] = thisChar;
                GpzdaNmeaField = GPZDA_NMEA_G3;
            }
            else
            {
                GpzdaNmeaField = GPZDA_NMEA_DOLLAR;
            }
            break;

        case GPZDA_NMEA_G3:
            if( thisChar == 'D' )
            {
                GpzdaSentenceBuffer[GpzdaSentenceBufferPtr++] = thisChar;
                GpzdaNmeaField = GPZDA_NMEA_A;
            }
            else
            {
                GpzdaNmeaField = GPZDA_NMEA_DOLLAR;
            }
            break;

        case GPZDA_NMEA_A:
            if( thisChar == 'A' )
            {
                GpzdaSentenceBuffer[GpzdaSentenceBufferPtr++] = thisChar;
                GpzdaNmeaField = GPZDA_NMEA_CR;
            }
            else
            {
                GpzdaNmeaField = GPZDA_NMEA_DOLLAR;
            }
            break;

        case GPZDA_NMEA_CR:
            GpzdaSentenceBuffer[GpzdaSentenceBufferPtr++] = thisChar;
            if( GpzdaSentenceBufferPtr >= MAX_NMEA_SENTENCE_LENGTH )
            {
                GpzdaNmeaField = GPZDA_NMEA_DOLLAR;
            }
            if( thisChar == 0x0A )
            {
                ParseNmeaGpzdaSentence( );
            }
            break;

        case GPZDA_NMEA_LF:
            break;
    }
}

static void ParseNmeaGpggaSentence( void )
{
    GpggaStruct thisFix;
    int sentencePtr = 1;
    int subStrPtr = 0;
    int commaCount = 0;
    uint8_t checkSum = 0;
    uint8_t thisChar;
    bool contFlag = true;
    char tArray[3];
    char compArray[3];
    
    while( contFlag ) //Get the checksum
    {
        thisChar = GpggaSentenceBuffer[sentencePtr];
        if( thisChar == '*' )
        {
            contFlag = false;
        }
        else
        {
            checkSum ^= thisChar;
        }
        sentencePtr++;
        if( sentencePtr >= MAX_NMEA_SENTENCE_LENGTH )
        {
            thisFix.Fixed = false;
            return;
        }
    }
    compArray[0] = GpggaSentenceBuffer[sentencePtr++];
    compArray[1] = GpggaSentenceBuffer[sentencePtr];
    compArray[2] = 0x00;
    sentencePtr = 0;
    sprintf( tArray, "%02X", checkSum );
    if( strcmp( tArray, compArray ) != 0 ) //Fails checksum
    {
        thisFix.Fixed = false;
        return;
    }
    while( commaCount < 6 ) //Find fix quality
    {
        if( GpggaSentenceBuffer[sentencePtr++] == ',' )
        {
            commaCount++;
        }
        if( sentencePtr >= MAX_NMEA_SENTENCE_LENGTH )
        {
            thisFix.Fixed = false;
            return;
        }
    }
    switch( GpggaSentenceBuffer[sentencePtr++] )
    {
        case '1':
        case '2':
        case '3':
        case '4':
            thisFix.Fixed = true;
            break;

        default:
            thisFix.Fixed = false;
            break;
    }
    sentencePtr++; //Skip comma after fix
    thisFix.NumSats[subStrPtr++] = GpggaSentenceBuffer[sentencePtr++];
    thisFix.NumSats[subStrPtr++] = GpggaSentenceBuffer[sentencePtr++];
    thisFix.NumSats[subStrPtr] = 0;
    if( thisFix.Fixed )
    {
        sentencePtr = 0;
        commaCount = 0;
        while( commaCount < 1 ) //Find fix time
        {
            if( GpggaSentenceBuffer[sentencePtr++] == ',' )
            {
                commaCount++;
            }
            if( sentencePtr >= MAX_NMEA_SENTENCE_LENGTH )
            {
                thisFix.Fixed = false;
                return;
            }
        }
        subStrPtr = 0;
        // Skip over time field as this can be picked up from the other sentence
        sentencePtr += 6;
        while( commaCount < 2 ) //Find Latitude
        {
            if( GpzdaSentenceBuffer[sentencePtr++] == ',' )
            {
                commaCount++;
            }
            if( sentencePtr >= MAX_NMEA_SENTENCE_LENGTH )
            {
                thisFix.Fixed = false;
                return;
            }
        }
        subStrPtr = 0;
        for( commaCount = 0; commaCount < 10; commaCount++ )
        {
            thisFix.Lat[subStrPtr++] = GpggaSentenceBuffer[sentencePtr++];
        }
        sentencePtr++; // Skip next comma
        thisFix.Lat[subStrPtr++] = ' '; //Add a space
        thisFix.Lat[subStrPtr++] = GpggaSentenceBuffer[sentencePtr++]; //N or S
        thisFix.Lat[subStrPtr] = 0x00; //String terminate
        
        while( commaCount < 4 ) //Find Longitude
        {
            if( GpggaSentenceBuffer[sentencePtr++] == ',' )
            {
                commaCount++;
            }
            if( sentencePtr >= MAX_NMEA_SENTENCE_LENGTH )
            {
                thisFix.Fixed = false;
                return;
            }
        }
        sentencePtr++; // Skip this comma
        subStrPtr = 0;
        for( commaCount = 0; commaCount < 11; commaCount++ )
        {
            thisFix.Long[subStrPtr++] = GpggaSentenceBuffer[sentencePtr++];
        }
        sentencePtr++; // Skip next comma
        thisFix.Long[subStrPtr++] = ' '; //Add a space
        thisFix.Long[subStrPtr++] = GpggaSentenceBuffer[sentencePtr++]; //E or W
        thisFix.Long[subStrPtr] = 0x00; //String terminate
    }
    thisFix.Updated = true;
    Gps.Position = thisFix;
}

static void ParseNmeaGpzdaSentence( void )
{
    GpzdaStruct thisTime;
    int sentencePtr = 1;
    int commaCount = 0;
    uint8_t checkSum = 0;
    uint8_t thisChar;
    bool contFlag = true;
    char tArray[3];
    char compArray[3];
    
    while( contFlag ) //Get the checksum
    {
        thisChar = GpzdaSentenceBuffer[sentencePtr];
        if( thisChar == '*' )
        {
            contFlag = false;
        }
        else
        {
            checkSum ^= thisChar;
        }
        sentencePtr++;
        if( sentencePtr >= MAX_NMEA_SENTENCE_LENGTH )
        {
            return;
        }
    }
    compArray[0] = GpzdaSentenceBuffer[sentencePtr++];
    compArray[1] = GpzdaSentenceBuffer[sentencePtr];
    compArray[2] = 0x00;
    sentencePtr = 0;
    sprintf( tArray, "%02X", checkSum );
    if( strcmp( tArray, compArray ) != 0 ) //Fails checksum
    {
        return;
    }
    while( commaCount < 1 ) //Start with hours (first field)
    {
        if( GpzdaSentenceBuffer[sentencePtr++] == ',' )
        {
            commaCount++;
        }
        if( sentencePtr >= MAX_NMEA_SENTENCE_LENGTH )
        {
            return;
        }
    }
    thisTime.Hour[0] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Hour[1] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Hour[2] = 0x00;
    
    thisTime.Minute[0] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Minute[1] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Minute[2] = 0x00;
    
    thisTime.Second[0] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Second[1] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Second[2] = 0x00;
    sentencePtr += 4;

    thisTime.Day[0] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Day[1] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Day[2] = 0x00; 
    sentencePtr += 1;

    thisTime.Month[0] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Month[1] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Month[2] = 0x00; 
    sentencePtr += 1;
    
    thisTime.Year[0] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Year[1] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Year[2] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Year[3] = GpzdaSentenceBuffer[sentencePtr++];
    thisTime.Year[4] = 0x00;
    thisTime.Updated = true;
    Gps.Time = thisTime;
}

GpsStruct* Max7GpsgetData( void )
{
    return &Gps ;
}
