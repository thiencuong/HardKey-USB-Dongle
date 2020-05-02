#include "usb_process.h"
#include "memory.h"
#include "Flash.h"
#include <stdio.h>
//
uint8_t msg_id = 0;
uint8_t msg_size = 0;
uint32_t DataPost = 0;
extern uint8_t usb_TX_buffer;
extern int DataSendAlready; // dat bang 1 se truyen du lieu di
//
extern struct buttonState MyButton;
extern bool	RequestUpdate;
//
int UsbProcess(uint8_t *DataUsb)
{
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
	printf("ID=%d, Size=%d\r\n",msg_id,msg_size);
	printf("Post= 0x%X\r\n",DataPost);
	//
	switch(msg_id)
	{
		case 0x03:// Neu yeu cau thoat che do download
			RequestUpdate = true;
			printf("Start Mark start App\r\n");
			FlagSaveFlash(100,0);
			printf("Mark start App ok\r\n");
			DataSendAlready = 1;
			break;
		case 0x04:// yeu cau vao che do DownLoad
			RequestUpdate = true;
			printf("Start Clear Mark start App\r\n");
			FlagSaveFlash(200,0);
			printf("Clear Mark start App ok, Reset system\r\n");
			HAL_NVIC_SystemReset();
			break;
		case 0x05:// neu co yeu cau cap nhap thong tin firmware
			printf("Start Set firmware\r\n");
			FlagSaveFlash(100,'R');
			printf("Start Set firmware ok\r\n");
			DataSendAlready = 1;
			break;
		case 0x08:
			SaveCPUid(DataUsb+5,msg_size);
			DataSendAlready = 1;
			printf("Save CPU id OK\r\n");
			break;
		case 0x09:
			SavePCname(DataUsb+5,msg_size);
			DataSendAlready = 1;
			printf("Save PC Name OK\r\n");
			break;
		case 0x0A:
			UpdateLastUsingDate(DataUsb);
			DataSendAlready = 1;
			printf("Update Time OK\r\n");
			break;
		default:
			return 2;
			break;
	}
	return 0;
}
// Save data to flash
int FlashNewFirmware(uint8_t *DataFlash, uint32_t Address, char Len)
{
	uint8_t *DataToWrite = DataFlash + 9;
	//
	for(int jk = 0; jk < Len; jk++)
	{
		printf("%d-",*(DataToWrite + jk));
	}
	printf("\r\n->%d<-\r\n",*(DataToWrite));
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
