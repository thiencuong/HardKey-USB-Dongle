#include <Flash.h>
#include "memory.h"
#include <desig.h>
#include <stdio.h>
#include <string.h>
//
static FLASH_EraseInitTypeDef EraseInitStruct;
uint32_t FirstSector = 0, NbOfSectors = 0;
uint32_t Address = 0, PAGEError = 0;
extern _Data_UserTypeDef mSettingData;
//
uint8_t ReadFlashByte(uint32_t address)
{
	return MMIO8(address);
}
uint16_t ReadFlashHalfWord(uint32_t address)
{
	return MMIO16(address);
}
uint32_t ReadFlashWord(uint32_t address)
{
	return MMIO32(address);
}
uint64_t ReadFlashDoubleWord(uint32_t address)
{
	return MMIO64(address);
}
//
void ProgramFlash(uint8_t *DataFlash, uint32_t StartAddress, uint32_t Length)
{
	uint32_t* DataWrite = (uint32_t*)DataFlash;
	uint8_t * DataSource = DataFlash;
	int timeLoop = 0;
	//
	Address = StartAddress;
  while (Address < (StartAddress + Length))
  {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, *DataWrite) == HAL_OK)
    {
			DataWrite ++;
      Address += 4;
			timeLoop++;
    }
		else
    {
      /* Error occurred while writing data in Flash memory.
         User can add here some code to deal with this error */
				HAL_Delay(1000);
				HAL_NVIC_SystemReset();
    }
  }
}
// xoa bo nho user flast de ghi chuong trinh moi
void UserFlashErase(void)
{
	int TimeDelay = 0;
	  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();
	//
	  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
  EraseInitStruct.NbPages     = (FLASH_USER_END_ADDR - FLASH_USER_START_ADDR) / FLASH_PAGE_SIZE;
	
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    /*
      Error occurred while sector erase.
      User can add here some code to deal with this error.
      SECTORError will contain the faulty sector and then to know the code error on this sector,
      user can call function 'HAL_FLASH_GetError()'
    */
    /* Infinite loop */
    while (1)
    {
      /* Make LED1 blink (100ms on, 2s off) to indicate error in Erase operation */
      HAL_Delay(30);
			TimeDelay++;
			if(TimeDelay > 400)
			{
				TimeDelay = 0;
				HAL_NVIC_SystemReset();
			}
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_9);
    }
  }
	/* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
}
//
uint8_t FlagSaveFlash(uint16_t BootCommand, uint16_t FirmwareAlready)
{
	int TimeDelay = 0;
	uint16_t FirmwareCheck = 0;
	uint8_t DataTmp = 0;
	_Data_UserTypeDef mData;
	uint8_t *DataPoint;
	
	if(FirmwareAlready == 'R')
	{
		// Kiem tra xem da co firmware chua. Check 1024 byte first, if no data mean no firmware
		for(int i = 0; i < 1024; i++)
		{
			DataTmp = ReadFlashByte(FLASH_APP_START + i);
			if(DataTmp == 0 || DataTmp == 255)
			{
				FirmwareCheck++;
			}
		}
		if(FirmwareCheck == 1024)
		{
			return 1;
		}
	}
	//
	DataPoint = (uint8_t *)&mData;
	for(int i = 0; i < sizeof(mData); i++)
	{
		*DataPoint = ReadFlashByte(FLASH_BOOT_COMMAND + i);
		DataPoint++;
	}
	//
	mData.nullChar = 0;
	if(BootCommand > 0)
	{
		mData.BootCommand = BootCommand;
	}
	if(FirmwareAlready > 0)
	{
		mData.FirmwareAlready = FirmwareAlready;
	}
	// ======================================================================
	HAL_FLASH_Unlock();
	// xoa bo nho
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_BOOT_COMMAND; // bat dau xoa 1 page tu dia chi luu tru
  EraseInitStruct.NbPages     = 1; // xoa 1 page
	
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    /*
      Error occurred while sector erase.
      User can add here some code to deal with this error.
      SECTORError will contain the faulty sector and then to know the code error on this sector,
      user can call function 'HAL_FLASH_GetError()'
    */
    /* Infinite loop */
    while (1)
    {
      /* Make LED1 blink (100ms on, 2s off) to indicate error in Erase operation */
      HAL_Delay(30);
			TimeDelay++;
			if(TimeDelay > 400)
			{
				TimeDelay = 0;
				HAL_NVIC_SystemReset();
			}
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_9);
    }
  }
	//=================================================================================
	mData.nullChar = 0;
	Address = FLASH_BOOT_COMMAND;
	uint32_t *DataWrite = (uint32_t *)&mData;
	for(int i=0; i < (sizeof(mData)/4); i++)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, *DataWrite) == HAL_OK)
    {
			DataWrite ++;
      Address += 4;
    }
		else
    {
#if defined(DEBUG_MODE)
			printf("Error when save password to Flash");
#endif
    }
	}
	//
	HAL_FLASH_Lock();
	return 0;
}
//
void SaveCPUid(uint8_t *CPUid, int Len)
{
	_Data_UserTypeDef mData;
	uint8_t *DataPoint;
	uint8_t ShaTmp[4];
	uint8_t SHAdata[65];
	uint16_t TimeDelay = 0;
	//
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	//
	DataPoint = (uint8_t *)&mData;
	// read memory
	for(int i = 0; i < sizeof(mData); i++)
	{
		*DataPoint = ReadFlashByte(FLASH_BOOT_COMMAND + i);
		DataPoint++;
	}
	mData.nullChar = 0;
	//
	sha256_Init(&sha256);
	sha256_Update(&sha256, CPUid, Len);
	sha256_Final(&sha256,hash);
	//
	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf((char*)ShaTmp,"%02X",hash[i]);
		SHAdata[i*2] = ShaTmp[0];
		SHAdata[i*2 + 1] = ShaTmp[1];
	}
	SHAdata[64] = 0;
	memcpy(mData.CPUid,SHAdata,64);
	//=========================================================================================
	HAL_FLASH_Unlock();
	// xoa bo nho
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_BOOT_COMMAND; // bat dau xoa 1 page tu dia chi luu tru
  EraseInitStruct.NbPages     = 1; // xoa 1 page
	
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    /*
      Error occurred while sector erase.
      User can add here some code to deal with this error.
      SECTORError will contain the faulty sector and then to know the code error on this sector,
      user can call function 'HAL_FLASH_GetError()'
    */
    /* Infinite loop */
    while (1)
    {
      /* Make LED1 blink (100ms on, 2s off) to indicate error in Erase operation */
      HAL_Delay(30);
			TimeDelay++;
			if(TimeDelay > 400)
			{
				TimeDelay = 0;
				HAL_NVIC_SystemReset();
			}
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_9);
    }
  }
	//=================================================================================
	mData.nullChar = 0;
	Address = FLASH_BOOT_COMMAND;
	uint32_t *DataWrite = (uint32_t *)&mData;;
	for(int i=0; i < (sizeof(mData)/4); i++)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, *DataWrite) == HAL_OK)
    {
			DataWrite ++;
      Address += 4;
    }
		else
    {
#if defined(DEBUG_MODE)
			printf("Error when save password to Flash");
#endif
    }
	}
	//
	HAL_FLASH_Lock();
}
// save PC Name
void SavePCname(uint8_t *PCname, int Len)
{
	_Data_UserTypeDef mData;
	uint8_t *DataPoint;
	uint16_t TimeDelay = 0;
	//
	//
	DataPoint = (uint8_t *)&mData;
	// read memory
	for(int i = 0; i < sizeof(mData); i++)
	{
		*DataPoint = ReadFlashByte(FLASH_BOOT_COMMAND + i);
		DataPoint++;
	}
	mData.nullChar = 0;
	//
	memcpy(mData.PCname,PCname,Len);
	mData.PCname[Len] = 0;
	//=========================================================================================
	HAL_FLASH_Unlock();
	// xoa bo nho
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_BOOT_COMMAND; // bat dau xoa 1 page tu dia chi luu tru
  EraseInitStruct.NbPages     = 1; // xoa 1 page
	
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    /*
      Error occurred while sector erase.
      User can add here some code to deal with this error.
      SECTORError will contain the faulty sector and then to know the code error on this sector,
      user can call function 'HAL_FLASH_GetError()'
    */
    /* Infinite loop */
    while (1)
    {
      /* Make LED1 blink (100ms on, 2s off) to indicate error in Erase operation */
      HAL_Delay(30);
			TimeDelay++;
			if(TimeDelay > 400)
			{
				TimeDelay = 0;
				HAL_NVIC_SystemReset();
			}
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_9);
    }
  }
	//=================================================================================
	mData.nullChar = 0;
	Address = FLASH_BOOT_COMMAND;
	uint32_t *DataWrite = (uint32_t *)&mData;
	for(int i=0; i < (sizeof(mData)/4); i++)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, *DataWrite) == HAL_OK)
    {
			DataWrite ++;
      Address += 4;
    }
		else
    {
#if defined(DEBUG_MODE)
			printf("Error when save password to Flash");
#endif
    }
	}
	//
	HAL_FLASH_Lock();
}
// update install date
uint8_t UpdateLastUsingDate(uint8_t *DataUsb)
{
	_Data_UserTypeDef mData;
	uint8_t *DataPoint;
	uint16_t TimeDelay = 0;
	uint64_t UsingDate =((uint64_t) DataUsb[12] << 56) +  ((uint64_t) DataUsb[11] << 48) + ((uint64_t) DataUsb[10] << 40) +  ((uint64_t) DataUsb[9] << 32) + ((uint32_t) DataUsb[8] << 24) + (DataUsb[7] << 16) + (DataUsb[6] << 8) + DataUsb[5];;
	//
	//
	DataPoint = (uint8_t *)&mData;
	// read memory
	for(int i = 0; i < sizeof(mData); i++)
	{
		*DataPoint = ReadFlashByte(FLASH_BOOT_COMMAND + i);
		DataPoint++;
	}
	mData.nullChar = 0;
	//
#if defined(DEBUG_MODE)
	printf("Data R = %d - %d - %d - %d - %d - %d - %d - %d \r\n",DataUsb[0],DataUsb[1],DataUsb[2],DataUsb[3],DataUsb[4],DataUsb[5],DataUsb[6],DataUsb[7]);
	printf("mData.InstallDate(Flash) = %lld\r\n",mData.InstallDate);
	printf("UsingDate = %lld\r\n",UsingDate);
	printf("UsingDate Check = %lld\r\n",UsingDate - mData.LastDateUsing);
#endif
	if(mData.InstallDate == 0 || mData.InstallDate == 0XFFFFFFFFFFFFFFFF)
	{
		mData.InstallDate = UsingDate;
		mData.LastDateUsing = UsingDate;
#if defined(DEBUG_MODE)
			printf("Default Data, setup install date\r\n");
#endif
	}
	else
	{
		if(UsingDate + 86400 > mData.LastDateUsing)
		{
			mData.LastDateUsing = UsingDate;
#if defined(DEBUG_MODE)
			printf("Update LastUsingdate\r\n");
#endif
		}
		else
		{
#if defined(DEBUG_MODE)
			printf("Not Update LastUsingdate this time\r\n");
#endif
			return 1;
		}
	}
	//=========================================================================================
	HAL_FLASH_Unlock();
	// xoa bo nho
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.PageAddress = FLASH_BOOT_COMMAND; // bat dau xoa 1 page tu dia chi luu tru
  EraseInitStruct.NbPages     = 1; // xoa 1 page
	
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
  {
    /*
      Error occurred while sector erase.
      User can add here some code to deal with this error.
      SECTORError will contain the faulty sector and then to know the code error on this sector,
      user can call function 'HAL_FLASH_GetError()'
    */
    /* Infinite loop */
    while (1)
    {
      /* Make LED1 blink (100ms on, 2s off) to indicate error in Erase operation */
      HAL_Delay(30);
			TimeDelay++;
			if(TimeDelay > 400)
			{
				TimeDelay = 0;
				HAL_NVIC_SystemReset();
			}
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_9);
    }
  }
	//=================================================================================
	mData.nullChar = 0;
	Address = FLASH_BOOT_COMMAND;
	uint32_t *DataWrite = (uint32_t *)&mData;
	for(int i=0; i < (sizeof(mData)/4); i++)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, *DataWrite) == HAL_OK)
    {
			DataWrite ++;
      Address += 4;
    }
		else
    {
#if defined(DEBUG_MODE)
			printf("Error when save password to Flash");
#endif
    }
	}
	//
	HAL_FLASH_Lock();
	return 0;
}
