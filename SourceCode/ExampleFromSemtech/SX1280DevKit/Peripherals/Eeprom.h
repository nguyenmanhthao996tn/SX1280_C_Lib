/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: EEPROM routines header

Maintainer: Gregory Cristian & Gilbert Menth
*/

#ifndef EEPROM_H
#define EEPROM_H


#include "Menu.h"
#include "DemoApplication.h"
#include "sx1280.h"


/*!
 * \brief Define Buffer indexes for each EepromData for copy and restore
 */
#define SCR_CAL_FLAG_EEPROM_ADDR            0   // boolean
#define SCR_CAL_POSA_EEPROM_ADDR            1   // int32
#define SCR_CAL_POSB_EEPROM_ADDR            5   // int32
#define SCR_CAL_POSC_EEPROM_ADDR            9   // int32
#define SCR_CAL_POSD_EEPROM_ADDR            13  // int32
#define SCR_CAL_POSE_EEPROM_ADDR            17  // int32
#define SCR_CAL_POSF_EEPROM_ADDR            21  // int32
#define APP_ENTITY_EEPROM_ADDR              25  // uint8
#define APP_ANT_SW_EEPROM_ADDR              26  // uint8
#define APP_FREQ_EEPROM_ADDR                27  // uint32_t Frequency;
#define APP_TXPWR_EEPROM_ADDR               31  // int8_t TxPower;
#define APP_MOD_TYPE_EEPROM_ADDR            32  // uint8_t ModulationType;
#define APP_PER_NPAK_MAX_EEPROM_ADDR        33  // uint32_t MaxNumPacket;
#define APP_RADIO_POWER_MODE_EEPROM_ADDR    37  // RadioRegulatorMode_t
#define APP_RNG_REQ_COUNT_EEPROM_ADDR       38  // uint8_t
#define APP_RNG_FULLSCALE_EEPROM_ADDR       39  // uint16_t
#define APP_RNG_RFU_EEPROM_ADDR             41
#define APP_RNG_UNIT_EEPROM_ADDR            42  // ranging distance unit
#define APP_RNG_ADDR_EEPROM_ADDR            43  // uint32_t
#define APP_RNG_ANT_EEPROM_ADDR             47  // uint8_t RngAntenna
#define MOD_PAK_TYPE_EEPROM_ADDR            48  // enum
#define MOD_GFS_BRBW_EEPROM_ADDR            49  // BitrateBandwidth
#define MOD_GFS_MOD_IND_EEPROM_ADDR         50  // ModulationIndex
#define MOD_GFS_MOD_SHAP_EEPROM_ADDR        51  // ModulationShaping
#define MOD_LOR_SPREADF_EEPROM_ADDR         52  // SpreadingFactor
#define MOD_LOR_BW_EEPROM_ADDR              53  // Bandwidth
#define MOD_LOR_CODERATE_EEPROM_ADDR        54  // CodingRate
#define MOD_FLR_BRBW_EEPROM_ADDR            55  // BitrateBandwidth
#define MOD_FLR_CODERATE_EEPROM_ADDR        56  // CodingRate
#define MOD_FLR_MOD_SHAP_EEPROM_ADDR        57  // ModulationShaping
#define MOD_BLE_BRBW_EEPROM_ADDR            58  // BitrateBandwidth
#define MOD_BLE_MOD_IND_EEPROM_ADDR         59  // ModulationIndex
#define MOD_BLE_MOD_SHAP_EEPROM_ADDR        60  // ModulationShaping
#define MOD_RNG_SPREADF_EEPROM_ADDR         61  // SpreadingFactor
#define MOD_RNG_BW_EEPROM_ADDR              62  // Bandwidth
#define MOD_RNG_CODERATE_EEPROM_ADDR        63  // CodingRate
#define PAK_PAK_TYPE_EEPROM_ADDR            64  // 
#define PAK_GFS_PREAMBLE_LEN_EEPROM_ADDR    65  // PreambleLength
#define PAK_GFS_SYNC_LEN_EEPROM_ADDR        66  // SyncWordLength
#define PAK_GFS_SYNC_MATCH_EEPROM_ADDR      67  // SyncWordMatch
#define PAK_GFS_HEADERTYPE_EEPROM_ADDR      68  // HeaderType
#define PAK_GFS_PL_LEN_EEPROM_ADDR          69  // PayloadLength
#define PAK_GFS_CRC_LEN_EEPROM_ADDR         70  // CrcLength
#define PAK_GFS_WHITENING_EEPROM_ADDR       71  // Whitening
#define PAK_LOR_PREAMBLE_LEN_EEPROM_ADDR    72  // PreambleLength
#define PAK_LOR_HEADERTYPE_EEPROM_ADDR      73  // HeaderType
#define PAK_LOR_PL_LEN_EEPROM_ADDR          74  // PayloadLength
#define PAK_LOR_CRC_MODE_EEPROM_ADDR        75  // CrcMode
#define PAK_LOR_IQ_INV_EEPROM_ADDR          76  // InvertIQ
#define PAK_RNG_PREAMBLE_LEN_EEPROM_ADDR    77  // PreambleLength
#define PAK_RNG_HEADERTYPE_EEPROM_ADDR      78  // HeaderType
#define PAK_RNG_PL_LEN_EEPROM_ADDR          79  // PayloadLength
#define PAK_RNG_CRC_MODE_EEPROM_ADDR        80  // CrcMode
#define PAK_RNG_IQ_INV_EEPROM_ADDR          81  // InvertIQ
#define PAK_FLR_PREAMBLE_LEN_EEPROM_ADDR    82  // PreambleLength
#define PAK_FLR_SYNC_LEN_EEPROM_ADDR        83  // SyncWordLength
#define PAK_FLR_SYNC_MATCH_EEPROM_ADDR      84  // SyncWordMatch
#define PAK_FLR_HEADERTYPE_EEPROM_ADDR      85  // HeaderType
#define PAK_FLR_PL_LEN_EEPROM_ADDR          86  // PayloadLength
#define PAK_FLR_CRC_LEN_EEPROM_ADDR         87  // CrcLength
#define PAK_FLR_WHITENING_EEPROM_ADDR       88  // Whitening
#define PAK_BLE_CON_STATE_EEPROM_ADDR       89  // ConnectionState
#define PAK_BLE_CRC_FIELD_EEPROM_ADDR       90  // CrcField
#define PAK_BLE_PAK_TYPE_EEPROM_ADDR        91  // BlePacketType
#define PAK_BLE_WHITENING_EEPROM_ADDR       92  // Whitening
#define EEPROM_CRC_EEPROM_ADDR              93  // uint16

/*!
 * \brief Eeprom buffer size. Cf. above.
 */
#define EEPROM_BUFFER_SIZE                  95


/*!
 * \brief Part of EEPROM to save or restore
 */
typedef enum
{
    ALL_DATA,
    SCREEN_DATA,
    DEMO_SETTINGS,
    RADIO_LORA_PARAMS,
    RADIO_RANGING_PARAMS,
    RADIO_FLRC_PARAMS,
    RADIO_GFSK_PARAMS,
    RADIO_BLE_PARAMS
}EepromDataSet_t;

/*!
 * \brief EepromData structure
 */
typedef struct
{
    MenuSettings_t MenuSettings;
    DemoSettings_t DemoSettings;
    ModulationParams_t ModulationParams;
    PacketParams_t PacketParams;
    uint16_t CheckSum;
}EepromData_t;

/*!
 * \brief Eeprom structure
 */
typedef struct
{
    EepromData_t EepromData;
    // Allows for the checksum to be carried out
    uint8_t Buffer[EEPROM_BUFFER_SIZE];
}Eeprom_t;


/*!
 * \brief Local copy of Eeprom. (defined in Eeprom.cpp)
 */
extern Eeprom_t Eeprom;


/*!
 * \brief Initialises the contents of EepromData
 */
void EepromInit( void );

/*!
 * \brief Read Eeprom from emulated EEPROM (in fact in Flash " higher address).
 *
 * \param [in]  addr          address of data (EEPROM offset not to be include)
 * \param [in]  buffer        buffer to use for copy
 * \param [in]  size          size of data to copy
 *
 * \retval      status        Status of operation (SUCCESS, ..)
 */
uint8_t EepromMcuReadBuffer( uint16_t addr, uint8_t *buffer, uint16_t size );

/*!
 * \brief Writes the EepromData to emulated EEPROM
 *
 * \param [in]  dataSet       Set of data to save or restore
 */
void EepromSaveSettings( EepromDataSet_t dataSet);

/*!
 * \brief Loads EepromData from emulated EEPROM
 */
void EepromLoadGeneralSettings ( void );

/*!
 * \brief Loads EepromData with updated modulation and packet parameters
 *
 * \param [in]  modulation    modulation type to select for mod. & packet params
 */
void EepromLoadSettings( RadioPacketTypes_t modulation );

/*!
 * \brief Initialises the contents of flash to default values & save to EEPROM
 */
void EepromSetDefaultSettings( void );

/*!
 * \brief Initialises the contents of flash to default values (for ranging
 *        demo : best set) & save to EEPROM
 */
void EepromSetRangingDefaultSettings( void );

/*!
 * \brief Reset the EEPROM to factory state
 */
void EepromFactoryReset( void );

#endif //EEPROM_H
