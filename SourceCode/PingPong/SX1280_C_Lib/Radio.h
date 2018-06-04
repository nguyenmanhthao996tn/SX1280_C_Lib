#ifndef __RADIO_H__
#define __RADIO_H__

#include "Radio_Methods.h"

typedef struct {
  void (*Init)(RadioCallbacks_t* callbacks);
  void (*SetPollingMode)(void);
  void (*SetInterruptMode)(void);
  void (*SetRegistersDefault)(void);
  uint16_t (*GetFirmwareVersion)(void);
  void (*Reset)(void);
  void (*Wakeup)(void);
  void (*WriteCommand)(RadioCommands_t opcode, uint8_t *buffer, uint16_t size);
  void (*ReadCommand)(RadioCommands_t opcode, uint8_t *buffer, uint16_t size);
  void (*WriteRegister)(uint16_t address, uint8_t *buffer, uint16_t size);
  //  void (*WriteRegister)(uint16_t address, uint8_t value);
  void (*ReadRegister)(uint16_t address, uint8_t *buffer, uint16_t size);
  //  uint8_t (*ReadRegister)(uint16_t address);
  void (*WriteBuffer)(uint8_t offset, uint8_t *buffer, uint8_t size);
  void (*ReadBuffer)(uint8_t offset, uint8_t *buffer, uint8_t size);
  uint8_t (*GetDioStatus)(void);
  RadioOperatingModes_t (*GetOpMode)(void);
  RadioStatus_t (*GetStatus)(void);
  void (*SetSleep)(SleepParams_t sleepConfig);
  void (*SetStandby)(RadioStandbyModes_t mode);
  void (*SetFs)(void);
  void (*SetTx)(TickTime_t timeout);
  void (*SetRx)(TickTime_t timeout);
  void (*SetRxDutyCycle)(RadioTickSizes_t periodBase, uint16_t periodBaseCountRx, uint16_t periodBaseCountSleep);
  void (*SetCad)(void);
  void (*SetTxContinuousWave)(void);
  void (*SetTxContinuousPreamble)(void);
  void (*SetPacketType)(RadioPacketTypes_t packetType);
  RadioPacketTypes_t (*GetPacketType)(bool returnLocalCopy = false);
  void (*SetRfFrequency)(uint32_t rfFrequency);
  void (*SetTxParams)(int8_t power, RadioRampTimes_t rampTime);
  void (*SetCadParams)(RadioLoRaCadSymbols_t cadSymbolNum);
  void (*SetBufferBaseAddresses)(uint8_t txBaseAddress, uint8_t rxBaseAddress);
  void (*SetModulationParams)(ModulationParams_t *modParams);
  void (*SetPacketParams)(PacketParams_t *packetParams);
  void (*GetRxBufferStatus)(uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer);
  void (*GetPacketStatus)(PacketStatus_t *packetStatus);
  int8_t (*GetRssiInst)(void);
  void (*SetDioIrqParams)(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask);
  uint16_t (*GetIrqStatus)(void);
  void (*ClearIrqStatus)(uint16_t irqMask);
  void (*Calibrate)(CalibrationParams_t calibParam);
  void (*SetRegulatorMode)(RadioRegulatorModes_t mode);
  void (*SetSaveContext)(void);
  void (*SetAutoTx)(uint16_t time);
  void (*SetAutoFs)(bool enableAutoFs);
  void (*SetLongPreamble)(bool enable);
  void (*SetPayload)(uint8_t *payload, uint8_t size, uint8_t offsetx00);
  uint8_t (*GetPayload)(uint8_t *payload, uint8_t *size, uint8_t maxSize);
  void (*SendPayload)(uint8_t *payload, uint8_t size, TickTime_t timeout, uint8_t offset);
  uint8_t (*SetSyncWord)(uint8_t syncWordIdx, uint8_t *syncWord);
  void (*SetSyncWordErrorTolerance)(uint8_t errorBits);
  uint8_t (*SetCrcSeed)(uint8_t *seed);
  void (*SetBleAccessAddress)(uint32_t accessAddress);
  void (*SetBleAdvertizerAccessAddress)(void);
  void (*SetCrcPolynomial)(uint16_t polynomial);
  void (*SetWhiteningSeed)(uint8_t seed);
  void (*SetRangingIdLength)(RadioRangingIdCheckLengths_t length);
  void (*SetDeviceRangingAddress)(uint32_t address);
  void (*SetRangingRequestAddress)(uint32_t address);
  double (*GetRangingResult)(RadioRangingResultTypes_t resultType);
  void (*SetRangingCalibration)(uint16_t cal);
  void (*RangingClearFilterResult)(void);
  void (*RangingSetFilterNumSamples)(uint8_t numSample);
  double (*GetFrequencyError)();
  void (*ProcessIrqs)(void);
  void (*ForcePreambleLength)(RadioPreambleLengths_t preambleLength);
} Radio_t;

static const Radio_t Radio = {
  __Init,
  __SetPollingMode,
  __SetInterruptMode,
  __SetRegistersDefault,
  __GetFirmwareVersion,
  __Reset,
  __Wakeup,
  __WriteCommand,
  __ReadCommand,
  __WriteRegister,
  __ReadRegister,
  __WriteBuffer,
  __ReadBuffer,
  __GetDioStatus,
  __GetOpMode,
  __GetStatus,
  __SetSleep,
  __SetStandby,
  __SetFs,
  __SetTx,
  __SetRx,
  __SetRxDutyCycle,
  __SetCad,
  __SetTxContinuousWave,
  __SetTxContinuousPreamble,
  __SetPacketType,
  __GetPacketType,
  __SetRfFrequency,
  __SetTxParams,
  __SetCadParams,
  __SetBufferBaseAddresses,
  __SetModulationParams,
  __SetPacketParams,
  __GetRxBufferStatus,
  __GetPacketStatus,
  __GetRssiInst,
  __SetDioIrqParams,
  __GetIrqStatus,
  __ClearIrqStatus,
  __Calibrate,
  __SetRegulatorMode,
  __SetSaveContext,
  __SetAutoTx,
  __SetAutoFs,
  __SetLongPreamble,
  __SetPayload,
  __GetPayload,
  __SendPayload,
  __SetSyncWord,
  __SetSyncWordErrorTolerance,
  __SetCrcSeed,
  __SetBleAccessAddress,
  __SetBleAdvertizerAccessAddress,
  __SetCrcPolynomial,
  __SetWhiteningSeed,
  __SetRangingIdLength,
  __SetDeviceRangingAddress,
  __SetRangingRequestAddress,
  __GetRangingResult,
  __SetRangingCalibration,
  __RangingClearFilterResult,
  __RangingSetFilterNumSamples,
  __GetFrequencyError,
  __ProcessIrqs,
  __ForcePreambleLength
};

#endif /* __RADIO_H__ */
