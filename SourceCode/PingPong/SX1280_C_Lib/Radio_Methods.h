#ifndef __RADIO_METHODS_H__
#define __RADIO_METHODS_H__

#include "Header.h"

void __Init(RadioCallbacks_t* callbacks);
void __SetPollingMode(void);
void __SetInterruptMode(void);
void __SetRegistersDefault(void);
uint16_t __GetFirmwareVersion(void);
void __Reset(void);
void __Wakeup(void);
void __WriteCommand(RadioCommands_t opcode, uint8_t *buffer, uint16_t size);
void __ReadCommand(RadioCommands_t opcode, uint8_t *buffer, uint16_t size);
void __WriteRegister(uint16_t address, uint8_t *buffer, uint16_t size);
//void __WriteRegister(uint16_t address, uint8_t value);
void __ReadRegister(uint16_t address, uint8_t *buffer, uint16_t size);
//uint8_t __ReadRegister(uint16_t address);
void __WriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size);
void __ReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size);
uint8_t __GetDioStatus(void);
RadioOperatingModes_t __GetOpMode(void);
RadioStatus_t __GetStatus(void);
void __SetSleep(SleepParams_t sleepConfig);
void __SetStandby(RadioStandbyModes_t mode);
void __SetFs(void);
void __SetTx(TickTime_t timeout);
void __SetRx(TickTime_t timeout);
void __SetRxDutyCycle(RadioTickSizes_t periodBase, uint16_t periodBaseCountRx, uint16_t periodBaseCountSleep);
void __SetCad(void);
void __SetTxContinuousWave(void);
void __SetTxContinuousPreamble(void);
void __SetPacketType(RadioPacketTypes_t packetType);
RadioPacketTypes_t __GetPacketType(bool returnLocalCopy);
void __SetRfFrequency(uint32_t rfFrequency);
void __SetTxParams(int8_t power, RadioRampTimes_t rampTime);
void __SetCadParams(RadioLoRaCadSymbols_t cadSymbolNum);
void __SetBufferBaseAddresses(uint8_t txBaseAddress, uint8_t rxBaseAddress);
void __SetModulationParams(ModulationParams_t *modParams);
void __SetPacketParams(PacketParams_t *packetParams);
void __GetRxBufferStatus(uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer);
void __GetPacketStatus(PacketStatus_t *packetStatus);
int8_t __GetRssiInst(void);
void __SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask);
uint16_t __GetIrqStatus(void);
void __ClearIrqStatus(uint16_t irqMask);
void __Calibrate(CalibrationParams_t calibParam);
void __SetRegulatorMode(RadioRegulatorModes_t mode);
void __SetSaveContext(void);
void __SetAutoTx(uint16_t time);
void __SetAutoFs(bool enableAutoFs);
void __SetLongPreamble(bool enable);
void __SetPayload(uint8_t *payload, uint8_t size, uint8_t offsetx00);
uint8_t __GetPayload(uint8_t *payload, uint8_t *size, uint8_t maxSize);
void __SendPayload(uint8_t *payload, uint8_t size, TickTime_t timeout, uint8_t offset);
uint8_t __SetSyncWord(uint8_t syncWordIdx, uint8_t *syncWord);
void __SetSyncWordErrorTolerance(uint8_t errorBits);
uint8_t __SetCrcSeed(uint8_t *seed);
void __SetBleAccessAddress(uint32_t accessAddress);
void __SetBleAdvertizerAccessAddress(void);
void __SetCrcPolynomial(uint16_t polynomial);
void __SetWhiteningSeed(uint8_t seed);
void __SetRangingIdLength(RadioRangingIdCheckLengths_t length);
void __SetDeviceRangingAddress(uint32_t address);
void __SetRangingRequestAddress(uint32_t address);
double __GetRangingResult(RadioRangingResultTypes_t resultType);
void __SetRangingCalibration(uint16_t cal);
void __RangingClearFilterResult(void);
void __RangingSetFilterNumSamples(uint8_t numSample);
double __GetFrequencyError();
void __ProcessIrqs(void);
void __ForcePreambleLength(RadioPreambleLengths_t preambleLength);

#endif /* __RADIO_METHODS_H__ */
