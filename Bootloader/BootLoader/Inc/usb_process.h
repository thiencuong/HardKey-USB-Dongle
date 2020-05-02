#ifndef __USB_PROCESS_H__
#define __USB_PROCESS_H__

#include "stm32f1xx_hal.h"
#include <stdbool.h>
//
int UsbProcess(uint8_t *DataUsb);
int FlashNewFirmware(uint8_t *DataFlash, uint32_t Address, char Len);
int EasyFlash(void);
int PasswordProcess(uint8_t *DataUsb, uint8_t *Passwrod);
void LockFlash(void);
void UnLockFlash(void);
#endif
