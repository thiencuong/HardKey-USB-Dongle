#ifndef  _EEPROM_PROCESS_H_
#define  _EEPROM_PROCESS_H_
#include "main.h"

__packed typedef struct _Data_UserTypeDef
{
  uint16_t                BootCommand;
  uint16_t                FirmwareAlready;
  uint8_t                 PasswordSave[64];
  uint8_t                 CPUid[64];
  uint8_t                 PCname[64];
	uint64_t								InstallDate; // date install software
	uint64_t								LifeTime; // life time of soft
	uint64_t								LastDateUsing; // Last date runing soft
  uint8_t                 Passphase[32]; //Pass phase
	uint32_t                nullChar;
} _Data_UserTypeDef;

uint8_t ReadFlashByte(uint32_t address);
uint16_t ReadFlashHalfWord(uint32_t address);
uint32_t ReadFlashWord(uint32_t address);
uint64_t ReadFlashDoubleWord(uint32_t address);
//
void UserFlashErase(void);
void ProgramFlash(uint8_t *DataFlash, uint32_t StartAddress, uint32_t Length);
uint8_t FlagSaveFlash(uint16_t BootCommand, uint16_t FirmwareAlready);
void SavePassword(uint8_t *Password, uint8_t Len);
#endif
