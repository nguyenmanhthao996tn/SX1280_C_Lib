/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: uBlox MAX7 GPS header

Maintainer: Gregory Cristian & Gilbert Menth
*/

#ifndef MAX7_GPS_H
#define MAX7_GPS_H


/*!
 * \brief GPGGA format structure.
 */
struct GpggaStruct
{
    char Lat[15];
    char Long[15];
    char NumSats[3];
    bool Fixed;
    bool Updated;
};

/*!
 * \brief GPZDA format structure.
 */
struct GpzdaStruct
{
    char Hour[3];
    char Minute[3];
    char Second[3];
    char Day[3];
    char Month[3];
    char Year[5];
    bool Updated;
};

/*!
 * \brief GPS data structure.
 */
struct GpsStruct
{
    GpggaStruct Position;
    GpzdaStruct Time;
};


/*!
 * \brief Initialses the hardware and variables associated with the MAX7.
 */
void Max7GpsInit( void );

/*!
 * \brief Returns the required data from the MAX7.
 *
 * \retval      GpsStruct*    Pointer to the current GPS data.
 */
GpsStruct* Max7GpsgetData( void );

 /*!
 * \brief Called from the main loop in order to deal with the MAX7 communications.
 */
void Max7GpsHandle( void );

#endif //MAX7_GPS_H
