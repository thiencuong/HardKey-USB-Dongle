#include "usb_process.h"
#include "memory.h"
#include "Flash.h"
#include <stdio.h>
#include <string.h>
//
extern int DataSendAlready; // dat bang 1 se truyen du lieu di
//
extern struct buttonState MyButton;
extern bool	RequestUpdate;
//
int UsbProcess(uint8_t *DataUsb)
{
	uint8_t msg_id = 0;
	uint8_t msg_size = 0;
	uint32_t DataPost = 0;
	//
	if (DataUsb[0] != '?' || DataUsb[1] != '#' || DataUsb[2] != '#') 
	{	// invalid start - discard
		return 1;
	}
	//
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_9);
	HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
	//
	msg_id = DataUsb[3];
	msg_size = DataUsb[4];
	DataPost = ((uint32_t) DataUsb[5] << 24) + (DataUsb[6] << 16) + (DataUsb[7] << 8) + DataUsb[8];
	//
#if defined(DEBUG_MODE)
	printf("ID=%d, Size=%d\r\n",msg_id,msg_size);
	printf("Post= 0x%X\r\n",DataPost);
#endif
	//
	switch(msg_id)
	{
		case 0x01:// FirmwareErase message (id 1)
			EasyFlash();
			FlagSaveFlash(200,'N');
			printf("easy complete");
			DataSendAlready = 1;
			break;
		case 0x02:// FirmwareUpload message (id 2)
			FlashNewFirmware(DataUsb,DataPost,msg_size);
			DataSendAlready = 2;
			break;
		case 0x03:// neu co yeu cau cap nhap thong tin firmware
			RequestUpdate = true;
			printf("Start Mark start App\r\n");
			FlagSaveFlash(100,0);
			printf("Mark start App ok, Reset system\r\n");
			HAL_NVIC_SystemReset();
			break;
		case 0x04:// yeu cau vao che do DownLoad
			RequestUpdate = true;
			printf("Start Clear Mark start App\r\n");
			FlagSaveFlash(200,0);
			printf("Clear Mark start App ok\r\n");
			DataSendAlready = 1;
			break;
		case 0x05:// neu co yeu cau cap nhap thong tin firmware
			printf("Start Set firmware\r\n");
			if(FlagSaveFlash(100,'R') == 0)
			{
				printf("Start Set firmware already ok\r\n");
			}
			else
			{
				printf("Start Set firmware Already Fail\r\n");
			}
			DataSendAlready = 1;
			break;
		case 0x06:// neu co yeu cau cap nhap thong tin firmware
			printf("Start Lock Flash\r\n");
			LockFlash();
			DataSendAlready = 1;
			break;
		case 0x07:// neu co yeu cau cap nhap thong tin firmware
			printf("Start Unlock Flash\r\n");
			UnLockFlash();
			DataSendAlready = 1;
			break;
		case 0xA0:// receive password
			SavePassword(DataUsb+5,msg_size);
			printf("Set Password complete\r\n");
			DataSendAlready = 1;
			break;
		default:
			return 2;
			break;
	}
	return 0;
}
// xu ly password
int PasswordProcess(uint8_t *DataUsb, uint8_t *Passwrod)
{
	uint8_t msg_id = 0;
	uint8_t msg_size = 0;
	uint8_t SHAdata[65];
	uint8_t ShaTmp[4];
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	sha256_Init(&sha256);
	//
	if (DataUsb[0] != '?' || DataUsb[1] != '#' || DataUsb[2] != '#') 
	{	// invalid start - discard
		return 1;
	}
	//
	HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_9);
	HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
	//
	msg_id = DataUsb[3];
	msg_size = DataUsb[4];
	//
#if defined(DEBUG_MODE)
	printf("ID=%d, Size=%d\r\n",msg_id,msg_size);
#endif
	//
	uint8_t *DataUsbCompare = DataUsb;
	DataUsbCompare += 5;
	//
	sha256_Update(&sha256, DataUsbCompare, msg_size);
	sha256_Final(&sha256,hash);
	//
	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf((char*)ShaTmp,"%02X",hash[i]);
		SHAdata[i*2] = ShaTmp[0];
		SHAdata[i*2 + 1] = ShaTmp[1];
	}
	//
	SHAdata[64] = 0;
	switch(msg_id)
	{
		case 0xA1:// receive password
#if defined(DEBUG_MODE)
			printf("Check Password complete, %d, %s\r\n", strcmp((char *)Passwrod,(char *)SHAdata), DataUsbCompare);
			printf("Pass save: %s\r\n", Passwrod);
			printf("SHA Pass: %s\r\n", SHAdata);
#endif
			if(strcmp((char *)Passwrod,(char *)SHAdata) == 0)
			{
				return 0;
			}
			else
			{
				return 4;
			}
			break;
		default:
			return 2;
			break;
	}
	return 3;
}
// Save data to flash
int FlashNewFirmware(uint8_t *DataFlash, uint32_t Address, char Len)
{
	uint8_t *DataToWrite = DataFlash + 9;
	//
#if defined(DEBUG_MODE)
	for(int jk = 0; jk < Len; jk++)
	{
		printf("%d-",*(DataToWrite + jk));
	}
	printf("\r\n->%d<-\r\n",*(DataToWrite));
#endif
	//
	HAL_FLASH_Unlock();
	ProgramFlash(DataToWrite,FLASH_APP_START + Address,Len);
	//
	HAL_FLASH_Lock();
	return 0;
}
//
int EasyFlash(void)
{
	UserFlashErase();
	return 0;
}
//
// lock Flash
void LockFlash(void)
{
	FLASH_OBProgramInitTypeDef OBInit; // programming option structure
	uint8_t mode = HAL_FLASHEx_OBGetUserData(OB_DATA_ADDRESS_DATA0); // from option byte 0
	//
	OBInit.DATAAddress = OB_DATA_ADDRESS_DATA0; // address of type FLASHEx_OB_Data_Address
	OBInit.DATAData = mode; // mode value to be saved
	OBInit.OptionType = OPTIONBYTE_RDP; // of type FLASHEx_OB_Type
	OBInit.RDPLevel = OB_RDP_LEVEL_1;

	 
	// unlock FLASH in general
	 
	if(HAL_FLASH_Unlock() == HAL_OK) 
	{
		 // unlock option bytes in particular
		 if(HAL_FLASH_OB_Unlock() == HAL_OK) 
		 {
				// erase option bytes before programming
				if(HAL_FLASHEx_OBErase() == HAL_OK) 
				{
					 // program selected option byte
					 HAL_FLASHEx_OBProgram(&OBInit); // result not checked as there is no recourse at this point
					 if(HAL_FLASH_OB_Lock() == HAL_OK) 
						{
							HAL_FLASH_Lock(); // again, no recourse
							//HAL_FLASH_OB_Launch(); // reset occurs here (sorry, debugger)
					 }
				}
		 }
	}
}
//
void UnLockFlash(void)
{
	FLASH_OBProgramInitTypeDef OBInit; // programming option structure
	uint8_t mode = HAL_FLASHEx_OBGetUserData(OB_DATA_ADDRESS_DATA0); // from option byte 0
	//
	OBInit.DATAAddress = OB_DATA_ADDRESS_DATA0; // address of type FLASHEx_OB_Data_Address
	OBInit.DATAData = mode; // mode value to be saved
	OBInit.OptionType = OPTIONBYTE_RDP; // of type FLASHEx_OB_Type
	OBInit.RDPLevel = OB_RDP_LEVEL_0;

	 
	// unlock FLASH in general
	 
	if(HAL_FLASH_Unlock() == HAL_OK) 
	{
		 // unlock option bytes in particular
		 if(HAL_FLASH_OB_Unlock() == HAL_OK) 
		 {
				// erase option bytes before programming
				if(HAL_FLASHEx_OBErase() == HAL_OK) 
				{
					 // program selected option byte
					 HAL_FLASHEx_OBProgram(&OBInit); // result not checked as there is no recourse at this point
					 if(HAL_FLASH_OB_Lock() == HAL_OK) 
						{
							HAL_FLASH_Lock(); // again, no recourse
							//HAL_FLASH_OB_Launch(); // reset occurs here (sorry, debugger)
					 }
				}
		 }
	}
}
