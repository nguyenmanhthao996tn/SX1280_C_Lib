/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Display demo menus and manage touch sensor, header

Maintainer: Gregory Cristian & Gilbert Menth
*/

#ifndef MENU_H
#define MENU_H


#include "DisplayDriver.h"
#include "DmTouchCalibration.h"


/*!
 * \brief TFT calibration structure.
 */
typedef struct
{
    bool ScreenCalibrated;
    CalibrationMatrix Calibration;
}MenuSettings_t;

/*!
 * \brief Available pages in menu.
 */
enum MenuPages
{
    START_PAGE = 0,
    PAGE_PING_PONG,
    PAGE_PER,
    PAGE_RANGING_MASTER,
    PAGE_RANGING_SLAVE,
    PAGE_RANGING_PARAM,
    PAGE_RADIO_PARAM,
    PAGE_RADIO_PARAM_FREQ,
    PAGE_UTILITIES,
    PAGE_RADIO_TEST,
    PAGE_SLEEP_MODE,
    PAGE_STBY_RC_MODE,
    PAGE_STBY_XOSC_MODE,
    PAGE_TX_CW,
    PAGE_CONT_MODULATION
};


/*!
 * \brief Common var to manage TFT calibration.
 */
extern MenuSettings_t MenuSettings;


/*!
 * \brief Init menu settings.
 */
void MenuInit( void );

/*!
 * \brief Permanently display Handler for touch sensor.
 *
 * \param [in]  refresh       Flag indicates refresh display required (touch)
 *
 * \retval      CurrentPage   The updated current page
 */
uint8_t MenuHandler( bool refresh );

/*!
 * \brief Update or refresh the activated menu page.
 *
 * \param [in]  page          Page to display
 */
void MenuSetPage( uint8_t page );

/*!
 * \brief Writes 3 lines on display, with current radio parameters.
 *
 * \param [in]  page          Current page to choose what to display.
 */
void DisplayCurrentRadioParams( uint8_t page );

/*!
 * \brief Return text with current frame type.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuRadioFrameType( void );

/*!
 * \brief Return text with current Radio Modulation Parameters1.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioModulationParameters1( void );

/*!
 * \brief Return text with current Radio Modulatio nParameters2.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioModulationParameters2( void );

/*!
 * \brief Return text with current Radio Modulation Parameters3.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioModulationParameters3( void );

/*!
 * \brief Return text with current Radio Frequency.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioFrequency( void );

/*!
 * \brief Return text with current Radio Frequency [GHz] format #.###.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioFrequencyGHz( void );

/*!
 * \brief Return text with current Radio Freq Base.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioFreqBase( void );

/*!
 * \brief Return text with Radio preset Frequency 1.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioFreqBasePS1( void );

/*!
 * \brief Return text with Radio preset Frequency 2.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioFreqBasePS2( void );

/*!
 * \brief Return text with Radio preset Frequency 3.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioFreqBasePS3( void );

/*!
 * \brief Return text with current Radio Tx Power.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioTxPower( void );

/*!
 * \brief Return text with current Radio Payload Length.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRadioPayloadLength( void );

/*!
 * \brief Return text with Max Num Packet.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoMaxNumPacket( void );

/*!
 * \brief Return text with current Demo Num Sent Packet.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoNumSentPacket( void );

/*!
 * \brief Return text with current Rx frame Ok count.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoRxOk( void );

/*!
 * \brief Return text with current Rx frame Ko count.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoRxKo( void );

/*!
 * \brief Return text with current Rx frame Ok (on slave side) count.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoRxOkSlave( void );

/*!
 * \brief Return text with current Rx frame Ko (on slave side) count.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoRxKoSlave( void );

/*!
 * \brief Return text with current Result PerCent1, format ###.##.
 *
 * \param [in]  value         value to compute in [%]
 * \param [in]  reference     reference value for % computation
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoResultPerCent1( uint32_t value, uint32_t reference );

/*!
 * \brief Return text with current Result PerCent2, format ###.##.
 *
 * \param [in]  value         value to compute in [%]
 * \param [in]  reference     reference value for % computation
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoResultPerCent2( uint32_t value, uint32_t reference );

/*!
 * \brief Return text with current Rssi.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoRssi( void );

/*!
 * \brief Return text with current Snr.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoSnr( void );

/*!
 * \brief Return text with current Antenna Setting.
 *
 * \retval      text          Pointer on text to display
 */
char* GetAntennaSetting( void );

/*!
 * \brief Return text with Total Packet for the test.
 *
 * \retval      text          Pointer on text to display
 */
char* GetTotalPackets( void );

/*!
 * \brief Return text with current GPS Time.
 *
 * \retval      text          Pointer on text to display
 */
char* GetGpsTime( void );

/*!
 * \brief Return text with current GPS Position.
 *
 * \retval      text          Pointer on text to display
 */
char* GetGpsPos( void );

/*!
 * \brief Return text with current Proximity Value.
 *
 * \retval      text          Pointer on text to display
 */
char* GetProximityValue( void );

/*!
 * \brief Return text with current Radio Power Mode Value.
 *
 * \retval      text          Pointer on text to display
 */
char* GetMenuDemoRadioPowerMode( void );

/*!
 * \brief Return text with current Frequency Error Value.
 *
 * \retval      text          Pointer on text to display
 */
char* GetFrequencyError( void );

/*!
 * \brief Return text with current Ranging Channels Successfully Done Value.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRngChannelsOk( void );

/*!
 * \brief Return text with current Ranging Request Count Value.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRangingRequestCount( void );

/*!
 * \brief Return text with current Ranging Address Value.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRangingAddress( void );

/*!
 * \brief Return text with current Ranging Antenna Value.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRangingAntenna( void );

/*!
 * \brief Return text with current Ranging Distance Unit.
 *
 * \retval      text          Pointer on text to display
 */
char* GetRangingUnit( void );

#endif // MENU_H
