/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Display driver header

Maintainer: Gregory Cristian & Gilbert Menth
*/

#ifndef DEMO_APPLICATION_H
#define DEMO_APPLICATION_H


/*!
 * \brief Used to display firmware version on TFT (Utilities menu)
 */
#define FIRMWARE_VERSION    ( ( char* )"Firmware Version: 170818" )

/*!
 * \brief Define range of central frequency [Hz]
 */
#define DEMO_CENTRAL_FREQ_MIN       2400000000UL
#define DEMO_CENTRAL_FREQ_MAX       2483500000UL

/*!
 * \brief Define 3 preset central frequencies [Hz]
 */
#define DEMO_CENTRAL_FREQ_PRESET1   2402000000UL
#define DEMO_CENTRAL_FREQ_PRESET2   2450000000UL
#define DEMO_CENTRAL_FREQ_PRESET3   2480000000UL

/*!
 * \brief Define 5 preset ranging addresses
 */
#define DEMO_RNG_ADDR_1             0x10000000
#define DEMO_RNG_ADDR_2             0x32100000
#define DEMO_RNG_ADDR_3             0x20012301
#define DEMO_RNG_ADDR_4             0x20000abc
#define DEMO_RNG_ADDR_5             0x32101230

/*!
 * \brief Define antenna selection for ranging
 */
#define DEMO_RNG_ANT_1              1
#define DEMO_RNG_ANT_2              2
#define DEMO_RNG_ANT_BOTH           0

/*!
 * \brief Define units for ranging distances
 */
#define DEMO_RNG_UNIT_CONV_M        1.0 // not used
#define DEMO_RNG_UNIT_CONV_YD       1.0936
#define DEMO_RNG_UNIT_CONV_MI       6.2137e-4
#define DEMO_RNG_UNIT_SEL_M         0
#define DEMO_RNG_UNIT_SEL_YD        1
#define DEMO_RNG_UNIT_SEL_MI        2

/*!
 * \brief Define min and max Tx power [dBm]
 */
#define DEMO_POWER_TX_MIN           -18
#define DEMO_POWER_TX_MAX           13

/*!
 * \brief Define min and max ranging channels count
 */
const uint16_t DEMO_RNG_CHANNELS_COUNT_MAX = 255;
const uint16_t DEMO_RNG_CHANNELS_COUNT_MIN = 10;

/*!
 * \brief Define min and max Z Score for ranging filtered results
 */
#define DEMO_RNG_ZSCORE_MIN         1
#define DEMO_RNG_ZSCORE_MAX         5

/*!
 * \brief Define min and max payload length for demo applications
 */
#define DEMO_MIN_PAYLOAD            12
#define DEMO_FLRC_MAX_PAYLOAD       127
#define DEMO_GFS_LORA_MAX_PAYLOAD   255


/*!
 * \brief Define current demo mode
 */
enum DemoMode
{
    MASTER = 0,
    SLAVE
};

/*!
 * \brief Status of ranging distance
 */
enum RangingStatus
{
    RNG_INIT = 0,
    RNG_PROCESS,
    RNG_VALID,
    RNG_TIMEOUT,
    RNG_PER_ERROR
};

/*!
 * \brief List of states for demo state machine
 */
enum DemoInternalStates
{
    APP_IDLE = 0,               // nothing to do (or wait a radio interrupt)
    APP_RANGING_DONE,
    APP_RANGING_TIMEOUT,
    APP_RANGING_CONFIG,
    APP_RNG,
    SEND_PING_MSG,
    SEND_PONG_MSG,
    APP_RX,                     // Rx done
    APP_RX_TIMEOUT,             // Rx timeout
    APP_RX_ERROR,               // Rx error
    APP_TX,                     // Tx done
    APP_TX_TIMEOUT,             // Tx error
    PER_TX_START,               // PER master
    PER_RX_START                // PER slave
};

/*!
 * \brief Demo Settings structure of Eeprom structure
 */
typedef struct
{
    uint8_t Entity;              // Master or Slave
    uint8_t HoldDemo;            // Put demo in hold status
    uint8_t AntennaSwitch;       // Witch antenna connected
    uint32_t Frequency;          // Demo frequency
    int8_t TxPower;              // Demo Tx power
    uint8_t RadioPowerMode;      // Radio Power Mode [0: LDO, 1:DC_DC]
    uint8_t PayloadLength;       // Demo payload length
    uint8_t ModulationType;      // Demo modulation type (LORA, GFSK, FLRC)
    uint8_t ModulationParam1;    // Demo Mod. Param1 (depend on modulation type)
    uint8_t ModulationParam2;    // Demo Mod. Param2 (depend on modulation type)
    uint8_t ModulationParam3;    // Demo Mod. Param3 (depend on modulation type)
    uint8_t PacketParam1;        // Demo Pack. Param1 (depend on packet type)
    uint8_t PacketParam2;        // Demo Pack. Param2 (depend on packet type)
    uint8_t PacketParam3;        // Demo Pack. Param3 (depend on packet type)
    uint8_t PacketParam4;        // Demo Pack. Param4 (depend on packet type)
    uint8_t PacketParam5;        // Demo Pack. Param5 (depend on packet type)
    uint8_t PacketParam6;        // Demo Pack. Param6 (depend on packet type)
    uint8_t PacketParam7;        // Demo Pack. Param7 (depend on packet type)
    uint32_t MaxNumPacket;       // Demo Max Num Packet for PingPong and PER
    uint16_t InterPacketDelay;   // Demo Inter-Packet Delay for PingPong and PER
    uint32_t CntPacketTx;        // Tx packet transmitted
    uint32_t CntPacketRxOK;      // Rx packet received OK
    uint32_t CntPacketRxOKSlave; // Rx packet received OK (slave side)
    uint32_t CntPacketRxKO;      // Rx packet received KO
    uint32_t CntPacketRxKOSlave; // Rx packet received KO (slave side)
    uint16_t RxTimeOutCount;     // Rx packet received KO (by timeout)
    double RngDistance;          // Distance measured by ranging demo
    double RngRawDistance;       // Uncorrected measured distance [m]
    uint32_t RngAddress;         // Ranging Address
    uint16_t RngFullScale;       // Full range of measuring distance (Ranging)
    uint8_t RngRequestCount;     // Ranging Request Count
    uint8_t RngUnit;             // Ranging distance unit [m]/[mi]
    uint8_t RngStatus;           // Status of ranging distance
    double RngFei;               // Ranging Frequency Error Indicator
    uint8_t RngAntenna;          // Ranging antenna selection
    double RngFeiFactor;         // Ranging frequency correction factor
    uint16_t RngReqDelay;        // Time between ranging request
    uint16_t RngCalib;           // Ranging Calibration
    uint8_t RFU;                 // -------------------------
    int8_t RssiValue;            // Demo Rssi Value
    int8_t SnrValue;             // Demo Snr Value (only for LORA mod. type)
}DemoSettings_t;

/*!
 * \brief Define freq offset for config central freq in "Radio Config Freq" menu
 */
enum FreqBase
{
    FB1     = 1,            //   1 Hz
    FB10    = 10,           //  10 Hz
    FB100   = 100,          // 100 Hz
    FB1K    = 1000,         //   1 kHz
    FB10K   = 10000,        //  10 kHz
    FB100K  = 100000,       // 100 kHz
    FB1M    = 1000000,      //   1 MHz
    FB10M   = 10000000      //  10 MHz
};


/*!
 * \brief Init RAM copy of Eeprom structure and init radio with it.
 */
void InitDemoApplication( void );

/*!
 * \brief Init vars of demo and fix APP_IDLE state to demo state machine.
 */
void StopDemoApplication( void );

/*!
 * \brief Run Demo in sleep mode.
 *
 * \retval      demoStatusUpdate    page refresh status ( >0 : refresh)
 */
uint8_t RunDemoSleepMode( void );

/*!
 * \brief Run Demo in standby RC mode.
 *
 * \retval      demoStatusUpdate    page refresh status ( >0 : refresh)
 */
uint8_t RunDemoStandbyRcMode( void );

/*!
 * \brief Run Demo in standby XOSC mode.
 *
 * \retval      demoStatusUpdate    page refresh status ( >0 : refresh)
 */
uint8_t RunDemoStandbyXoscMode( void );

/*!
 * \brief Run Demo Tx in continuous mode without modulation.
 *
 * \retval      demoStatusUpdate    page refresh status ( >0 : refresh)
 */
uint8_t RunDemoTxCw( void );

/*!
 * \brief Run Demo Tx in continuous modulation.
 *
 * \retval      demoStatusUpdate    page refresh status ( >0 : refresh)
 */
uint8_t RunDemoTxContinuousModulation( void );

/*!
 * \brief Run demo PingPong.
 *
 * \retval      demoStatusUpdate    page refresh status ( >0 : refresh)
 */
uint8_t RunDemoApplicationPingPong( void );

/*!
 * \brief Compute payload of Rx frame and update current counts and indicators.
 *
 * \param [in]  buffer        buffer with frame to compute
 * \param [in]  buffersize    size of frame data in the buffer
 */
void ComputePingPongPayload( uint8_t *buffer, uint8_t bufferSize );

/*!
 * \brief Run demo PER.
 *
 * \retval      demoStatusUpdate    page refresh status ( >0 : refresh)
 */
uint8_t RunDemoApplicationPer( void );

/*!
 * \brief Compute payload of Rx frame and update current counts and indicators.
 *
 * \param [in]  buffer        buffer with frame to compute
 * \param [in]  buffersize    size of frame data in the buffer
 */
void ComputePerPayload( uint8_t *buffer, uint8_t bufferSize );

/*!
 * \brief Run ranging demo.
 *
 * \retval      demoStatusUpdate    page refresh status ( >0 : refresh)
 */
uint8_t RunDemoApplicationRanging( void );

#endif // DEMO_APPLICATION_H
