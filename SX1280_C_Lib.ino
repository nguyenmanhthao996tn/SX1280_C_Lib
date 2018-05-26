#include "Sx1280Hal.h"

#define RF_FREQUENCY 2444000000ul
#define MASTER

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(void);

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(void);

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout(void);

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout(void);

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError(IrqErrorCode_t);

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRangingDone(IrqRangingCode_t);

/*!
 * \brief All the callbacks are stored in a structure
 */
RadioCallbacks_t Callbacks =
    {
        &OnTxDone,      // txDone
        &OnRxDone,      // rxDone
        NULL,           // syncWordDone
        NULL,           // headerDone
        &OnTxTimeout,   // txTimeout
        &OnRxTimeout,   // rxTimeout
        &OnRxError,     // rxError
        &OnRangingDone, // rangingDone
        NULL,           // cadDone
};

/*!
 * \brief Define IO and callbacks for radio
 * mosi, miso, sclk, nss, busy, dio1, dio2, dio3, rst, callbacks
 */
SX1280Hal Radio(10, 5, 2, 3, 4, 6, &Callbacks);

void DioInterruptISR(void);

void setup()
{
  Serial.begin(9600);
  Serial.println("Arduino SX1280 PingPong demo");

  // put your setup code here, to run once:
  attachInterrupt(digitalPinToInterrupt(2), DioInterruptISR, RISING);
  attachInterrupt(digitalPinToInterrupt(3), DioInterruptISR, RISING);

  // SX1280 Init
  Radio.Init();
  Radio.SetRegulatorMode(USE_LDO);
  Radio.SetStandby(STDBY_RC);
  Radio.SetPacketType(PACKET_TYPE_LORA);
  Radio.SetRfFrequency(RF_FREQUENCY);
  Radio.SetBufferBaseAddresses(0x00, 0x00);

  ModulationParams_t ModulationParams;
  ModulationParams.PacketType = PACKET_TYPE_LORA;
  ModulationParams.Params.LoRa.SpreadingFactor = LORA_SF5;
  ModulationParams.Params.LoRa.Bandwidth = LORA_BW_1600;
  ModulationParams.Params.LoRa.CodingRate = LORA_CR_4_5;
  Radio.SetModulationParams(&ModulationParams);
  // Radio.WriteRegister(0x925, 0x1E);

  PacketParams_t PacketParams;
  PacketParams.PacketType = PACKET_TYPE_LORA;
  PacketParams.Params.LoRa.PreambleLength = 0x0C; // 12 bit
  PacketParams.Params.LoRa.HeaderType = LORA_PACKET_IMPLICIT;
  PacketParams.Params.LoRa.PayloadLength = 128;
  PacketParams.Params.LoRa.Crc = LORA_CRC_ON;
  PacketParams.Params.LoRa.InvertIQ = LORA_IQ_NORMAL;
  Radio.SetPacketParams(&PacketParams);
  Serial.println("SX1280 Initialized");

#ifdef MASTER
  // Init for Tx
#else
  // Init for Rx
#endif
}

void loop()
{
  // put your main code here, to run repeatedly:
}

// ************************     Radio Callbacks     ****************************
// *                                                                           *
// * These functions are called through function pointer by the Radio low      *
// * level drivers                                                             *
// *                                                                           *
// *****************************************************************************
void OnTxDone(void)
{
  // DemoInternalState = APP_TX;
}

void OnRxDone(void)
{
  // DemoInternalState = APP_RX;
}

void OnTxTimeout(void)
{
  // DemoInternalState = APP_TX_TIMEOUT;
}

void OnRxTimeout(void)
{
  // DemoInternalState = APP_RX_TIMEOUT;
}

void OnRxError(IrqErrorCode_t errorCode)
{
  // DemoInternalState = APP_RX_ERROR;
}

void OnRangingDone(IrqRangingCode_t val)
{
  // if( val == IRQ_RANGING_MASTER_VALID_CODE || val == IRQ_RANGING_SLAVE_VALID_CODE )
  // {
  //     DemoInternalState = APP_RANGING_DONE;
  // }
  // else if( val == IRQ_RANGING_MASTER_ERROR_CODE || val == IRQ_RANGING_SLAVE_ERROR_CODE )
  // {
  //     DemoInternalState = APP_RANGING_TIMEOUT;
  // }
  // else
  // {
  //     DemoInternalState = APP_RANGING_TIMEOUT;
  // }
}

void OnCadDone(bool channelActivityDetected)
{
}

void DioInterruptISR(void)
{
  Radio.OnDioIrq();
}
