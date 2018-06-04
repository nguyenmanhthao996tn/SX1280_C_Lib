#include "Radio_Methods.h"
#include "Arduino.h"
#include "SPI.h"

static RadioCallbacks_t *__callbacks = NULL;
static bool __IrqState = false;
static bool __PollingMode = false;
static RadioOperatingModes_t __OperatingMode = MODE_STDBY_RC;
static RadioPacketTypes_t __PacketType = PACKET_TYPE_NONE;
static RadioLoRaBandwidths_t __LoRaBandwidth = LORA_BW_1600;
/*!
   \brief Radio registers definition

*/
typedef struct
{
  uint16_t      Addr;                             //!< The address of the register
  uint8_t       Value;                            //!< The value of the register
} RadioRegisters_t;

/*!
   \brief Radio hardware registers initialization definition
*/
#define RADIO_INIT_REGISTERS_VALUE  { }

/*!
   \brief Radio hardware registers initialization
*/
const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

void GPIO_Init(void)
{
  pinMode(NSS, OUTPUT);
  digitalWrite(NSS, HIGH);

  pinMode(NRESET, OUTPUT);
  digitalWrite(NRESET, HIGH);

  pinMode(BUSY, INPUT);
}

void SPI_Init(void)
{
  SPI.begin();
}

void IoIrqInit(void)
{
  pinMode(DIO1, INPUT);
  attachInterrupt(digitalPinToInterrupt(DIO1), __ProcessIrqs, RISING);
}

void WaitOnBusy(void)
{
  while (digitalRead(BUSY) == HIGH) {}
}

void __Init(RadioCallbacks_t* callbacks)
{
  __callbacks = callbacks;

  // GPIO Init
  GPIO_Init();

  // Reset
  __Reset();

  // SPI Init
  SPI_Init();

  // IoIrqInit
  IoIrqInit();

  // Wakeup
  __Wakeup();

  // SetRegistersDefault
  __SetRegistersDefault();
}

void __SetPollingMode(void)
{
  __PollingMode = true;
}

void __SetInterruptMode(void)
{
  __PollingMode = false;
}

void __SetRegistersDefault(void)
{
  for ( int16_t i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
  {
    uint8_t value = RadioRegsInit[i].Value;
    __WriteRegister( RadioRegsInit[i].Addr, &(value), 1 );
  }
}

uint16_t __GetFirmwareVersion(void)
{
  uint8_t highByte = 0,
          lowByte = 0;

  __ReadRegister(REG_LR_FIRMWARE_VERSION_MSB, &highByte, 1);
  __ReadRegister(REG_LR_FIRMWARE_VERSION_MSB + 1, &lowByte, 1);

  return ( ( highByte << 8 ) | ( lowByte ) );
}

void __Reset(void)
{
  // __disable_irq( );
  //  delay(20);
  digitalWrite(NRESET, LOW);
  //  delay(50);
  digitalWrite(NRESET, HIGH);
  //  delay(20);
  // __enable_irq( );
}

void __Wakeup(void)
{
  digitalWrite(NSS, LOW);    // RadioNss = 0;
  SPI.transfer(RADIO_GET_STATUS); // RadioSpi->write(RADIO_GET_STATUS);
  SPI.transfer(0);                // RadioSpi->write(0);
  digitalWrite(NSS, HIGH);   // RadioNss = 1;

  WaitOnBusy();
}

void __WriteCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{
  WaitOnBusy();

  digitalWrite(NSS, LOW);    // RadioNss = 0;
  SPI.transfer((uint8_t)command); // RadioSpi->write((uint8_t)command);
  for (uint16_t i = 0; i < size; i++)
  {
    SPI.transfer(buffer[i]); // RadioSpi->write(buffer[i]);
  }
  digitalWrite(NSS, HIGH); // RadioNss = 1;

  if (command != RADIO_SET_SLEEP)
  {
    WaitOnBusy();
  }
}

void __ReadCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{
  WaitOnBusy();

  digitalWrite(NSS, LOW); // RadioNss = 0;
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
  digitalWrite(NSS, HIGH); // RadioNss = 1;

  WaitOnBusy();
}

void __WriteRegister(uint16_t address, uint8_t *buffer, uint16_t size)
{
  WaitOnBusy( );

  digitalWrite(NSS, LOW); // RadioNss = 0;
  SPI.transfer( RADIO_WRITE_REGISTER ); // RadioSpi->write( RADIO_WRITE_REGISTER );
  SPI.transfer( ( address & 0xFF00 ) >> 8 ); // RadioSpi->write( ( address & 0xFF00 ) >> 8 );
  SPI.transfer( address & 0x00FF );// RadioSpi->write( address & 0x00FF );
  for ( uint16_t i = 0; i < size; i++ )
  {
    SPI.transfer(buffer[i]);// RadioSpi->write( buffer[i] );
  }
  digitalWrite(NSS, HIGH); // RadioNss = 1;

  WaitOnBusy( );
}

void __WriteRegister_1(uint16_t address, uint8_t *buffer)
{
  __WriteRegister(address, buffer, 1);
}

void __ReadRegister(uint16_t address, uint8_t *buffer, uint16_t size)
{
  WaitOnBusy( );

  digitalWrite(NSS, LOW); // RadioNss = 0;
  SPI.transfer( RADIO_READ_REGISTER ); // RadioSpi->write( RADIO_READ_REGISTER );
  SPI.transfer((address & 0xFF00 ) >> 8 ); // RadioSpi->write( ( address & 0xFF00 ) >> 8 );
  SPI.transfer( address & 0x00FF ); // RadioSpi->write( address & 0x00FF );
  SPI.transfer( 0 ); // RadioSpi->write( 0 );
  for ( uint16_t i = 0; i < size; i++ )
  {
    buffer[i] = SPI.transfer( 0 );// buffer[i] = RadioSpi->write( 0 );
  }
  digitalWrite(NSS, HIGH); // RadioNss = 1;

  WaitOnBusy( );
}

uint8_t __ReadRegister_1(uint16_t address)
{
  uint8_t reg = 0;
  __ReadRegister(address, &reg, 1);
  return reg;
}

void __WriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
  WaitOnBusy( );

  digitalWrite(NSS, LOW); // RadioNss = 0;
  SPI.transfer(RADIO_WRITE_BUFFER); // RadioSpi->write( RADIO_WRITE_BUFFER );
  SPI.transfer(offset); // RadioSpi->write( offset );
  for ( uint16_t i = 0; i < size; i++ )
  {
    SPI.transfer(buffer[i]);// RadioSpi->write( buffer[i] );
  }
  digitalWrite(NSS, HIGH); // RadioNss = 1;

  WaitOnBusy( );
}

void __ReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
  WaitOnBusy( );

  digitalWrite(NSS, LOW); // RadioNss = 0;
  SPI.transfer(RADIO_READ_BUFFER);// RadioSpi->write( RADIO_READ_BUFFER );
  SPI.transfer(offset);// RadioSpi->write( offset );
  SPI.transfer(0);// RadioSpi->write( 0 );
  for ( uint16_t i = 0; i < size; i++ )
  {
    buffer[i] = SPI.transfer(0); // buffer[i] = RadioSpi->write( 0 );
  }
  digitalWrite(NSS, HIGH); // RadioNss = 1;

  WaitOnBusy( );
}

uint8_t __GetDioStatus(void)
{
  uint8_t result = 0;
  result = (digitalRead(DIO3) << 3) | (digitalRead(DIO2) << 2) | (digitalRead(DIO1) << 1) | (digitalRead(BUSY) << 0);
  return result;
}

RadioOperatingModes_t __GetOpMode(void)
{
  return ( __OperatingMode );
}

RadioStatus_t __GetStatus(void)
{
  uint8_t stat = 0;
  RadioStatus_t status;

  __ReadCommand( RADIO_GET_STATUS, ( uint8_t * )&stat, 1 );
  status.Value = stat;
  return ( status );
}

void __SetSleep(SleepParams_t sleepConfig)
{
  uint8_t sleep = ( sleepConfig.WakeUpRTC << 3 ) |
                  ( sleepConfig.InstructionRamRetention << 2 ) |
                  ( sleepConfig.DataBufferRetention << 1 ) |
                  ( sleepConfig.DataRamRetention );

  __OperatingMode = MODE_SLEEP;
  __WriteCommand( RADIO_SET_SLEEP, &sleep, 1 );
}

void __SetStandby(RadioStandbyModes_t standbyConfig)
{
  __WriteCommand( RADIO_SET_STANDBY, ( uint8_t* )&standbyConfig, 1 );
  if ( standbyConfig == STDBY_RC )
  {
    __OperatingMode = MODE_STDBY_RC;
  }
  else
  {
    __OperatingMode = MODE_STDBY_XOSC;
  }
}

void __SetFs(void)
{
  __WriteCommand( RADIO_SET_FS, 0, 0 );
  __OperatingMode = MODE_FS;
}

/*!
      \brief Set the role of the radio during ranging operations

      \param [in]  role          Role of the radio
*/
void __SetRangingRole( RadioRangingRoles_t role )
{
  uint8_t buf[1];

  buf[0] = role;
  __WriteCommand( RADIO_SET_RANGING_ROLE, &buf[0], 1 );
}

void __SetTx(TickTime_t timeout)
{
  uint8_t buf[3];
  buf[0] = timeout.PeriodBase;
  buf[1] = ( uint8_t )( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
  buf[2] = ( uint8_t )( timeout.PeriodBaseCount & 0x00FF );

  __ClearIrqStatus( IRQ_RADIO_ALL );

  // If the radio is doing ranging operations, then apply the specific calls
  // prior to SetTx
  if ( __GetPacketType( true ) == PACKET_TYPE_RANGING )
  {
    __SetRangingRole( RADIO_RANGING_ROLE_MASTER );
  }
  __WriteCommand( RADIO_SET_TX, buf, 3 );
  __OperatingMode = MODE_TX;
}

void __SetRx(TickTime_t timeout)
{
  uint8_t buf[3];
  buf[0] = timeout.PeriodBase;
  buf[1] = ( uint8_t )( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
  buf[2] = ( uint8_t )( timeout.PeriodBaseCount & 0x00FF );

  __ClearIrqStatus( IRQ_RADIO_ALL );

  // If the radio is doing ranging operations, then apply the specific calls
  // prior to SetRx
  if ( __GetPacketType( true ) == PACKET_TYPE_RANGING )
  {
    __SetRangingRole( RADIO_RANGING_ROLE_SLAVE );
  }
  __WriteCommand( RADIO_SET_RX, buf, 3 );
  __OperatingMode = MODE_RX;
}

void __SetRxDutyCycle(RadioTickSizes_t periodBase, uint16_t periodBaseCountRx, uint16_t periodBaseCountSleep)
{
  uint8_t buf[5];

  buf[0] = periodBase;
  buf[1] = ( uint8_t )( ( periodBaseCountRx >> 8 ) & 0x00FF );
  buf[2] = ( uint8_t )( periodBaseCountRx & 0x00FF );
  buf[3] = ( uint8_t )( ( periodBaseCountSleep >> 8 ) & 0x00FF );
  buf[4] = ( uint8_t )( periodBaseCountSleep & 0x00FF );
  __WriteCommand( RADIO_SET_RXDUTYCYCLE, buf, 5 );
  __OperatingMode = MODE_RX;
}

void __SetCad(void)
{
  __WriteCommand( RADIO_SET_CAD, 0, 0 );
  __OperatingMode = MODE_CAD;
}

void __SetTxContinuousWave(void)
{
  __WriteCommand( RADIO_SET_TXCONTINUOUSWAVE, 0, 0 );
}

void __SetTxContinuousPreamble(void)
{
  __WriteCommand( RADIO_SET_TXCONTINUOUSPREAMBLE, 0, 0 );
}

void __SetPacketType(RadioPacketTypes_t packetType)
{
  // Save packet type internally to avoid questioning the radio
  __PacketType = packetType;

  __WriteCommand( RADIO_SET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
}

RadioPacketTypes_t __GetPacketType(bool returnLocalCopy)
{
  RadioPacketTypes_t packetType = PACKET_TYPE_NONE;
  if ( returnLocalCopy == false )
  {
    __ReadCommand( RADIO_GET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
    if ( __PacketType != packetType )
    {
      __PacketType = packetType;
    }
  }
  else
  {
    packetType = __PacketType;
  }
  return packetType;
}

void __SetRfFrequency(uint32_t rfFrequency)
{
  uint8_t buf[3];
  uint32_t freq = 0;

  freq = ( uint32_t )( ( double )rfFrequency / ( double )FREQ_STEP );
  buf[0] = ( uint8_t )( ( freq >> 16 ) & 0xFF );
  buf[1] = ( uint8_t )( ( freq >> 8 ) & 0xFF );
  buf[2] = ( uint8_t )( freq & 0xFF );
  __WriteCommand( RADIO_SET_RFFREQUENCY, buf, 3 );
}

void __SetTxParams(int8_t power, RadioRampTimes_t rampTime)
{
  uint8_t buf[2];

  // The power value to send on SPI/UART is in the range [0..31] and the
  // physical output power is in the range [-18..13]dBm
  buf[0] = power + 18;
  buf[1] = ( uint8_t )rampTime;
  __WriteCommand( RADIO_SET_TXPARAMS, buf, 2 );
}

void __SetCadParams(RadioLoRaCadSymbols_t cadSymbolNum)
{
  __WriteCommand( RADIO_SET_CADPARAMS, ( uint8_t* )&cadSymbolNum, 1 );
  __OperatingMode = MODE_CAD;
}

void __SetBufferBaseAddresses(uint8_t txBaseAddress, uint8_t rxBaseAddress)
{
  uint8_t buf[2];

  buf[0] = txBaseAddress;
  buf[1] = rxBaseAddress;
  __WriteCommand( RADIO_SET_BUFFERBASEADDRESS, buf, 2 );
}

void __SetModulationParams(ModulationParams_t *modParams)
{
  uint8_t buf[3];

  // Check if required configuration corresponds to the stored packet type
  // If not, silently update radio packet type
  if ( __PacketType != modParams->PacketType )
  {
    __SetPacketType( modParams->PacketType );
  }

  switch ( modParams->PacketType )
  {
    case PACKET_TYPE_GFSK:
      buf[0] = modParams->Params.Gfsk.BitrateBandwidth;
      buf[1] = modParams->Params.Gfsk.ModulationIndex;
      buf[2] = modParams->Params.Gfsk.ModulationShaping;
      break;
    case PACKET_TYPE_LORA:
    case PACKET_TYPE_RANGING:
      buf[0] = modParams->Params.LoRa.SpreadingFactor;
      buf[1] = modParams->Params.LoRa.Bandwidth;
      buf[2] = modParams->Params.LoRa.CodingRate;
      __LoRaBandwidth = modParams->Params.LoRa.Bandwidth;
      break;
    case PACKET_TYPE_FLRC:
      buf[0] = modParams->Params.Flrc.BitrateBandwidth;
      buf[1] = modParams->Params.Flrc.CodingRate;
      buf[2] = modParams->Params.Flrc.ModulationShaping;
      break;
    case PACKET_TYPE_BLE:
      buf[0] = modParams->Params.Ble.BitrateBandwidth;
      buf[1] = modParams->Params.Ble.ModulationIndex;
      buf[2] = modParams->Params.Ble.ModulationShaping;
      break;
    case PACKET_TYPE_NONE:
      buf[0] = 0;
      buf[1] = 0;
      buf[2] = 0;
      break;
  }
  __WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, 3 );
}

void __SetPacketParams(PacketParams_t *packetParams)
{
  uint8_t buf[7];
  // Check if required configuration corresponds to the stored packet type
  // If not, silently update radio packet type
  if ( __PacketType != packetParams->PacketType )
  {
    __SetPacketType( packetParams->PacketType );
  }

  switch ( packetParams->PacketType )
  {
    case PACKET_TYPE_GFSK:
      buf[0] = packetParams->Params.Gfsk.PreambleLength;
      buf[1] = packetParams->Params.Gfsk.SyncWordLength;
      buf[2] = packetParams->Params.Gfsk.SyncWordMatch;
      buf[3] = packetParams->Params.Gfsk.HeaderType;
      buf[4] = packetParams->Params.Gfsk.PayloadLength;
      buf[5] = packetParams->Params.Gfsk.CrcLength;
      buf[6] = packetParams->Params.Gfsk.Whitening;
      break;
    case PACKET_TYPE_LORA:
    case PACKET_TYPE_RANGING:
      buf[0] = packetParams->Params.LoRa.PreambleLength;
      buf[1] = packetParams->Params.LoRa.HeaderType;
      buf[2] = packetParams->Params.LoRa.PayloadLength;
      buf[3] = packetParams->Params.LoRa.Crc;
      buf[4] = packetParams->Params.LoRa.InvertIQ;
      buf[5] = 0;
      buf[6] = 0;
      break;
    case PACKET_TYPE_FLRC:
      buf[0] = packetParams->Params.Flrc.PreambleLength;
      buf[1] = packetParams->Params.Flrc.SyncWordLength;
      buf[2] = packetParams->Params.Flrc.SyncWordMatch;
      buf[3] = packetParams->Params.Flrc.HeaderType;
      buf[4] = packetParams->Params.Flrc.PayloadLength;
      buf[5] = packetParams->Params.Flrc.CrcLength;
      buf[6] = packetParams->Params.Flrc.Whitening;
      break;
    case PACKET_TYPE_BLE:
      buf[0] = packetParams->Params.Ble.ConnectionState;
      buf[1] = packetParams->Params.Ble.CrcLength;
      buf[2] = packetParams->Params.Ble.BleTestPayload;
      buf[3] = packetParams->Params.Ble.Whitening;
      buf[4] = 0;
      buf[5] = 0;
      buf[6] = 0;
      break;
    case PACKET_TYPE_NONE:
      buf[0] = 0;
      buf[1] = 0;
      buf[2] = 0;
      buf[3] = 0;
      buf[4] = 0;
      buf[5] = 0;
      buf[6] = 0;
      break;
  }
  __WriteCommand( RADIO_SET_PACKETPARAMS, buf, 7 );
}

void __GetRxBufferStatus(uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer)
{
  uint8_t status[2];

  __ReadCommand( RADIO_GET_RXBUFFERSTATUS, status, 2 );

  // In case of LORA fixed header, the rxPayloadLength is obtained by reading
  // the register REG_LR_PAYLOADLENGTH
  if ( ( __GetPacketType( true ) == PACKET_TYPE_LORA ) && ( __ReadRegister_1( REG_LR_PACKETPARAMS ) >> 7 == 1 ) )
  {
    *rxPayloadLength = __ReadRegister_1( REG_LR_PAYLOADLENGTH );
  }
  else if ( __GetPacketType( true ) == PACKET_TYPE_BLE )
  {
    // In the case of BLE, the size returned in status[0] do not include the 2-byte length PDU header
    // so it is added there
    *rxPayloadLength = status[0] + 2;
  }
  else
  {
    *rxPayloadLength = status[0];
  }

  *rxStartBufferPointer = status[1];
}

void __GetPacketStatus(PacketStatus_t *packetStatus)
{
  uint8_t status[5];

  __ReadCommand( RADIO_GET_PACKETSTATUS, status, 5 );

  packetStatus->packetType = __GetPacketType( true );
  switch ( packetStatus->packetType )
  {
    case PACKET_TYPE_GFSK:
      packetStatus->Gfsk.RssiSync = -( status[1] / 2 );

      packetStatus->Gfsk.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
      packetStatus->Gfsk.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
      packetStatus->Gfsk.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
      packetStatus->Gfsk.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
      packetStatus->Gfsk.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
      packetStatus->Gfsk.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
      packetStatus->Gfsk.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

      packetStatus->Gfsk.TxRxStatus.RxNoAck = ( status[3] >> 5 ) & 0x01;
      packetStatus->Gfsk.TxRxStatus.PacketSent = status[3] & 0x01;

      packetStatus->Gfsk.SyncAddrStatus = status[4] & 0x07;
      break;

    case PACKET_TYPE_LORA:
    case PACKET_TYPE_RANGING:
      packetStatus->LoRa.RssiPkt = -( status[0] / 2 );
      ( status[1] < 128 ) ? ( packetStatus->LoRa.SnrPkt = status[1] / 4 ) : ( packetStatus->LoRa.SnrPkt = ( ( status[1] - 256 ) / 4 ) );
      break;

    case PACKET_TYPE_FLRC:
      packetStatus->Flrc.RssiSync = -( status[1] / 2 );

      packetStatus->Flrc.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
      packetStatus->Flrc.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
      packetStatus->Flrc.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
      packetStatus->Flrc.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
      packetStatus->Flrc.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
      packetStatus->Flrc.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
      packetStatus->Flrc.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

      packetStatus->Flrc.TxRxStatus.RxPid = ( status[3] >> 6 ) & 0x03;
      packetStatus->Flrc.TxRxStatus.RxNoAck = ( status[3] >> 5 ) & 0x01;
      packetStatus->Flrc.TxRxStatus.RxPidErr = ( status[3] >> 4 ) & 0x01;
      packetStatus->Flrc.TxRxStatus.PacketSent = status[3] & 0x01;

      packetStatus->Flrc.SyncAddrStatus = status[4] & 0x07;
      break;

    case PACKET_TYPE_BLE:
      packetStatus->Ble.RssiSync =  -( status[1] / 2 );

      packetStatus->Ble.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
      packetStatus->Ble.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
      packetStatus->Ble.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
      packetStatus->Ble.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
      packetStatus->Ble.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
      packetStatus->Ble.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
      packetStatus->Ble.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

      packetStatus->Ble.TxRxStatus.PacketSent = status[3] & 0x01;

      packetStatus->Ble.SyncAddrStatus = status[4] & 0x07;
      break;

    case PACKET_TYPE_NONE:
      // In that specific case, we set everything in the packetStatus to zeros
      // and reset the packet type accordingly
      memset( packetStatus, 0, sizeof( PacketStatus_t ) );
      packetStatus->packetType = PACKET_TYPE_NONE;
      break;
  }
}

int8_t __GetRssiInst(void)
{
  uint8_t raw = 0;

  __ReadCommand( RADIO_GET_RSSIINST, &raw, 1 );

  return ( int8_t ) ( -raw / 2 );
}

void __SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask)
{
  uint8_t buf[8];

  buf[0] = ( uint8_t )( ( irqMask >> 8 ) & 0x00FF );
  buf[1] = ( uint8_t )( irqMask & 0x00FF );
  buf[2] = ( uint8_t )( ( dio1Mask >> 8 ) & 0x00FF );
  buf[3] = ( uint8_t )( dio1Mask & 0x00FF );
  buf[4] = ( uint8_t )( ( dio2Mask >> 8 ) & 0x00FF );
  buf[5] = ( uint8_t )( dio2Mask & 0x00FF );
  buf[6] = ( uint8_t )( ( dio3Mask >> 8 ) & 0x00FF );
  buf[7] = ( uint8_t )( dio3Mask & 0x00FF );
  __WriteCommand( RADIO_SET_DIOIRQPARAMS, buf, 8 );
}

uint16_t __GetIrqStatus(void)
{
  uint8_t irqStatus[2];
  __ReadCommand( RADIO_GET_IRQSTATUS, irqStatus, 2 );
  return ( irqStatus[0] << 8 ) | irqStatus[1];
}

void __ClearIrqStatus(uint16_t irqMask)
{
  uint8_t buf[2];

  buf[0] = ( uint8_t )( ( ( uint16_t )irqMask >> 8 ) & 0x00FF );
  buf[1] = ( uint8_t )( ( uint16_t )irqMask & 0x00FF );
  __WriteCommand( RADIO_CLR_IRQSTATUS, buf, 2 );
}

void __Calibrate(CalibrationParams_t calibParam)
{
  uint8_t cal = ( calibParam.ADCBulkPEnable << 5 ) |
                ( calibParam.ADCBulkNEnable << 4 ) |
                ( calibParam.ADCPulseEnable << 3 ) |
                ( calibParam.PLLEnable << 2 ) |
                ( calibParam.RC13MEnable << 1 ) |
                ( calibParam.RC64KEnable );
  __WriteCommand( RADIO_CALIBRATE, &cal, 1 );
}

void __SetRegulatorMode(RadioRegulatorModes_t mode)
{
  __WriteCommand( RADIO_SET_REGULATORMODE, ( uint8_t* )&mode, 1 );
}

void __SetSaveContext(void)
{
  __WriteCommand( RADIO_SET_SAVECONTEXT, 0, 0 );
}

void __SetAutoTx(uint16_t time)
{
  uint16_t compensatedTime = time - ( uint16_t )AUTO_TX_OFFSET;
  uint8_t buf[2];

  buf[0] = ( uint8_t )( ( compensatedTime >> 8 ) & 0x00FF );
  buf[1] = ( uint8_t )( compensatedTime & 0x00FF );
  __WriteCommand( RADIO_SET_AUTOTX, buf, 2 );
}

void __SetAutoFs(bool enableAutoFs)
{
  __WriteCommand( RADIO_SET_AUTOFS, ( uint8_t * )&enableAutoFs, 1 );
}

void __SetLongPreamble(bool enable)
{
  __WriteCommand( RADIO_SET_LONGPREAMBLE, ( uint8_t * )&enable, 1 );
}

void __SetPayload(uint8_t *buffer, uint8_t size, uint8_t offset)
{
  __WriteBuffer( offset, buffer, size );
}

uint8_t __GetPayload(uint8_t *buffer, uint8_t *size , uint8_t maxSize)
{
  uint8_t offset;

  __GetRxBufferStatus( size, &offset );
  if ( *size > maxSize )
  {
    return 1;
  }
  __ReadBuffer( offset, buffer, *size );
  return 0;
}

void __SendPayload(uint8_t *payload, uint8_t size, TickTime_t timeout, uint8_t offset)
{
  __SetPayload( payload, size, offset );
  __SetTx( timeout );
}

uint8_t __SetSyncWord(uint8_t syncWordIdx, uint8_t *syncWord)
{
  uint16_t addr;
  uint8_t syncwordSize = 0;

  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_GFSK:
      syncwordSize = 5;
      switch ( syncWordIdx )
      {
        case 1:
          addr = REG_LR_SYNCWORDBASEADDRESS1;
          break;
        case 2:
          addr = REG_LR_SYNCWORDBASEADDRESS2;
          break;
        case 3:
          addr = REG_LR_SYNCWORDBASEADDRESS3;
          break;
        default:
          return 1;
      }
      break;
    case PACKET_TYPE_FLRC:
      // For FLRC packet type, the SyncWord is one byte shorter and
      // the base address is shifted by one byte
      syncwordSize = 4;
      switch ( syncWordIdx )
      {
        case 1:
          addr = REG_LR_SYNCWORDBASEADDRESS1 + 1;
          break;
        case 2:
          addr = REG_LR_SYNCWORDBASEADDRESS2 + 1;
          break;
        case 3:
          addr = REG_LR_SYNCWORDBASEADDRESS3 + 1;
          break;
        default:
          return 1;
      }
      break;
    case PACKET_TYPE_BLE:
      // For Ble packet type, only the first SyncWord is used and its
      // address is shifted by one byte
      syncwordSize = 4;
      switch ( syncWordIdx )
      {
        case 1:
          addr = REG_LR_SYNCWORDBASEADDRESS1 + 1;
          break;
        default:
          return 1;
      }
      break;
    default:
      return 1;
  }
  __WriteRegister( addr, syncWord, syncwordSize );
  return 0;
}

void __SetSyncWordErrorTolerance(uint8_t errorBits)
{
  errorBits = ( __ReadRegister_1( REG_LR_SYNCWORDTOLERANCE ) & 0xF0 ) | ( errorBits & 0x0F );
  __WriteRegister_1( REG_LR_SYNCWORDTOLERANCE, errorBits );
}

uint8_t __SetCrcSeed(uint8_t *seed)
{
  uint8_t updated = 0;
  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_GFSK:
    case PACKET_TYPE_FLRC:
      __WriteRegister( REG_LR_CRCSEEDBASEADDR, seed, 2 );
      updated = 1;
      break;
    case PACKET_TYPE_BLE:
      __WriteRegister_1(0x9c7, seed[2] );
      __WriteRegister_1(0x9c8, seed[1] );
      __WriteRegister_1(0x9c9, seed[0] );
      updated = 1;
      break;
    default:
      break;
  }
  return updated;
}

void __SetBleAccessAddress(uint32_t accessAddress)
{
  __WriteRegister_1( REG_LR_BLE_ACCESS_ADDRESS, ( accessAddress >> 24 ) & 0x000000FF );
  __WriteRegister_1( REG_LR_BLE_ACCESS_ADDRESS + 1, ( accessAddress >> 16 ) & 0x000000FF );
  __WriteRegister_1( REG_LR_BLE_ACCESS_ADDRESS + 2, ( accessAddress >> 8 ) & 0x000000FF );
  __WriteRegister_1( REG_LR_BLE_ACCESS_ADDRESS + 3, accessAddress & 0x000000FF );
}

void __SetBleAdvertizerAccessAddress(void)
{
  __SetBleAccessAddress( BLE_ADVERTIZER_ACCESS_ADDRESS );
}

void __SetCrcPolynomial(uint16_t polynomial)
{
  uint8_t val[2];

  val[0] = ( uint8_t )( polynomial >> 8 ) & 0xFF;
  val[1] = ( uint8_t )( polynomial  & 0xFF );

  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_GFSK:
    case PACKET_TYPE_FLRC:
      __WriteRegister( REG_LR_CRCPOLYBASEADDR, val, 2 );
      break;
    default:
      break;
  }
}

void __SetWhiteningSeed(uint8_t seed)
{
  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_GFSK:
    case PACKET_TYPE_FLRC:
    case PACKET_TYPE_BLE:
      __WriteRegister_1( REG_LR_WHITSEEDBASEADDR, seed );
      break;
    default:
      break;
  }
}

void __SetRangingIdLength(RadioRangingIdCheckLengths_t length)
{
  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_RANGING:
      __WriteRegister_1( REG_LR_RANGINGIDCHECKLENGTH, ( ( ( ( uint8_t )length ) & 0x03 ) << 6 ) | ( __ReadRegister_1( REG_LR_RANGINGIDCHECKLENGTH ) & 0x3F ) );
      break;
    default:
      break;
  }
}

void __SetDeviceRangingAddress(uint32_t address)
{
  uint8_t addrArray[] = { address >> 24, address >> 16, address >> 8, address };

  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_RANGING:
      __WriteRegister( REG_LR_DEVICERANGINGADDR, addrArray, 4 );
      break;
    default:
      break;
  }
}

void __SetRangingRequestAddress(uint32_t address)
{
  uint8_t addrArray[] = { address >> 24, address >> 16, address >> 8, address };

  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_RANGING:
      __WriteRegister( REG_LR_REQUESTRANGINGADDR, addrArray, 4 );
      break;
    default:
      break;
  }
}

int32_t complement2( const uint32_t num, const uint8_t bitCnt )
{
  int32_t retVal = ( int32_t )num;
  if ( num >= 2 << ( bitCnt - 2 ) )
  {
    retVal -= 2 << ( bitCnt - 1 );
  }
  return retVal;
}

int32_t __GetLoRaBandwidth( )
{
  int32_t bwValue = 0;

  switch ( __LoRaBandwidth )
  {
    case LORA_BW_0200:
      bwValue = 203125;
      break;
    case LORA_BW_0400:
      bwValue = 406250;
      break;
    case LORA_BW_0800:
      bwValue = 812500;
      break;
    case LORA_BW_1600:
      bwValue = 1625000;
      break;
    default:
      bwValue = 0;
  }
  return bwValue;
}

double __GetRangingResult(RadioRangingResultTypes_t resultType)
{
  uint32_t valLsb = 0;
  double val = 0.0;

  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_RANGING:
      __SetStandby( STDBY_XOSC );
      __WriteRegister_1( 0x97F, __ReadRegister_1( 0x97F ) | ( 1 << 1 ) ); // enable LORA modem clock
      __WriteRegister_1( REG_LR_RANGINGRESULTCONFIG, ( __ReadRegister_1( REG_LR_RANGINGRESULTCONFIG ) & MASK_RANGINGMUXSEL ) | ( ( ( ( uint8_t )resultType ) & 0x03 ) << 4 ) );
      valLsb = ( ( (uint32_t)__ReadRegister_1( REG_LR_RANGINGRESULTBASEADDR ) << 16 ) | ( (uint32_t)__ReadRegister_1( REG_LR_RANGINGRESULTBASEADDR + 1 ) << 8 ) | ( (uint32_t)__ReadRegister_1( REG_LR_RANGINGRESULTBASEADDR + 2 ) ) );
      __SetStandby( STDBY_RC );

      // Convertion from LSB to distance. For explanation on the formula, refer to Datasheet of SX1280
      switch ( resultType )
      {
        case RANGING_RESULT_RAW:
          // Convert the ranging LSB to distance in meter
          // The theoretical conversion from register value to distance [m] is given by:
          // distance [m] = ( complement2( register ) * 150 ) / ( 2^12 * bandwidth[MHz] ) )
          // The API provide BW in [Hz] so the implemented formula is complement2( register ) / bandwidth[Hz] * A,
          // where A = 150 / (2^12 / 1e6) = 36621.09
          val = ( double )complement2( valLsb, 24 ) / ( double )__GetLoRaBandwidth( ) * 36621.09375;
          break;

        case RANGING_RESULT_AVERAGED:
        case RANGING_RESULT_DEBIASED:
        case RANGING_RESULT_FILTERED:
          val = ( double )valLsb * 20.0 / 100.0;
          break;
        default:
          val = 0.0;
      }
      break;
    default:
      break;
  }
  return val;
}

void __SetRangingCalibration(uint16_t cal)
{
  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_RANGING:
      __WriteRegister_1( REG_LR_RANGINGRERXTXDELAYCAL, ( uint8_t )( ( cal >> 8 ) & 0xFF ) );
      __WriteRegister_1( REG_LR_RANGINGRERXTXDELAYCAL + 1, ( uint8_t )( ( cal ) & 0xFF ) );
      break;
    default:
      break;
  }
}

void __RangingClearFilterResult(void)
{
  uint8_t regVal = __ReadRegister_1( REG_LR_RANGINGRESULTCLEARREG );

  // To clear result, set bit 5 to 1 then to 0
  __WriteRegister_1( REG_LR_RANGINGRESULTCLEARREG, regVal | ( 1 << 5 ) );
  __WriteRegister_1( REG_LR_RANGINGRESULTCLEARREG, regVal & ( ~( 1 << 5 ) ) );
}

void __RangingSetFilterNumSamples(uint8_t num)
{
  // Silently set 8 as minimum value
  __WriteRegister_1( REG_LR_RANGINGFILTERWINDOWSIZE, ( num < DEFAULT_RANGING_FILTER_SIZE ) ? DEFAULT_RANGING_FILTER_SIZE : num );
}

double __GetFrequencyError()
{
  uint8_t efeRaw[3] = {0};
  uint32_t efe = 0;
  double efeHz = 0.0;

  switch ( __GetPacketType( true ) )
  {
    case PACKET_TYPE_LORA:
    case PACKET_TYPE_RANGING:
      efeRaw[0] = __ReadRegister_1( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB );
      efeRaw[1] = __ReadRegister_1( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB + 1 );
      efeRaw[2] = __ReadRegister_1( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB + 2 );
      efe = ( efeRaw[0] << 16 ) | ( efeRaw[1] << 8 ) | efeRaw[2];
      efe &= REG_LR_ESTIMATED_FREQUENCY_ERROR_MASK;

      efeHz = 1.55 * ( double )complement2( efe, 20 ) / ( 1600.0 / ( double )__GetLoRaBandwidth( ) * 1000.0 );
      break;

    case PACKET_TYPE_NONE:
    case PACKET_TYPE_BLE:
    case PACKET_TYPE_FLRC:
    case PACKET_TYPE_GFSK:
      break;
  }

  return efeHz;
}

void __ProcessIrqs(void)
{
  RadioPacketTypes_t packetType = PACKET_TYPE_NONE;

  if ( __PollingMode == true )
  {
    if ( __IrqState == true )
    {
      // __disable_irq( );
      __IrqState = false;
      // __enable_irq( );
    }
    else
    {
      return;
    }
  }

  if (__callbacks == NULL)
  {
    return;
  }

  packetType = __GetPacketType( true );
  uint16_t irqRegs = __GetIrqStatus( );
  __ClearIrqStatus( IRQ_RADIO_ALL );

  switch ( packetType )
  {
    case PACKET_TYPE_GFSK:
    case PACKET_TYPE_FLRC:
    case PACKET_TYPE_BLE:
      switch ( __OperatingMode )
      {
        case MODE_RX:
          if ( ( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
          {
            if ( ( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
            {
              if ( __callbacks->rxError != NULL )
              {
                __callbacks->rxError( IRQ_CRC_ERROR_CODE );
              }
            }
            else if ( ( irqRegs & IRQ_SYNCWORD_ERROR ) == IRQ_SYNCWORD_ERROR )
            {
              if ( __callbacks->rxError != NULL )
              {
                __callbacks->rxError( IRQ_SYNCWORD_ERROR_CODE );
              }
            }
            else
            {
              if ( __callbacks->rxDone != NULL )
              {
                __callbacks->rxDone( );
              }
            }
          }
          if ( ( irqRegs & IRQ_SYNCWORD_VALID ) == IRQ_SYNCWORD_VALID )
          {
            if ( __callbacks->rxSyncWordDone != NULL )
            {
              __callbacks->rxSyncWordDone( );
            }
          }
          if ( ( irqRegs & IRQ_SYNCWORD_ERROR ) == IRQ_SYNCWORD_ERROR )
          {
            if ( __callbacks->rxError != NULL )
            {
              __callbacks->rxError( IRQ_SYNCWORD_ERROR_CODE );
            }
          }
          if ( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
          {
            if ( __callbacks->rxTimeout != NULL )
            {
              __callbacks->rxTimeout( );
            }
          }
          break;
        case MODE_TX:
          if ( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
          {
            if ( __callbacks->txDone != NULL )
            {
              __callbacks->txDone( );
            }
          }
          if ( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
          {
            if ( __callbacks->txTimeout != NULL )
            {
              __callbacks->txTimeout( );
            }
          }
          break;
        default:
          // Unexpected IRQ: silently returns
          break;
      }
      break;
    case PACKET_TYPE_LORA:
      switch ( __OperatingMode )
      {
        case MODE_RX:
          if ( ( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
          {
            if ( ( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
            {
              if ( __callbacks->rxError != NULL )
              {
                __callbacks->rxError( IRQ_CRC_ERROR_CODE );
              }
            }
            else
            {
              if ( __callbacks->rxDone != NULL )
              {
                __callbacks->rxDone( );
              }
            }
          }
          if ( ( irqRegs & IRQ_HEADER_VALID ) == IRQ_HEADER_VALID )
          {
            if ( __callbacks->rxHeaderDone != NULL )
            {
              __callbacks->rxHeaderDone( );
            }
          }
          if ( ( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
          {
            if ( __callbacks->rxError != NULL )
            {
              __callbacks->rxError( IRQ_HEADER_ERROR_CODE );
            }
          }
          if ( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
          {
            if ( __callbacks->rxTimeout != NULL )
            {
              __callbacks->rxTimeout( );
            }
          }
          if ( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_DISCARDED ) == IRQ_RANGING_SLAVE_REQUEST_DISCARDED )
          {
            if ( __callbacks->rxError != NULL )
            {
              __callbacks->rxError( IRQ_RANGING_ON_LORA_ERROR_CODE );
            }
          }
          break;
        case MODE_TX:
          if ( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
          {
            if ( __callbacks->txDone != NULL )
            {
              __callbacks->txDone( );
            }
          }
          if ( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
          {
            if ( __callbacks->txTimeout != NULL )
            {
              __callbacks->txTimeout( );
            }
          }
          break;
        case MODE_CAD:
          if ( ( irqRegs & IRQ_CAD_DONE ) == IRQ_CAD_DONE )
          {
            if ( ( irqRegs & IRQ_CAD_DETECTED ) == IRQ_CAD_DETECTED )
            {
              if ( __callbacks->cadDone != NULL )
              {
                __callbacks->cadDone( true );
              }
            }
            else
            {
              if ( __callbacks->cadDone != NULL )
              {
                __callbacks->cadDone( false );
              }
            }
          }
          else if ( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
          {
            if ( __callbacks->rxTimeout != NULL )
            {
              __callbacks->rxTimeout( );
            }
          }
          break;
        default:
          // Unexpected IRQ: silently returns
          break;
      }
      break;
    case PACKET_TYPE_RANGING:
      switch ( __OperatingMode )
      {
        // MODE_RX indicates an IRQ on the Slave side
        case MODE_RX:
          if ( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_DISCARDED ) == IRQ_RANGING_SLAVE_REQUEST_DISCARDED )
          {
            if ( __callbacks->rangingDone != NULL )
            {
              __callbacks->rangingDone( IRQ_RANGING_SLAVE_ERROR_CODE );
            }
          }
          if ( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_VALID ) == IRQ_RANGING_SLAVE_REQUEST_VALID )
          {
            if ( __callbacks->rangingDone != NULL )
            {
              __callbacks->rangingDone( IRQ_RANGING_SLAVE_VALID_CODE );
            }
          }
          if ( ( irqRegs & IRQ_RANGING_SLAVE_RESPONSE_DONE ) == IRQ_RANGING_SLAVE_RESPONSE_DONE )
          {
            if ( __callbacks->rangingDone != NULL )
            {
              __callbacks->rangingDone( IRQ_RANGING_SLAVE_VALID_CODE );
            }
          }
          if ( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
          {
            if ( __callbacks->rangingDone != NULL )
            {
              __callbacks->rangingDone( IRQ_RANGING_SLAVE_ERROR_CODE );
            }
          }
          if ( ( irqRegs & IRQ_HEADER_VALID ) == IRQ_HEADER_VALID )
          {
            if ( __callbacks->rxHeaderDone != NULL )
            {
              __callbacks->rxHeaderDone( );
            }
          }
          if ( ( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
          {
            if ( __callbacks->rxError != NULL )
            {
              __callbacks->rxError( IRQ_HEADER_ERROR_CODE );
            }
          }
          break;
        // MODE_TX indicates an IRQ on the Master side
        case MODE_TX:
          if ( ( irqRegs & IRQ_RANGING_MASTER_TIMEOUT ) == IRQ_RANGING_MASTER_TIMEOUT )
          {
            if ( __callbacks->rangingDone != NULL )
            {
              __callbacks->rangingDone( IRQ_RANGING_MASTER_ERROR_CODE );
            }
          }
          if ( ( irqRegs & IRQ_RANGING_MASTER_RESULT_VALID ) == IRQ_RANGING_MASTER_RESULT_VALID )
          {
            if ( __callbacks->rangingDone != NULL )
            {
              __callbacks->rangingDone( IRQ_RANGING_MASTER_VALID_CODE );
            }
          }
          break;
        default:
          // Unexpected IRQ: silently returns
          break;
      }
      break;
    default:
      // Unexpected IRQ: silently returns
      break;
  }
}

void __ForcePreambleLength(RadioPreambleLengths_t preambleLength)
{
  __WriteRegister_1( REG_LR_PREAMBLELENGTH, ( __ReadRegister_1( REG_LR_PREAMBLELENGTH ) & MASK_FORCE_PREAMBLELENGTH ) | preambleLength );
}
