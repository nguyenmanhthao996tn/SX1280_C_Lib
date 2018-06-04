/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: SX9306 Proximity header

Maintainer: Gregory Cristian & Gilbert Menth
*/

#ifndef PROXIMITY_H
#define PROXIMITY_H


#define ANTENNA_1   0
#define ANTENNA_2   1


#define REG_IRQ_SRC      ( 0x00 ) // Interrupt sources
#define REG_STATUS       ( 0x01 ) // Status
#define REG_IRQ_MASK     ( 0x03 ) // Irq mask
#define REG_SENSORSEL    ( 0x20 ) // Select which sensor
#define REG_PROXUSEFUL   ( 0x21 ) // Instantaneous sensor value
#define REG_PROXAVG      ( 0x23 ) // Averaged sensor value
#define REG_CONTROL_0    ( 0x06 ) // Enable and scan period
#define REG_CONTROL_1    ( 0x07 ) //                            
#define REG_CONTROL_2    ( 0x08 ) //                            
#define REG_CONTROL_3    ( 0x09 ) // Doze and filter            
#define REG_CONTROL_4    ( 0x0A ) //                            
#define REG_CONTROL_5    ( 0x0B ) //                            
#define REG_CONTROL_6    ( 0x0C ) //                            
#define REG_CONTROL_7    ( 0x0D ) //                            
#define REG_CONTROL_8    ( 0x0E ) //                            
                                                                
#define SAR_RATIO_THRESH ( 0x10 ) //Just for test               

#define SENSOR_SEL_1       ( 0x02 ) // Select antenna 1
#define SENSOR_SEL_2       ( 0x03 ) // Select antenna 2
#define SENSOR_ENABLE_23   ( 0x0C ) // Enabled sensors 2 & 3 (anteenas 1 & 2)
#define SENSOR_DOZE_OFF    ( 0x00 ) // Prevents doze mode starting
#define PROXIMITY_I2C_ADDR ( 0x54 ) // Proximity IC I2C address
#define MAX_GAIN           ( 0x77 ) // Maximum gain and best granularity

struct ProximityStruct
{
    int16_t Instantaneous;
    int16_t Averaged;
};


 /*!
 * \brief Initialses the hardware and variables associated with the SX9306.
 */
void SX9306ProximityInit( void );

 /*!
 * \brief Called from the main loop in order to deal with SX9306 communications.
 */
void SX9306ProximityHandle( void );

 /*!
 * \brief Generic command used to read and write from the various SX9306
 *        registers. Called from the USB serial port
 *
 * \param [in]  WriteNotRead  If true defines a write operation
 * \param [in]  Address       Addess of the register inside the SX9306 to access
 * \param [in]  WriteValue    Value to be written to the defined register, if a
 *                            write is specified
 * \param [in]  *ReadValue    Pointer to the where a read operation should place
 *                            the data
 *
 * \retval      Status        Non zero = sucess.
 */
uint8_t SX9306proximitySerialCommand( uint8_t writeNotRead, uint8_t address, \
                                      uint8_t writeValue, uint8_t *readValue );

 /*!
 * \brief Generic command used to read and write from the various SX9306 
 *        registers. Called from the USB serial port
 *
 * \param [in]  ThisAntenna   Defines which antenna is to be read (0 or 1)
 *
 * \retval      Value         The value read from the defined antenna.
 */
uint16_t SX9306proximityGetReadValue( uint32_t thisAntenna );

#endif //PROXIMITY_H
