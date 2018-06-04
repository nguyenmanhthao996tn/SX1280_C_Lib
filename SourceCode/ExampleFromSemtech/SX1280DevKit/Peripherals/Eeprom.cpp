/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: EEPROM routines

Maintainer: Gregory Cristian & Gilbert Menth
*/

#include "mbed.h"
#include "string.h"
#include "Eeprom.h"
#include "Menu.h"
#include "DemoApplication.h"
#include "sx1280.h"
#if defined( TARGET_NUCLEO_L476RG )
#include "stm32l4xx_hal_flash.h"
#elif defined( TARGET_NUCLEO_L152RE )
#include "stm32l1xx_hal_flash.h"
#else
#error "Please define include file"
#endif
#include "DisplayDriver.h"


/*!
 * \brief Define address of Emulated EEPROM (in fact region of Flash)
 */
#if defined( TARGET_NUCLEO_L476RG )
#define DATA_EEPROM_BASE    ( ( uint32_t )0x0807F800U )
#define DATA_EEPROM_END     ( ( uint32_t )DATA_EEPROM_BASE + 2048 )
#elif defined( TARGET_NUCLEO_L152RE )
#define DATA_EEPROM_BASE    ( ( uint32_t )0x08080000U ) 
#define DATA_EEPROM_END     ( ( uint32_t )0x080807FFU )
#else
#error "Please define EEPROM base address and size for your board "
#endif


/*!
 * \brief CRC of EEPROM buffer and its valid status
 */
typedef struct
{
    uint16_t Value;
    bool Valid;
}MemTestStruct_t;

/*!
 * \brief Local copy of Eeprom.
 */
Eeprom_t Eeprom;


// Check CRC of local copy of Eeprom (Buffer). This update Valid & Value
static MemTestStruct_t EepromDataCheckSum( void );
uint8_t EepromMcuWriteBuffer( uint16_t addr, uint8_t *buffer, uint16_t size );

void EepromInit( void )
{
    MemTestStruct_t memTestStruct;

    EepromMcuReadBuffer( 0, Eeprom.Buffer, EEPROM_BUFFER_SIZE );
    EepromLoadGeneralSettings( );

    memTestStruct = EepromDataCheckSum( );
    if( !( memTestStruct.Valid ) )
    {
        printf("EepromDataCheckSum failed\n\r");
        EepromSetDefaultSettings( );
    }
    EepromLoadSettings( ( RadioPacketTypes_t ) Eeprom.EepromData.DemoSettings.ModulationType );
}

void EepromSaveSettings( EepromDataSet_t dataSet)
{
    MemTestStruct_t memTestStruct;

    switch( dataSet )
    {
        case RADIO_LORA_PARAMS:
            Eeprom.EepromData.ModulationParams.Params.LoRa.SpreadingFactor = ( RadioLoRaSpreadingFactors_t )  Eeprom.EepromData.DemoSettings.ModulationParam1;
            Eeprom.EepromData.ModulationParams.Params.LoRa.Bandwidth       = ( RadioLoRaBandwidths_t )        Eeprom.EepromData.DemoSettings.ModulationParam2;
            Eeprom.EepromData.ModulationParams.Params.LoRa.CodingRate      = ( RadioLoRaCodingRates_t )       Eeprom.EepromData.DemoSettings.ModulationParam3;
            Eeprom.EepromData.PacketParams.Params.LoRa.PreambleLength      =                                  Eeprom.EepromData.DemoSettings.PacketParam1;
            Eeprom.EepromData.PacketParams.Params.LoRa.HeaderType          = ( RadioLoRaPacketLengthsModes_t )Eeprom.EepromData.DemoSettings.PacketParam2;
            Eeprom.EepromData.PacketParams.Params.LoRa.PayloadLength       =                                  Eeprom.EepromData.DemoSettings.PacketParam3;
            Eeprom.EepromData.PacketParams.Params.LoRa.Crc                 = ( RadioLoRaCrcModes_t )          Eeprom.EepromData.DemoSettings.PacketParam4;
            Eeprom.EepromData.PacketParams.Params.LoRa.InvertIQ            = ( RadioLoRaIQModes_t )           Eeprom.EepromData.DemoSettings.PacketParam5;

            memcpy( Eeprom.Buffer + MOD_LOR_SPREADF_EEPROM_ADDR,      &( Eeprom.EepromData.ModulationParams.Params.LoRa.SpreadingFactor ), 1 );
            memcpy( Eeprom.Buffer + MOD_LOR_BW_EEPROM_ADDR,           &( Eeprom.EepromData.ModulationParams.Params.LoRa.Bandwidth ),       1 );
            memcpy( Eeprom.Buffer + MOD_LOR_CODERATE_EEPROM_ADDR,     &( Eeprom.EepromData.ModulationParams.Params.LoRa.CodingRate ),      1 );
            memcpy( Eeprom.Buffer + PAK_LOR_PREAMBLE_LEN_EEPROM_ADDR, &( Eeprom.EepromData.PacketParams.Params.LoRa.PreambleLength ),      1 );
            memcpy( Eeprom.Buffer + PAK_LOR_HEADERTYPE_EEPROM_ADDR,   &( Eeprom.EepromData.PacketParams.Params.LoRa.HeaderType ),          1 );
            memcpy( Eeprom.Buffer + PAK_LOR_PL_LEN_EEPROM_ADDR,       &( Eeprom.EepromData.PacketParams.Params.LoRa.PayloadLength ),       1 );
            memcpy( Eeprom.Buffer + PAK_LOR_CRC_MODE_EEPROM_ADDR,     &( Eeprom.EepromData.PacketParams.Params.LoRa.Crc ),                 1 );
            memcpy( Eeprom.Buffer + PAK_LOR_IQ_INV_EEPROM_ADDR,       &( Eeprom.EepromData.PacketParams.Params.LoRa.InvertIQ ),            1 );
            printf("Saved RADIO_LORA_PARAMS\n\r");
            break;

        case RADIO_RANGING_PARAMS:
            memcpy( Eeprom.Buffer + MOD_RNG_SPREADF_EEPROM_ADDR,      &( Eeprom.EepromData.DemoSettings.ModulationParam1 ), 1 );
            memcpy( Eeprom.Buffer + MOD_RNG_BW_EEPROM_ADDR,           &( Eeprom.EepromData.DemoSettings.ModulationParam2 ), 1 );
            memcpy( Eeprom.Buffer + MOD_RNG_CODERATE_EEPROM_ADDR,     &( Eeprom.EepromData.DemoSettings.ModulationParam3 ), 1 );
            memcpy( Eeprom.Buffer + PAK_RNG_PREAMBLE_LEN_EEPROM_ADDR, &( Eeprom.EepromData.DemoSettings.PacketParam1 ),     1 );
            memcpy( Eeprom.Buffer + PAK_RNG_HEADERTYPE_EEPROM_ADDR,   &( Eeprom.EepromData.DemoSettings.PacketParam2 ),     1 );
            memcpy( Eeprom.Buffer + PAK_RNG_PL_LEN_EEPROM_ADDR,       &( Eeprom.EepromData.DemoSettings.PacketParam3 ),     1 );
            memcpy( Eeprom.Buffer + PAK_RNG_CRC_MODE_EEPROM_ADDR,     &( Eeprom.EepromData.DemoSettings.PacketParam4 ),     1 );
            memcpy( Eeprom.Buffer + PAK_RNG_IQ_INV_EEPROM_ADDR,       &( Eeprom.EepromData.DemoSettings.PacketParam5 ),     1 );

            memcpy( Eeprom.Buffer + APP_RNG_REQ_COUNT_EEPROM_ADDR, &( Eeprom.EepromData.DemoSettings.RngRequestCount ), 1 );
            memcpy( Eeprom.Buffer + APP_RNG_FULLSCALE_EEPROM_ADDR, &( Eeprom.EepromData.DemoSettings.RngFullScale ),    2 );
            memcpy( Eeprom.Buffer + APP_RNG_ADDR_EEPROM_ADDR,      &( Eeprom.EepromData.DemoSettings.RngAddress ),      4 );
            memcpy( Eeprom.Buffer + APP_RNG_ANT_EEPROM_ADDR,       &( Eeprom.EepromData.DemoSettings.RngAntenna ),      1 );
            memcpy( Eeprom.Buffer + APP_RNG_UNIT_EEPROM_ADDR,      &( Eeprom.EepromData.DemoSettings.RngUnit ),         1 );
            printf("Saved RADIO_RANGING_PARAMS\n\r");
            break;

        case RADIO_FLRC_PARAMS:
            Eeprom.EepromData.ModulationParams.Params.Flrc.BitrateBandwidth  = ( RadioFlrcBitrates_t )       Eeprom.EepromData.DemoSettings.ModulationParam1;
            Eeprom.EepromData.ModulationParams.Params.Flrc.CodingRate        = ( RadioFlrcCodingRates_t )    Eeprom.EepromData.DemoSettings.ModulationParam2;
            Eeprom.EepromData.ModulationParams.Params.Flrc.ModulationShaping = ( RadioModShapings_t )        Eeprom.EepromData.DemoSettings.ModulationParam3;
            Eeprom.EepromData.PacketParams.Params.Flrc.PreambleLength        = ( RadioPreambleLengths_t )    Eeprom.EepromData.DemoSettings.PacketParam1;
            Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordLength        = ( RadioFlrcSyncWordLengths_t )Eeprom.EepromData.DemoSettings.PacketParam2;
            Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordMatch         = ( RadioSyncWordRxMatchs_t )   Eeprom.EepromData.DemoSettings.PacketParam3;
            Eeprom.EepromData.PacketParams.Params.Flrc.HeaderType            = ( RadioPacketLengthModes_t )  Eeprom.EepromData.DemoSettings.PacketParam4;
            Eeprom.EepromData.PacketParams.Params.Flrc.PayloadLength         =                               Eeprom.EepromData.DemoSettings.PacketParam5;
            Eeprom.EepromData.PacketParams.Params.Flrc.CrcLength             = ( RadioCrcTypes_t )           Eeprom.EepromData.DemoSettings.PacketParam6;
            Eeprom.EepromData.PacketParams.Params.Flrc.Whitening             = ( RadioWhiteningModes_t )     Eeprom.EepromData.DemoSettings.PacketParam7;

            memcpy( Eeprom.Buffer + MOD_FLR_BRBW_EEPROM_ADDR,         &( Eeprom.EepromData.ModulationParams.Params.Flrc.BitrateBandwidth ),  1 );
            memcpy( Eeprom.Buffer + MOD_FLR_CODERATE_EEPROM_ADDR,     &( Eeprom.EepromData.ModulationParams.Params.Flrc.CodingRate ),        1 );
            memcpy( Eeprom.Buffer + MOD_FLR_MOD_SHAP_EEPROM_ADDR,     &( Eeprom.EepromData.ModulationParams.Params.Flrc.ModulationShaping ), 1 );
            memcpy( Eeprom.Buffer + PAK_FLR_PREAMBLE_LEN_EEPROM_ADDR, &( Eeprom.EepromData.PacketParams.Params.Flrc.PreambleLength ),        1 );
            memcpy( Eeprom.Buffer + PAK_FLR_SYNC_LEN_EEPROM_ADDR,     &( Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordLength ),        1 );
            memcpy( Eeprom.Buffer + PAK_FLR_SYNC_MATCH_EEPROM_ADDR,   &( Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordMatch ),         1 );
            memcpy( Eeprom.Buffer + PAK_FLR_HEADERTYPE_EEPROM_ADDR,   &( Eeprom.EepromData.PacketParams.Params.Flrc.HeaderType ),            1 );
            memcpy( Eeprom.Buffer + PAK_FLR_PL_LEN_EEPROM_ADDR,       &( Eeprom.EepromData.PacketParams.Params.Flrc.PayloadLength ),         1 );
            memcpy( Eeprom.Buffer + PAK_FLR_CRC_LEN_EEPROM_ADDR,      &( Eeprom.EepromData.PacketParams.Params.Flrc.CrcLength ),             1 );
            memcpy( Eeprom.Buffer + PAK_FLR_WHITENING_EEPROM_ADDR,    &( Eeprom.EepromData.PacketParams.Params.Flrc.Whitening ),             1 );
            printf("Saved RADIO_FLRC_PARAMS\n\r");
            break;

        case RADIO_GFSK_PARAMS:
            Eeprom.EepromData.ModulationParams.Params.Gfsk.BitrateBandwidth  = ( RadioGfskBleBitrates_t )  Eeprom.EepromData.DemoSettings.ModulationParam1;
            Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationIndex   = ( RadioGfskBleModIndexes_t )Eeprom.EepromData.DemoSettings.ModulationParam2;
            Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationShaping = ( RadioModShapings_t )      Eeprom.EepromData.DemoSettings.ModulationParam3;
            Eeprom.EepromData.PacketParams.Params.Gfsk.PreambleLength        = ( RadioPreambleLengths_t )  Eeprom.EepromData.DemoSettings.PacketParam1;
            Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordLength        = ( RadioSyncWordLengths_t )  Eeprom.EepromData.DemoSettings.PacketParam2;
            Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordMatch         = ( RadioSyncWordRxMatchs_t ) Eeprom.EepromData.DemoSettings.PacketParam3;
            Eeprom.EepromData.PacketParams.Params.Gfsk.HeaderType            = ( RadioPacketLengthModes_t )Eeprom.EepromData.DemoSettings.PacketParam4;
            Eeprom.EepromData.PacketParams.Params.Gfsk.PayloadLength         =                             Eeprom.EepromData.DemoSettings.PacketParam5;
            Eeprom.EepromData.PacketParams.Params.Gfsk.CrcLength             = ( RadioCrcTypes_t )         Eeprom.EepromData.DemoSettings.PacketParam6;
            Eeprom.EepromData.PacketParams.Params.Gfsk.Whitening             = ( RadioWhiteningModes_t )   Eeprom.EepromData.DemoSettings.PacketParam7;

            memcpy( Eeprom.Buffer + MOD_GFS_BRBW_EEPROM_ADDR,         &( Eeprom.EepromData.ModulationParams.Params.Gfsk.BitrateBandwidth ),  1 );
            memcpy( Eeprom.Buffer + MOD_GFS_MOD_IND_EEPROM_ADDR,      &( Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationIndex ),   1 );
            memcpy( Eeprom.Buffer + MOD_GFS_MOD_SHAP_EEPROM_ADDR,     &( Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationShaping ), 1 );
            memcpy( Eeprom.Buffer + PAK_GFS_PREAMBLE_LEN_EEPROM_ADDR, &( Eeprom.EepromData.PacketParams.Params.Gfsk.PreambleLength ),        1 );
            memcpy( Eeprom.Buffer + PAK_GFS_SYNC_LEN_EEPROM_ADDR,     &( Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordLength ),        1 );
            memcpy( Eeprom.Buffer + PAK_GFS_SYNC_MATCH_EEPROM_ADDR,   &( Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordMatch ),         1 );
            memcpy( Eeprom.Buffer + PAK_GFS_HEADERTYPE_EEPROM_ADDR,   &( Eeprom.EepromData.PacketParams.Params.Gfsk.HeaderType ),            1 );
            memcpy( Eeprom.Buffer + PAK_GFS_PL_LEN_EEPROM_ADDR,       &( Eeprom.EepromData.PacketParams.Params.Gfsk.PayloadLength ),         1 );
            memcpy( Eeprom.Buffer + PAK_GFS_CRC_LEN_EEPROM_ADDR,      &( Eeprom.EepromData.PacketParams.Params.Gfsk.CrcLength ),             1 );
            memcpy( Eeprom.Buffer + PAK_GFS_WHITENING_EEPROM_ADDR,    &( Eeprom.EepromData.PacketParams.Params.Gfsk.Whitening ),             1 );
            printf("Saved RADIO_GFSK_PARAMS\n\r");
            break;

        case DEMO_SETTINGS:
            memcpy( Eeprom.Buffer + APP_ENTITY_EEPROM_ADDR,           &( Eeprom.EepromData.DemoSettings.Entity ),           1 );
            memcpy( Eeprom.Buffer + APP_ANT_SW_EEPROM_ADDR,           &( Eeprom.EepromData.DemoSettings.AntennaSwitch ),    1 );
            memcpy( Eeprom.Buffer + APP_FREQ_EEPROM_ADDR,             &( Eeprom.EepromData.DemoSettings.Frequency ),        4 );
            memcpy( Eeprom.Buffer + APP_TXPWR_EEPROM_ADDR,            &( Eeprom.EepromData.DemoSettings.TxPower ),          1 );
            memcpy( Eeprom.Buffer + APP_MOD_TYPE_EEPROM_ADDR,         &( Eeprom.EepromData.DemoSettings.ModulationType ),   1 );
            memcpy( Eeprom.Buffer + APP_PER_NPAK_MAX_EEPROM_ADDR,     &( Eeprom.EepromData.DemoSettings.MaxNumPacket ),     4 );
            memcpy( Eeprom.Buffer + APP_RADIO_POWER_MODE_EEPROM_ADDR, &( Eeprom.EepromData.DemoSettings.RadioPowerMode ),   1 );
            memcpy( Eeprom.Buffer + MOD_PAK_TYPE_EEPROM_ADDR,         &( Eeprom.EepromData.DemoSettings.ModulationType ),   1 );
            memcpy( Eeprom.Buffer + PAK_PAK_TYPE_EEPROM_ADDR,         &( Eeprom.EepromData.DemoSettings.ModulationType ),   1 );
            printf("Saved DEMO_SETTINGS\n\r");
            break;

        case SCREEN_DATA:
            memcpy( Eeprom.Buffer + SCR_CAL_FLAG_EEPROM_ADDR, &( Eeprom.EepromData.MenuSettings.ScreenCalibrated ), 1 );
            memcpy( Eeprom.Buffer + SCR_CAL_POSA_EEPROM_ADDR, &( Eeprom.EepromData.MenuSettings.Calibration.a ),    4 );
            memcpy( Eeprom.Buffer + SCR_CAL_POSB_EEPROM_ADDR, &( Eeprom.EepromData.MenuSettings.Calibration.b ),    4 );
            memcpy( Eeprom.Buffer + SCR_CAL_POSC_EEPROM_ADDR, &( Eeprom.EepromData.MenuSettings.Calibration.c ),    4 );
            memcpy( Eeprom.Buffer + SCR_CAL_POSD_EEPROM_ADDR, &( Eeprom.EepromData.MenuSettings.Calibration.d ),    4 );
            memcpy( Eeprom.Buffer + SCR_CAL_POSE_EEPROM_ADDR, &( Eeprom.EepromData.MenuSettings.Calibration.e ),    4 );
            memcpy( Eeprom.Buffer + SCR_CAL_POSF_EEPROM_ADDR, &( Eeprom.EepromData.MenuSettings.Calibration.f ),    4 );
            break;

        default:
            printf("data not saved\n\r");
            break;
    }

    memTestStruct = EepromDataCheckSum( );
    memcpy( Eeprom.Buffer + EEPROM_CRC_EEPROM_ADDR, &( memTestStruct.Value ), 2 );

    EepromMcuWriteBuffer( 0, Eeprom.Buffer, EEPROM_BUFFER_SIZE );
}

void EepromLoadGeneralSettings( void )
{
    printf("Load General Settings\n\r");
    memcpy( &( Eeprom.EepromData.MenuSettings.ScreenCalibrated ), Eeprom.Buffer + SCR_CAL_FLAG_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.MenuSettings.Calibration.a ),    Eeprom.Buffer + SCR_CAL_POSA_EEPROM_ADDR, 4 );
    memcpy( &( Eeprom.EepromData.MenuSettings.Calibration.b ),    Eeprom.Buffer + SCR_CAL_POSB_EEPROM_ADDR, 4 );
    memcpy( &( Eeprom.EepromData.MenuSettings.Calibration.c ),    Eeprom.Buffer + SCR_CAL_POSC_EEPROM_ADDR, 4 );
    memcpy( &( Eeprom.EepromData.MenuSettings.Calibration.d ),    Eeprom.Buffer + SCR_CAL_POSD_EEPROM_ADDR, 4 );
    memcpy( &( Eeprom.EepromData.MenuSettings.Calibration.e ),    Eeprom.Buffer + SCR_CAL_POSE_EEPROM_ADDR, 4 );
    memcpy( &( Eeprom.EepromData.MenuSettings.Calibration.f ),    Eeprom.Buffer + SCR_CAL_POSF_EEPROM_ADDR, 4 );

    memcpy( &( Eeprom.EepromData.DemoSettings.Entity ),         Eeprom.Buffer + APP_ENTITY_EEPROM_ADDR,           1 );
    memcpy( &( Eeprom.EepromData.DemoSettings.AntennaSwitch ),  Eeprom.Buffer + APP_ANT_SW_EEPROM_ADDR,           1 );
    memcpy( &( Eeprom.EepromData.DemoSettings.Frequency ),      Eeprom.Buffer + APP_FREQ_EEPROM_ADDR,             4 );
    memcpy( &( Eeprom.EepromData.DemoSettings.RadioPowerMode ), Eeprom.Buffer + APP_RADIO_POWER_MODE_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.DemoSettings.TxPower ),        Eeprom.Buffer + APP_TXPWR_EEPROM_ADDR,            1 );
    memcpy( &( Eeprom.EepromData.DemoSettings.ModulationType ), Eeprom.Buffer + APP_MOD_TYPE_EEPROM_ADDR,         1 );
    memcpy( &( Eeprom.EepromData.DemoSettings.MaxNumPacket ),   Eeprom.Buffer + APP_PER_NPAK_MAX_EEPROM_ADDR,     4 );

    memcpy( &( Eeprom.EepromData.ModulationParams.PacketType ),                    Eeprom.Buffer + MOD_PAK_TYPE_EEPROM_ADDR,     1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Gfsk.BitrateBandwidth ),  Eeprom.Buffer + MOD_GFS_BRBW_EEPROM_ADDR,     1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationIndex ),   Eeprom.Buffer + MOD_GFS_MOD_IND_EEPROM_ADDR,  1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationShaping ), Eeprom.Buffer + MOD_GFS_MOD_SHAP_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.SpreadingFactor ),   Eeprom.Buffer + MOD_LOR_SPREADF_EEPROM_ADDR,  1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.Bandwidth ),         Eeprom.Buffer + MOD_LOR_BW_EEPROM_ADDR,       1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.CodingRate ),        Eeprom.Buffer + MOD_LOR_CODERATE_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Flrc.BitrateBandwidth ),  Eeprom.Buffer + MOD_FLR_BRBW_EEPROM_ADDR,     1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Flrc.CodingRate ),        Eeprom.Buffer + MOD_FLR_CODERATE_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Flrc.ModulationShaping ), Eeprom.Buffer + MOD_FLR_MOD_SHAP_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Ble.BitrateBandwidth ),   Eeprom.Buffer + MOD_BLE_BRBW_EEPROM_ADDR,     1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Ble.ModulationIndex ),    Eeprom.Buffer + MOD_BLE_MOD_IND_EEPROM_ADDR,  1 );
    memcpy( &( Eeprom.EepromData.ModulationParams.Params.Ble.ModulationShaping ),  Eeprom.Buffer + MOD_BLE_MOD_SHAP_EEPROM_ADDR, 1 );

    memcpy( &( Eeprom.EepromData.PacketParams.PacketType ),                 Eeprom.Buffer + PAK_PAK_TYPE_EEPROM_ADDR,         1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.PreambleLength ), Eeprom.Buffer + PAK_GFS_PREAMBLE_LEN_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordLength ), Eeprom.Buffer + PAK_GFS_SYNC_LEN_EEPROM_ADDR,     1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordMatch ),  Eeprom.Buffer + PAK_GFS_SYNC_MATCH_EEPROM_ADDR,   1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.HeaderType ),     Eeprom.Buffer + PAK_GFS_HEADERTYPE_EEPROM_ADDR,   1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.PayloadLength ),  Eeprom.Buffer + PAK_GFS_PL_LEN_EEPROM_ADDR,       1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.CrcLength ),      Eeprom.Buffer + PAK_GFS_CRC_LEN_EEPROM_ADDR,      1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.Whitening ),      Eeprom.Buffer + PAK_GFS_WHITENING_EEPROM_ADDR,    1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.PreambleLength ), Eeprom.Buffer + PAK_LOR_PREAMBLE_LEN_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.HeaderType ),     Eeprom.Buffer + PAK_LOR_HEADERTYPE_EEPROM_ADDR,   1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.PayloadLength ),  Eeprom.Buffer + PAK_LOR_PL_LEN_EEPROM_ADDR,       1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.Crc ),            Eeprom.Buffer + PAK_LOR_CRC_MODE_EEPROM_ADDR,     1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.InvertIQ ),       Eeprom.Buffer + PAK_LOR_IQ_INV_EEPROM_ADDR,       1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.PreambleLength ), Eeprom.Buffer + PAK_FLR_PREAMBLE_LEN_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordLength ), Eeprom.Buffer + PAK_FLR_SYNC_LEN_EEPROM_ADDR,     1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordMatch ),  Eeprom.Buffer + PAK_FLR_SYNC_MATCH_EEPROM_ADDR,   1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.HeaderType ),     Eeprom.Buffer + PAK_FLR_HEADERTYPE_EEPROM_ADDR,   1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.PayloadLength ),  Eeprom.Buffer + PAK_FLR_PL_LEN_EEPROM_ADDR,       1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.CrcLength ),      Eeprom.Buffer + PAK_FLR_CRC_LEN_EEPROM_ADDR,      1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.Whitening ),      Eeprom.Buffer + PAK_FLR_WHITENING_EEPROM_ADDR,    1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Ble.BleTestPayload ),  Eeprom.Buffer + PAK_BLE_PAK_TYPE_EEPROM_ADDR,     1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Ble.ConnectionState ), Eeprom.Buffer + PAK_BLE_CON_STATE_EEPROM_ADDR,    1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Ble.CrcLength ),       Eeprom.Buffer + PAK_BLE_CRC_FIELD_EEPROM_ADDR,    1 );
    memcpy( &( Eeprom.EepromData.PacketParams.Params.Ble.Whitening ),       Eeprom.Buffer + PAK_BLE_WHITENING_EEPROM_ADDR,    1 );

    memcpy( &( Eeprom.EepromData.DemoSettings.RngRequestCount ), Eeprom.Buffer + APP_RNG_REQ_COUNT_EEPROM_ADDR, 1 );
    memcpy( &( Eeprom.EepromData.DemoSettings.RngFullScale ),    Eeprom.Buffer + APP_RNG_FULLSCALE_EEPROM_ADDR, 2 );
    memcpy( &( Eeprom.EepromData.DemoSettings.RngAddress ),      Eeprom.Buffer + APP_RNG_ADDR_EEPROM_ADDR,      4 );
    memcpy( &( Eeprom.EepromData.DemoSettings.RngAntenna ),      Eeprom.Buffer + APP_RNG_ANT_EEPROM_ADDR,       1 );
    memcpy( &( Eeprom.EepromData.DemoSettings.RngUnit ),         Eeprom.Buffer + APP_RNG_UNIT_EEPROM_ADDR,      1 );
}

void EepromLoadSettings( RadioPacketTypes_t modulation )
{
    if( modulation == PACKET_TYPE_LORA )
    {
        printf("Load Settings PACKET_TYPE_LORA\n\r");
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.SpreadingFactor ), Eeprom.Buffer + MOD_LOR_SPREADF_EEPROM_ADDR,      1 );
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.Bandwidth ),       Eeprom.Buffer + MOD_LOR_BW_EEPROM_ADDR,           1 );
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.CodingRate ),      Eeprom.Buffer + MOD_LOR_CODERATE_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.PreambleLength ),      Eeprom.Buffer + PAK_LOR_PREAMBLE_LEN_EEPROM_ADDR, 1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.HeaderType ),          Eeprom.Buffer + PAK_LOR_HEADERTYPE_EEPROM_ADDR,   1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.PayloadLength ),       Eeprom.Buffer + PAK_LOR_PL_LEN_EEPROM_ADDR,       1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.Crc ),                 Eeprom.Buffer + PAK_LOR_CRC_MODE_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.InvertIQ ),            Eeprom.Buffer + PAK_LOR_IQ_INV_EEPROM_ADDR,       1 );

        Eeprom.EepromData.ModulationParams.PacketType   = PACKET_TYPE_LORA;
        Eeprom.EepromData.PacketParams.PacketType       = PACKET_TYPE_LORA;
        Eeprom.EepromData.DemoSettings.ModulationParam1 = Eeprom.EepromData.ModulationParams.Params.LoRa.SpreadingFactor;
        Eeprom.EepromData.DemoSettings.ModulationParam2 = Eeprom.EepromData.ModulationParams.Params.LoRa.Bandwidth;
        Eeprom.EepromData.DemoSettings.ModulationParam3 = Eeprom.EepromData.ModulationParams.Params.LoRa.CodingRate;

        Eeprom.EepromData.DemoSettings.PacketParam1 = Eeprom.EepromData.PacketParams.Params.LoRa.PreambleLength;
        Eeprom.EepromData.DemoSettings.PacketParam2 = Eeprom.EepromData.PacketParams.Params.LoRa.HeaderType;
        Eeprom.EepromData.DemoSettings.PacketParam3 = Eeprom.EepromData.PacketParams.Params.LoRa.PayloadLength;
        Eeprom.EepromData.DemoSettings.PacketParam4 = Eeprom.EepromData.PacketParams.Params.LoRa.Crc;
        Eeprom.EepromData.DemoSettings.PacketParam5 = Eeprom.EepromData.PacketParams.Params.LoRa.InvertIQ;
        Eeprom.EepromData.DemoSettings.PacketParam6 = 0x00;
        Eeprom.EepromData.DemoSettings.PacketParam7 = 0x00;
    }
    else if( modulation == PACKET_TYPE_RANGING )
    {
        printf("Load Settings PACKET_TYPE_RANGING\n\r");
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.SpreadingFactor ), Eeprom.Buffer + MOD_RNG_SPREADF_EEPROM_ADDR,      1 );
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.Bandwidth ),       Eeprom.Buffer + MOD_RNG_BW_EEPROM_ADDR,           1 );
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.LoRa.CodingRate ),      Eeprom.Buffer + MOD_RNG_CODERATE_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.PreambleLength ),      Eeprom.Buffer + PAK_RNG_PREAMBLE_LEN_EEPROM_ADDR, 1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.HeaderType ),          Eeprom.Buffer + PAK_RNG_HEADERTYPE_EEPROM_ADDR,   1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.PayloadLength ),       Eeprom.Buffer + PAK_RNG_PL_LEN_EEPROM_ADDR,       1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.Crc ),                 Eeprom.Buffer + PAK_RNG_CRC_MODE_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.LoRa.InvertIQ ),            Eeprom.Buffer + PAK_RNG_IQ_INV_EEPROM_ADDR,       1 );
        memcpy( &( Eeprom.EepromData.DemoSettings.RngRequestCount ),                 Eeprom.Buffer + APP_RNG_REQ_COUNT_EEPROM_ADDR,    1 );
        memcpy( &( Eeprom.EepromData.DemoSettings.RngFullScale ),                    Eeprom.Buffer + APP_RNG_FULLSCALE_EEPROM_ADDR,    2 );
        memcpy( &( Eeprom.EepromData.DemoSettings.RngAddress ),                      Eeprom.Buffer + APP_RNG_ADDR_EEPROM_ADDR,         4 );
        memcpy( &( Eeprom.EepromData.DemoSettings.RngAntenna ),                      Eeprom.Buffer + APP_RNG_ANT_EEPROM_ADDR,          1 );
        memcpy( &( Eeprom.EepromData.DemoSettings.RngUnit ),                         Eeprom.Buffer + APP_RNG_UNIT_EEPROM_ADDR,         1 );

        Eeprom.EepromData.ModulationParams.PacketType   = PACKET_TYPE_RANGING;
        Eeprom.EepromData.PacketParams.PacketType       = PACKET_TYPE_RANGING;
        Eeprom.EepromData.DemoSettings.ModulationParam1 = Eeprom.EepromData.ModulationParams.Params.LoRa.SpreadingFactor;
        Eeprom.EepromData.DemoSettings.ModulationParam2 = Eeprom.EepromData.ModulationParams.Params.LoRa.Bandwidth;
        Eeprom.EepromData.DemoSettings.ModulationParam3 = Eeprom.EepromData.ModulationParams.Params.LoRa.CodingRate;

        Eeprom.EepromData.DemoSettings.PacketParam1 = Eeprom.EepromData.PacketParams.Params.LoRa.PreambleLength;
        Eeprom.EepromData.DemoSettings.PacketParam2 = Eeprom.EepromData.PacketParams.Params.LoRa.HeaderType;
        Eeprom.EepromData.DemoSettings.PacketParam3 = Eeprom.EepromData.PacketParams.Params.LoRa.PayloadLength;
        Eeprom.EepromData.DemoSettings.PacketParam4 = Eeprom.EepromData.PacketParams.Params.LoRa.Crc;
        Eeprom.EepromData.DemoSettings.PacketParam5 = Eeprom.EepromData.PacketParams.Params.LoRa.InvertIQ;
        Eeprom.EepromData.DemoSettings.PacketParam6 = 0x00;
        Eeprom.EepromData.DemoSettings.PacketParam7 = 0x00;
    }
    else if( modulation == PACKET_TYPE_FLRC )
    {
        printf("Load Settings PACKET_TYPE_FLRC\n\r");
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.Flrc.BitrateBandwidth ),  Eeprom.Buffer + MOD_FLR_BRBW_EEPROM_ADDR,         1 );
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.Flrc.CodingRate ),        Eeprom.Buffer + MOD_FLR_CODERATE_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.Flrc.ModulationShaping ), Eeprom.Buffer + MOD_FLR_MOD_SHAP_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.PreambleLength ),        Eeprom.Buffer + PAK_FLR_PREAMBLE_LEN_EEPROM_ADDR, 1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordLength ),        Eeprom.Buffer + PAK_FLR_SYNC_LEN_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordMatch ),         Eeprom.Buffer + PAK_FLR_SYNC_MATCH_EEPROM_ADDR,   1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.HeaderType ),            Eeprom.Buffer + PAK_FLR_HEADERTYPE_EEPROM_ADDR,   1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.PayloadLength ),         Eeprom.Buffer + PAK_FLR_PL_LEN_EEPROM_ADDR,       1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.CrcLength ),             Eeprom.Buffer + PAK_FLR_CRC_LEN_EEPROM_ADDR,      1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Flrc.Whitening ),             Eeprom.Buffer + PAK_FLR_WHITENING_EEPROM_ADDR,    1 );

        Eeprom.EepromData.ModulationParams.PacketType   = PACKET_TYPE_FLRC;
        Eeprom.EepromData.PacketParams.PacketType       = PACKET_TYPE_FLRC;
        Eeprom.EepromData.DemoSettings.ModulationParam1 = Eeprom.EepromData.ModulationParams.Params.Flrc.BitrateBandwidth;
        Eeprom.EepromData.DemoSettings.ModulationParam2 = Eeprom.EepromData.ModulationParams.Params.Flrc.CodingRate;
        Eeprom.EepromData.DemoSettings.ModulationParam3 = Eeprom.EepromData.ModulationParams.Params.Flrc.ModulationShaping;

        Eeprom.EepromData.DemoSettings.PacketParam1 = Eeprom.EepromData.PacketParams.Params.Flrc.PreambleLength;
        Eeprom.EepromData.DemoSettings.PacketParam2 = Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordLength;
        Eeprom.EepromData.DemoSettings.PacketParam3 = Eeprom.EepromData.PacketParams.Params.Flrc.SyncWordMatch;
        Eeprom.EepromData.DemoSettings.PacketParam4 = Eeprom.EepromData.PacketParams.Params.Flrc.HeaderType;
        Eeprom.EepromData.DemoSettings.PacketParam5 = Eeprom.EepromData.PacketParams.Params.Flrc.PayloadLength;
        Eeprom.EepromData.DemoSettings.PacketParam6 = Eeprom.EepromData.PacketParams.Params.Flrc.CrcLength;
        Eeprom.EepromData.DemoSettings.PacketParam7 = Eeprom.EepromData.PacketParams.Params.Flrc.Whitening;
    }
    else // GFSK
    {
        printf("Load Settings PACKET_TYPE_GFSK\n\r");
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.Gfsk.BitrateBandwidth ),  Eeprom.Buffer + MOD_GFS_BRBW_EEPROM_ADDR,         1 );
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationIndex ),   Eeprom.Buffer + MOD_GFS_MOD_IND_EEPROM_ADDR,      1 );
        memcpy( &( Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationShaping ), Eeprom.Buffer + MOD_GFS_MOD_SHAP_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.PreambleLength ),        Eeprom.Buffer + PAK_GFS_PREAMBLE_LEN_EEPROM_ADDR, 1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordLength ),        Eeprom.Buffer + PAK_GFS_SYNC_LEN_EEPROM_ADDR,     1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordMatch ),         Eeprom.Buffer + PAK_GFS_SYNC_MATCH_EEPROM_ADDR,   1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.HeaderType ),            Eeprom.Buffer + PAK_GFS_HEADERTYPE_EEPROM_ADDR,   1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.PayloadLength ),         Eeprom.Buffer + PAK_GFS_PL_LEN_EEPROM_ADDR,       1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.CrcLength ),             Eeprom.Buffer + PAK_GFS_CRC_LEN_EEPROM_ADDR,      1 );
        memcpy( &( Eeprom.EepromData.PacketParams.Params.Gfsk.Whitening ),             Eeprom.Buffer + PAK_GFS_WHITENING_EEPROM_ADDR,    1 );

        Eeprom.EepromData.ModulationParams.PacketType   = PACKET_TYPE_GFSK;
        Eeprom.EepromData.PacketParams.PacketType       = PACKET_TYPE_GFSK;
        Eeprom.EepromData.DemoSettings.ModulationParam1 = Eeprom.EepromData.ModulationParams.Params.Gfsk.BitrateBandwidth;
        Eeprom.EepromData.DemoSettings.ModulationParam2 = Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationIndex;
        Eeprom.EepromData.DemoSettings.ModulationParam3 = Eeprom.EepromData.ModulationParams.Params.Gfsk.ModulationShaping;

        Eeprom.EepromData.DemoSettings.PacketParam1 = Eeprom.EepromData.PacketParams.Params.Gfsk.PreambleLength;
        Eeprom.EepromData.DemoSettings.PacketParam2 = Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordLength;
        Eeprom.EepromData.DemoSettings.PacketParam3 = Eeprom.EepromData.PacketParams.Params.Gfsk.SyncWordMatch;
        Eeprom.EepromData.DemoSettings.PacketParam4 = Eeprom.EepromData.PacketParams.Params.Gfsk.HeaderType;
        Eeprom.EepromData.DemoSettings.PacketParam5 = Eeprom.EepromData.PacketParams.Params.Gfsk.PayloadLength;
        Eeprom.EepromData.DemoSettings.PacketParam6 = Eeprom.EepromData.PacketParams.Params.Gfsk.CrcLength;
        Eeprom.EepromData.DemoSettings.PacketParam7 = Eeprom.EepromData.PacketParams.Params.Gfsk.Whitening;
    }

    Eeprom.EepromData.DemoSettings.ModulationType = modulation;
}

void EepromSetRangingDefaultSettings( void )
{
    printf("Set Ranging Default Settings\n\r");

    Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_RANGING;
    Eeprom.EepromData.ModulationParams.PacketType = PACKET_TYPE_RANGING;
    Eeprom.EepromData.PacketParams.PacketType     = PACKET_TYPE_RANGING;

    Eeprom.EepromData.DemoSettings.ModulationParam1 = LORA_SF6;
    Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_1600;
    Eeprom.EepromData.DemoSettings.ModulationParam3 = LORA_CR_4_5;

    Eeprom.EepromData.DemoSettings.PacketParam1    = 12; // PreambleLength
    Eeprom.EepromData.DemoSettings.PacketParam2    = LORA_PACKET_VARIABLE_LENGTH;
    Eeprom.EepromData.DemoSettings.PacketParam3    = 10; // PayloadLength
    Eeprom.EepromData.DemoSettings.PacketParam4    = LORA_CRC_ON;
    Eeprom.EepromData.DemoSettings.PacketParam5    = LORA_IQ_NORMAL;
    Eeprom.EepromData.DemoSettings.RngRequestCount = 60;
    Eeprom.EepromData.DemoSettings.RngAntenna      = DEMO_RNG_ANT_1;

    EepromSaveSettings( RADIO_RANGING_PARAMS );
}

void EepromSetDefaultSettings( void )
{
    Eeprom.EepromData.MenuSettings.ScreenCalibrated = false;

    printf("Set Default Settings\n\r");
    EepromSaveSettings( SCREEN_DATA );

    Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_RANGING;
    Eeprom.EepromData.ModulationParams.PacketType = PACKET_TYPE_RANGING;
    Eeprom.EepromData.PacketParams.PacketType     = PACKET_TYPE_RANGING;

    Eeprom.EepromData.DemoSettings.ModulationParam1 = LORA_SF6;
    Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_1600;
    Eeprom.EepromData.DemoSettings.ModulationParam3 = LORA_CR_4_5;

    Eeprom.EepromData.DemoSettings.PacketParam1    = 12; // PreambleLength
    Eeprom.EepromData.DemoSettings.PacketParam2    = LORA_PACKET_VARIABLE_LENGTH;
    Eeprom.EepromData.DemoSettings.PacketParam3    = 10; // PayloadLength
    Eeprom.EepromData.DemoSettings.PacketParam4    = LORA_CRC_ON;
    Eeprom.EepromData.DemoSettings.PacketParam5    = LORA_IQ_NORMAL;
    Eeprom.EepromData.DemoSettings.RngRequestCount = 60;
    Eeprom.EepromData.DemoSettings.RngFullScale    = 30;
    Eeprom.EepromData.DemoSettings.RngAddress      = DEMO_RNG_ADDR_1;
    Eeprom.EepromData.DemoSettings.RngAntenna      = DEMO_RNG_ANT_1;
    Eeprom.EepromData.DemoSettings.RngUnit         = DEMO_RNG_UNIT_SEL_M;

    EepromSaveSettings( RADIO_RANGING_PARAMS );

    Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_LORA;
    Eeprom.EepromData.ModulationParams.PacketType = PACKET_TYPE_LORA;
    Eeprom.EepromData.PacketParams.PacketType     = PACKET_TYPE_LORA;

    Eeprom.EepromData.DemoSettings.ModulationParam1 = LORA_SF10;
    Eeprom.EepromData.DemoSettings.ModulationParam2 = LORA_BW_1600;
    Eeprom.EepromData.DemoSettings.ModulationParam3 = LORA_CR_4_5;

    Eeprom.EepromData.DemoSettings.PacketParam1 = 12; // PreambleLength
    Eeprom.EepromData.DemoSettings.PacketParam2 = LORA_PACKET_VARIABLE_LENGTH;
    Eeprom.EepromData.DemoSettings.PacketParam3 = DEMO_MIN_PAYLOAD;
    Eeprom.EepromData.DemoSettings.PacketParam4 = LORA_CRC_ON;
    Eeprom.EepromData.DemoSettings.PacketParam5 = LORA_IQ_NORMAL;

    EepromSaveSettings( RADIO_LORA_PARAMS );

    Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_GFSK;
    Eeprom.EepromData.ModulationParams.PacketType = PACKET_TYPE_GFSK;
    Eeprom.EepromData.PacketParams.PacketType     = PACKET_TYPE_GFSK;

    Eeprom.EepromData.DemoSettings.ModulationParam1 = GFSK_BLE_BR_0_125_BW_0_3;
    Eeprom.EepromData.DemoSettings.ModulationParam2 = GFSK_BLE_MOD_IND_1_00;
    Eeprom.EepromData.DemoSettings.ModulationParam3 = RADIO_MOD_SHAPING_BT_1_0;

    Eeprom.EepromData.DemoSettings.PacketParam1 = PREAMBLE_LENGTH_32_BITS;
    Eeprom.EepromData.DemoSettings.PacketParam2 = GFSK_SYNCWORD_LENGTH_5_BYTE;
    Eeprom.EepromData.DemoSettings.PacketParam3 = RADIO_RX_MATCH_SYNCWORD_1;
    Eeprom.EepromData.DemoSettings.PacketParam4 = RADIO_PACKET_VARIABLE_LENGTH;
    Eeprom.EepromData.DemoSettings.PacketParam5 = DEMO_MIN_PAYLOAD;
    Eeprom.EepromData.DemoSettings.PacketParam6 = RADIO_CRC_3_BYTES;
    Eeprom.EepromData.DemoSettings.PacketParam7 = RADIO_WHITENING_ON;

    EepromSaveSettings( RADIO_GFSK_PARAMS );

    Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_FLRC;
    Eeprom.EepromData.ModulationParams.PacketType = PACKET_TYPE_FLRC;
    Eeprom.EepromData.PacketParams.PacketType     = PACKET_TYPE_FLRC;

    Eeprom.EepromData.DemoSettings.ModulationParam1 = FLRC_BR_0_260_BW_0_3;
    Eeprom.EepromData.DemoSettings.ModulationParam2 = FLRC_CR_1_2;
    Eeprom.EepromData.DemoSettings.ModulationParam3 = RADIO_MOD_SHAPING_BT_1_0;

    Eeprom.EepromData.DemoSettings.PacketParam1 = PREAMBLE_LENGTH_32_BITS;
    Eeprom.EepromData.DemoSettings.PacketParam2 = FLRC_SYNCWORD_LENGTH_4_BYTE;
    Eeprom.EepromData.DemoSettings.PacketParam3 = RADIO_RX_MATCH_SYNCWORD_1;
    Eeprom.EepromData.DemoSettings.PacketParam4 = RADIO_PACKET_VARIABLE_LENGTH;
    Eeprom.EepromData.DemoSettings.PacketParam5 = DEMO_MIN_PAYLOAD;
    Eeprom.EepromData.DemoSettings.PacketParam6 = RADIO_CRC_3_BYTES;
    Eeprom.EepromData.DemoSettings.PacketParam7 = RADIO_WHITENING_OFF;

    EepromSaveSettings( RADIO_FLRC_PARAMS );

    Eeprom.EepromData.DemoSettings.Entity         = SLAVE;
    Eeprom.EepromData.DemoSettings.AntennaSwitch  = 0x00;
    Eeprom.EepromData.DemoSettings.RadioPowerMode = USE_DCDC;
    Eeprom.EepromData.DemoSettings.Frequency      = DEMO_CENTRAL_FREQ_PRESET1;
    Eeprom.EepromData.DemoSettings.TxPower        = DEMO_POWER_TX_MAX;
    Eeprom.EepromData.DemoSettings.MaxNumPacket   = 0x00; // infinite
    Eeprom.EepromData.DemoSettings.ModulationType = PACKET_TYPE_LORA;

    EepromSaveSettings( DEMO_SETTINGS );
}

/*!
 * \brief Erase a page of Flash. Here used to Erase EEPROM region.
 *
 * \param [in]  page          address of page to erase
 * \param [in]  banks         address of banks to erase
 */
void FlashPageErase( uint32_t page, uint32_t banks )
{
    // Check the parameters
    assert_param( IS_FLASH_PAGE( page ) );
    assert_param( IS_FLASH_BANK_EXCLUSIVE( banks ) );

    if( ( banks & FLASH_BANK_1 ) != RESET )
    {
        CLEAR_BIT( FLASH->CR, FLASH_CR_BKER );
    }
    else
    {
        SET_BIT( FLASH->CR, FLASH_CR_BKER );
    }

    // Proceed to erase the page
    MODIFY_REG( FLASH->CR, FLASH_CR_PNB, ( page << 3 ) );
    SET_BIT( FLASH->CR, FLASH_CR_PER );
    SET_BIT( FLASH->CR, FLASH_CR_STRT );
}

/*!
 * \brief Write Eeprom to emulated EEPROM (in fact in Flash " higher address).
 *
 * \param [in]  addr          address of data (EEPROM offset not to be include)
 * \param [in]  buffer        buffer to use for copy
 * \param [in]  size          size of data to copy
 *
 * \retval      status        Status of operation (SUCCESS, ..)
 */
uint8_t EepromMcuWriteBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    uint64_t *flash = ( uint64_t* )buffer;

    // assert_param( addr >= DATA_EEPROM_BASE );
    assert_param( buffer != NULL );
    assert_param( size < ( DATA_EEPROM_END - DATA_EEPROM_BASE ) );

    HAL_FLASH_Unlock( );

    FlashPageErase( 255, 1 );

    WRITE_REG( FLASH->CR, 0x40000000 );

    for( uint32_t i = 0; i < size; i++ )
    {
        HAL_FLASH_Program( FLASH_TYPEPROGRAM_DOUBLEWORD, DATA_EEPROM_BASE + \
                           ( 8 * i ), flash[i] );
    }

    HAL_FLASH_Lock( );

    return SUCCESS;
}

uint8_t EepromMcuReadBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    assert_param( buffer != NULL );

    // assert_param( addr >= DATA_EEPROM_BASE );
    assert_param( buffer != NULL );
    assert_param( size < ( DATA_EEPROM_END - DATA_EEPROM_BASE ) );

    memcpy( buffer, ( uint8_t* )DATA_EEPROM_BASE, size );
    return SUCCESS;
}

static MemTestStruct_t EepromDataCheckSum( void )
{
    MemTestStruct_t memTestStruct;
    uint8_t x;
    uint8_t i;
    uint16_t crcBuf;
    memTestStruct.Value = 0xFFFF;

    for( i = 0; i < EEPROM_BUFFER_SIZE - sizeof( uint16_t ); i++ )
    {
        x = memTestStruct.Value >> 8 ^ Eeprom.Buffer[i];
        x ^= x >> 4;
        memTestStruct.Value = ( memTestStruct.Value << 8 ) ^ \
                              ( ( uint16_t )( x << 12 ) ) ^ \
                              ( ( uint16_t )( x << 5 ) ) ^ \
                              ( ( uint16_t )x );
    }
    memcpy( &crcBuf, Eeprom.Buffer + EEPROM_CRC_EEPROM_ADDR, 2 );
    memTestStruct.Valid = ( crcBuf == memTestStruct.Value );

    return memTestStruct;
}

void EepromFactoryReset( void )
{
    EepromSetDefaultSettings( );
    EepromLoadSettings( ( RadioPacketTypes_t )Eeprom.EepromData.DemoSettings.ModulationType );
}
