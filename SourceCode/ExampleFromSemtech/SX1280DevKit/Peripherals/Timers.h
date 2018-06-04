/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Timers header

Maintainer: Gregory Cristian & Gilbert Menth
*/

#ifndef TIMERS_H
#define TIMERS_H


#define TIM_MSEC        ( uint32_t )1
#define TIM_SEC         ( uint32_t )1000
#define TIM_MIN         ( uint32_t )60000
#define TIM_HOUR        ( uint32_t )3600000
#define MAX_TIMER_VALUE ( TIM_MIN * 150 ) // maximum time for timer


 /*!
 * \brief Initialses the hardware and variables associated with the timers.
 */
void TimersInit( void );

 /*!
 * \brief Sets a timer to a specific value
 *
 * \param [in]  *STimer       Pointer to the timer value to be set.
 * \param [in]  TimeLength    Value to set the timer to in milliseconds.
 */
void TimersSetTimer( uint32_t *sTimer, uint32_t timeLength );

 /*!
 * \brief Checks if a timer has expired.
 *
 * \param [in]  *STimer       Pointer to the timer value to be read.
 *
 * \retval      Status        Non zero if the timer has not expired and is still
 *                            running.
 */
uint32_t TimersTimerHasExpired ( const uint32_t * sTimer );

 /*!
 * \brief Returns the value of the current time in milliseconds
 *
 * \param [in]  refresh       Flag indicates refresh display required (touch)
 *
 * \retval      Value         value of current time in milliseconds
 */
uint32_t TimersTimerValue ( void );

#endif //TIMERS_H
