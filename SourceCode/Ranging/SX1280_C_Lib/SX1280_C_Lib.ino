#include "Radio.h"
#include "FreqLUT.h"

#define IS_MASTER 1

#define RF_FREQUENCY                                2400000000// Hz
#define TX_OUTPUT_POWER                             13 // dBm
#define RX_TIMEOUT_TICK_SIZE                        RADIO_TICK_SIZE_1000_US
#define RX_TIMEOUT_VALUE                            1000 // ms
#define TX_TIMEOUT_VALUE                            10000 // ms
#define BUFFER_SIZE                                 255
#define NO_OF_RANGING                               10

const uint32_t rangingAddress[] = {
  0x10000000,
  0x32100000,
  0x20012301,
  0x20000abc,
  0x32101230
};
#define RANGING_ADDRESS_SIZE 5

/*!
   \brief Ranging raw factors
                                    SF5     SF6     SF7     SF8     SF9     SF10
*/
const uint16_t RNG_CALIB_0400[] = { 10299,  10271,  10244,  10242,  10230,  10246  };
const uint16_t RNG_CALIB_0800[] = { 11486,  11474,  11453,  11426,  11417,  11401  };
const uint16_t RNG_CALIB_1600[] = { 13308,  13493,  13528,  13515,  13430,  13376};//13376
const double   RNG_FGRAD_0400[] = { -0.148, -0.214, -0.419, -0.853, -1.686, -3.423 };
const double   RNG_FGRAD_0800[] = { -0.041, -0.811, -0.218, -0.429, -0.853, -1.737 };
const double   RNG_FGRAD_1600[] = { 0.103,  -0.041, -0.101, -0.211, -0.424, -0.87  };
const double   RNG_RATIO_1600 = 0.000000023;

const char* IrqRangingCodeName[] = {
  "IRQ_RANGING_SLAVE_ERROR_CODE",
  "IRQ_RANGING_SLAVE_VALID_CODE",
  "IRQ_RANGING_MASTER_ERROR_CODE",
  "IRQ_RANGING_MASTER_VALID_CODE"
};

typedef enum
{
  APP_IDLE,
  APP_RX,
  APP_RX_TIMEOUT,
  APP_RX_ERROR,
  APP_TX,
  APP_TX_TIMEOUT,
  APP_RX_SYNC_WORD,
  APP_RX_HEADER,
  APP_RANGING,
  APP_CAD
} AppStates_t;

void txDoneIRQ( void );
void rxDoneIRQ( void );
void rxSyncWordDoneIRQ( void );
void rxHeaderDoneIRQ( void );
void txTimeoutIRQ( void );
void rxTimeoutIRQ( void );
void rxErrorIRQ( IrqErrorCode_t errCode );
void rangingDoneIRQ( IrqRangingCode_t val );
void cadDoneIRQ( bool cadFlag );

RadioCallbacks_t Callbacks = {
  txDoneIRQ,
  rxDoneIRQ,
  rxSyncWordDoneIRQ,
  rxHeaderDoneIRQ,
  txTimeoutIRQ,
  rxTimeoutIRQ,
  rxErrorIRQ,
  rangingDoneIRQ,
  cadDoneIRQ
};

extern const Radio_t Radio;

uint16_t masterIrqMask = IRQ_RANGING_MASTER_RESULT_VALID | IRQ_RANGING_MASTER_TIMEOUT;
uint16_t slaveIrqMask = IRQ_RANGING_SLAVE_RESPONSE_DONE | IRQ_RANGING_SLAVE_REQUEST_DISCARDED;

PacketParams_t packetParams;
PacketStatus_t packetStatus;
ModulationParams_t modulationParams;

volatile AppStates_t AppState = APP_IDLE;
IrqRangingCode_t MasterIrqRangingCode = IRQ_RANGING_MASTER_ERROR_CODE;
uint8_t Buffer[BUFFER_SIZE];
uint8_t BufferSize = BUFFER_SIZE;

uint16_t RxIrqMask = IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT;
uint16_t TxIrqMask = IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT;

uint16_t CalibVal = 10000;
enum _Role { SLAVE, MASTER } Role =  IS_MASTER;
uint16_t RangingData[10] = {0};

void LoraPacketInit(bool Tx)
{
  modulationParams.PacketType = PACKET_TYPE_LORA;
  modulationParams.Params.LoRa.SpreadingFactor = LORA_SF12;
  modulationParams.Params.LoRa.Bandwidth = LORA_BW_1600;
  modulationParams.Params.LoRa.CodingRate = LORA_CR_LI_4_7;

  packetParams.PacketType = PACKET_TYPE_LORA;
  packetParams.Params.LoRa.PreambleLength = 12;
  packetParams.Params.LoRa.HeaderType = LORA_PACKET_VARIABLE_LENGTH;
  packetParams.Params.LoRa.PayloadLength = 4;
  packetParams.Params.LoRa.Crc = LORA_CRC_ON;
  packetParams.Params.LoRa.InvertIQ = LORA_IQ_NORMAL;

  Radio.SetStandby( STDBY_RC );
  Radio.SetPacketType( modulationParams.PacketType );
  Radio.SetModulationParams( &modulationParams );
  Radio.SetPacketParams( &packetParams );
  Radio.SetRfFrequency( RF_FREQUENCY );
  Radio.SetBufferBaseAddresses( 0x00, 0x00 );
  Radio.SetTxParams( TX_OUTPUT_POWER, RADIO_RAMP_20_US );

  if (Tx) // Tx
    Radio.SetDioIrqParams( TxIrqMask, TxIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );

  else
  {
    Radio.SetDioIrqParams( RxIrqMask, RxIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
    Radio.SetRx( ( TickTime_t ) {
      RX_TIMEOUT_TICK_SIZE, 0
    }  );
  }
}

void RangingPacketInit(long RangingCalib)
{
  modulationParams.PacketType = PACKET_TYPE_RANGING;
  modulationParams.Params.LoRa.SpreadingFactor = LORA_SF10;
  modulationParams.Params.LoRa.Bandwidth = LORA_BW_1600;
  modulationParams.Params.LoRa.CodingRate = LORA_CR_LI_4_5;

  packetParams.PacketType = PACKET_TYPE_RANGING;
  packetParams.Params.LoRa.PreambleLength = 12;
  packetParams.Params.LoRa.HeaderType = LORA_PACKET_VARIABLE_LENGTH;
  packetParams.Params.LoRa.PayloadLength = 7;
  packetParams.Params.LoRa.Crc = LORA_CRC_ON;
  packetParams.Params.LoRa.InvertIQ = LORA_IQ_NORMAL;

  Radio.SetStandby( STDBY_RC );
  Radio.SetPacketType( modulationParams.PacketType );
  Radio.SetModulationParams( &modulationParams );
  Radio.SetPacketParams( &packetParams );
  Radio.SetRfFrequency( Channels[0] );
  Radio.SetTxParams( TX_OUTPUT_POWER, RADIO_RAMP_20_US );
  Radio.SetBufferBaseAddresses( 0x00, 0x00 );
  Radio.SetRangingCalibration( RangingCalib ); // Bandwith 1600, SF10   377577
  Radio.SetInterruptMode();

  if (IS_MASTER)
  {
    Master_Init();
  }
  else // SLAVE
  {
    Slave_Init();
  }
}

void Slave_Init()
{
  Role = SLAVE;
    Serial.println ("Salve role");
    Radio.SetRangingIdLength(RANGING_IDCHECK_LENGTH_32_BITS);
    Radio.SetDeviceRangingAddress(rangingAddress[2]);
    Radio.SetDioIrqParams( slaveIrqMask, slaveIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE);
    Radio.SetRx((TickTime_t) {
    RADIO_TICK_SIZE_1000_US, 0xFFFF
  });
}

void Master_Init()
{
  Role = MASTER ;
    Serial.println ("Master role");
    Radio.SetRangingRequestAddress(rangingAddress[2]);
    Radio.SetDioIrqParams( masterIrqMask, masterIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE);
    Radio.SetTx((TickTime_t) {
      RADIO_TICK_SIZE_1000_US, 0xFFFF
    });
}

void setup() {
  Serial.begin(115200);
  if (IS_MASTER)
  {
    Serial.println("SX1280 MASTER");
  }
  else
  {
    Serial.println("SX1280 SLAVE");
  }
  Radio.Init(&Callbacks);
  Radio.SetRegulatorMode( USE_DCDC ); // Can also be set in LDO mode but consume more power
  Serial.println( "\n\n\r     SX1280 Ranging Demo Application. \n\n\r");
}

void loop() {
  uint8_t counter = 0;
  bool Finish = false ; // indicate when 10 ranging switch finish  

  // remove Serial buffer 
  while (Serial.available())
    Serial.read();
    
  if (IS_MASTER)
  {
    Serial.println("Enter Calibrate value : ");
    while (!Serial.available()); // waiting for input from uart
    CalibVal = Serial.parseInt();
    Serial.print("Calib value is : " );
    Serial.println(CalibVal,BIN);
    
    uint8_t Payload[4] = {(CalibVal >> 24) & 0xFF , (CalibVal >> 16)&0xFF, (CalibVal >> 8)&0xFF, CalibVal&0xFF};
    for (int i = 0; i < 4; ++i)
    {
      Serial.print(Payload[i],BIN);
      Serial.print(" : ");
      Serial.println(Payload[i]);
    }
    LoraPacketInit(true);
    Radio.SendPayload(  Payload  , 4, ( TickTime_t ) {
        RX_TIMEOUT_TICK_SIZE, TX_TIMEOUT_VALUE
      }, 0 );
  } 
  else
    LoraPacketInit(false);
  AppState = APP_IDLE;
  
  while (!Finish)
  {
    switch (AppState)
    {
      case APP_IDLE:
        break;
      case APP_RX:
        AppState = APP_IDLE;
        Serial.println("APP_RX");
        Radio.GetPayload( Buffer, &BufferSize, BUFFER_SIZE );
        if (BufferSize > 0)
        {
          Serial.print("RX ");
          Serial.print(BufferSize);
          Serial.println(" bytes:");
  
          long RxData = 0;
          for (int i = 0; i < BufferSize; i++)
          {
            RxData <<= 8;
            RxData += Buffer[i];
            Serial.print(Buffer[i],BIN);
            Serial.print(" : ");
            Serial.println(Buffer[i]);
          }
          Serial.println( RxData);
          RangingPacketInit(RxData);
        }
        break;
      case APP_RX_TIMEOUT:
        AppState = APP_IDLE;
        //Serial.println("APP_RX_TIMEOUT");
        break;
      case APP_RX_ERROR:
        AppState = APP_IDLE;
        // Serial.println("APP_RX_ERROR");
        break;
      case APP_TX:
        AppState = APP_IDLE;
        Serial.println("APP_TX");
        delay(10);
        RangingPacketInit(CalibVal);
        break;
      case APP_TX_TIMEOUT:
        AppState = APP_IDLE;
        //Serial.println("APP_TX_TIMEOUT");
        break;
      case APP_RX_SYNC_WORD:
        AppState = APP_IDLE;
        // Serial.println("APP_RX_SYNC_WORD");
        break;
      case APP_RX_HEADER:
        AppState = APP_IDLE;
        // Serial.println("APP_RX_HEADER");
        break;
      case APP_RANGING:
        AppState = APP_IDLE;
        Serial.println("APP_RANGING");
        if (Role == MASTER )
        {
          switch (MasterIrqRangingCode)
          {
            case IRQ_RANGING_MASTER_VALID_CODE:
              uint8_t reg[3];
  
              double rangingResult;
              rangingResult = Radio.GetRangingResult(RANGING_RESULT_RAW);
              Serial.print("Measure no ");
              Serial.println(counter + 1);
              Serial.print("Raw data: ");
              Serial.println(rangingResult);
              Serial.println();
  
              // Store data into array 
              RangingData [counter++] = rangingResult;
  
              // Check if ranging finish 
              if ((counter == NO_OF_RANGING) & !IS_MASTER )
              {
                Finish = true;
                break;
              }
              //delay(2000);
              Slave_Init();
              break;
            case IRQ_RANGING_MASTER_ERROR_CODE:
              Serial.println("Raging Error");
              Master_Init();
              break;
            default:
              break;
          }
        }
        else if (Role == SLAVE)
        {
          if ((counter == NO_OF_RANGING) & IS_MASTER )
          {
            Finish = true;
            break;
          }
          delay(10);
          Master_Init();
        }
        break;
      case APP_CAD:
        AppState = APP_IDLE;
        // Serial.println("APP_CAD");
        break;
      default:
        AppState = APP_IDLE;
        break;
    }
  }
  // Calculate mean of ranging data
  uint16_t result = 0 ;
  for (int i = 0; i < 10; ++i)
    result += RangingData[i]*100 ; // Multiple by 100 to get 2 number after floating point 
  result /= 10;
  Serial.print ("Result is : ");
  Serial.println (result);
  // Waiting for slave result 
  if ( IS_MASTER )
  {
    LoraPacketInit(false);
    Serial.println("Waiting for Slave data");
    while ( AppState != APP_RX );

    Radio.GetPayload( Buffer, &BufferSize, BUFFER_SIZE );
    if (BufferSize > 0)
    {
      Serial.print("RX ");
      Serial.print(BufferSize);
      Serial.println(" bytes:");

      uint32_t RxData = 0;
      for (int i = 0; i < BufferSize; i++)
      {
        RxData <<= 8;
        RxData += Buffer[i];
      }
      Serial.print("Slave result : ");
      Serial.println(RxData);
      Serial.print("Ranging result is : ");
      Serial.println( (double)(RxData + result)/200 ); 
    }
  }
  // Calculate result and send to master 
  else 
  {
    uint8_t Payload[4] = {(result >> 24) & 0xFF , (result >> 16)&0xFF, (result >> 8)&0xFF, result&0xFF};
    LoraPacketInit(true);
    delay(10);
    Radio.SendPayload(  Payload  , 4, ( TickTime_t ) {
        RX_TIMEOUT_TICK_SIZE, TX_TIMEOUT_VALUE
      }, 0 );
    while ( (AppState != APP_TX) && (AppState != APP_TX_TIMEOUT));  // Wait for Tx to complete
  }
}

void txDoneIRQ( void )
{
  AppState = APP_TX;
}

void rxDoneIRQ( void )
{
  Serial.println("Rx done");
  AppState = APP_RX;
}

void rxSyncWordDoneIRQ( void )
{
  AppState = APP_RX_SYNC_WORD;
}

void rxHeaderDoneIRQ( void )
{
  AppState = APP_RX_HEADER;
}

void txTimeoutIRQ( void )
{
  AppState = APP_TX_TIMEOUT;
}

void rxTimeoutIRQ( void )
{
  AppState = APP_RX_TIMEOUT;
}

void rxErrorIRQ( IrqErrorCode_t errCode )
{
  AppState = APP_RX_ERROR;
}

void rangingDoneIRQ( IrqRangingCode_t val )
{
  AppState = APP_RANGING;
  MasterIrqRangingCode = val;
}

void cadDoneIRQ( bool cadFlag )
{
  AppState = APP_CAD;
}
