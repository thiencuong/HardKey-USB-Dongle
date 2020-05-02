#include <Flash.h>
#include "memory.h"
#include <desig.h>
#include <stdio.h>
//
static FLASH_EraseInitTypeDef EraseInitStruct;
uint32_t FirstSector = 0, NbOfSectors = 0;
uint32_t Address = 0, PAGEError = 0;
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
	mData.nullChar = 0;
	//
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
// save password
void SavePassword(uint8_t *Password, uint8_t Len)
{
	int TimeDelay = 0;
	uint32_t ddrWrite = FLASH_BOOT_COMMAND;
	uint8_t ShaTmp[4];
	unsigned char hash[SHA256_DIGEST_LENGTH];
	_Data_UserTypeDef mData;
	uint8_t *DataPoint;
	//
	SHA256_CTX sha256;
	sha256_Init(&sha256);
	sha256_Update(&sha256, Password, Len);
	sha256_Final(&sha256,hash);
	//
	DataPoint = (uint8_t *)&mData;
	for(int i = 0; i < sizeof(mData); i++)
	{
		*DataPoint = ReadFlashByte(FLASH_BOOT_COMMAND + i);
		DataPoint++;
	}
	mData.nullChar = 0;
	//
	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
	{
		sprintf((char*)ShaTmp,"%02X",hash[i]);
		mData.PasswordSave[i*2] = ShaTmp[0];
		mData.PasswordSave[i*2 + 1] = ShaTmp[1];
	}
	// xoa bo nho
	HAL_FLASH_Unlock();
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
	//
#if defined(DEBUG_MODE)
	printf("Password Save: %s\r\n",Password);
	printf("Password SHA Save: %s\r\n",mData.PasswordSave);
#endif
	//
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
