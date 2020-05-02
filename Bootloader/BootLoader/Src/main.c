/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_custom_hid_if.h"
#include "usb_process.h"
#include <stdio.h>
#include <Flash.h>
#include "memory.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
typedef  void (*pFunction)(void);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
static uint8_t usb_RX_buffer[64] __attribute__((aligned(4)));
uint8_t usb_TX_buffer[64];
static int DataReceiveAlready = 0; // neu =1 thi co nghia la du lieu trong usb_RX_buffer san sang
int DataSendAlready = 0; // neu set = 1 thi se tien hanh truyen du lieu di trong usb_TX_buffer
//
extern USBD_HandleTypeDef hUsbDeviceFS;
bool	RequestUpdate = false;
// Bien khai bao de nhay den chuong trinh
pFunction JumpToApplication;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_IWDG_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
uint8_t USBD_CUSTOM_HID_SendReport     (USBD_HandleTypeDef  *pdev, uint8_t *report, uint16_t len);
void USB_RX_Interrupt(void);
void SystickTimerInterrup(void);
void jump_to_app(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	uint8_t Password[65];
	int8_t CheckDefault = 0;
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	uint8_t LockFlashFlag = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	uint8_t ByteAppCommand = ReadFlashByte(FLASH_BOOT_COMMAND);
	uint8_t Firmware = ReadFlashByte(FIRMWARE_READY);
	// neu chua khoa read flash thi tien hanh khoa flash
	if(ReadFlashWord(OB_BASE) != 0xFFFFFF00)
	{
		//LockFlash();
		LockFlashFlag = 1;
	}
	//
	if(ByteAppCommand == 100 && Firmware == 'R')
	{
		jump_to_app();
	}
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_IWDG_Init();
  MX_RTC_Init();
  MX_USB_DEVICE_Init();
	MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
	memset((char*)usb_TX_buffer,0,64);
	printf("Bootloader Start: %d, %c\r\n",ByteAppCommand,Firmware);
	for(int i = 0; i < 64; i++)
	{
		Password[i] = ReadFlashByte(PASSWORD_SAVE + i);
		if(Password[i] == 0 || Password[i] == 255)
		{
			CheckDefault++;
		}
	}
	Password[64] = 0;
#if defined(DEBUG_MODE)
	printf("Pass save: %s\r\n",Password);
#endif

#if defined(DEBUG_MODE)
	printf("CheckDefault: %d\r\n", CheckDefault);
#endif
	// do not check password if password not set
	if(CheckDefault == 64)
	{
		goto PASSWORD_OK;
	}
	else
	{
		printf("Input Password to Download Mode\r\n");
		sprintf((char*)usb_TX_buffer,"PASSWORD_REQUEST\r\n");
		USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,usb_TX_buffer,64);
	}
	// start check password to download mode
	while(1)
	{
		if(DataReceiveAlready == 1)
		{
			DataReceiveAlready = 0;
			if(PasswordProcess(usb_RX_buffer,Password) == 0)
			{
				goto PASSWORD_OK;
			}
			else
			{
				USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,usb_TX_buffer,64);
				printf("Wrong Password\r\n");
			}
		}
	}
	PASSWORD_OK:
	memset((char*)usb_TX_buffer,0,64);
	sprintf((char*)usb_TX_buffer,"PASSWORD_OK\r\n");
	USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,usb_TX_buffer,64);
#if defined(DEBUG_MODE)
	printf("Enter Download Firmware\r\n");
	printf("Option Byte %X\r\n",ReadFlashWord(OB_BASE));
	printf("LockFlashFlag = %d\r\n",LockFlashFlag);
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if(DataReceiveAlready == 1)
		{
			DataReceiveAlready = 0;
			if(UsbProcess(usb_RX_buffer) == 2)
			{
				memset((char*)usb_TX_buffer,0,64);
				sprintf((char*)usb_TX_buffer,"WAIT COMMAND DOWNLOAD\r\n");
				USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,usb_TX_buffer,64);
			}
		}
		if(DataSendAlready != 0)
		{
			if(DataSendAlready == 2)
			{
				memset((char*)usb_TX_buffer,0,64);
				sprintf((char*)usb_TX_buffer,"FLASH_OK\r\n");
			}
			if(DataSendAlready == 1)
			{
				memset((char*)usb_TX_buffer,0,64);
				sprintf((char*)usb_TX_buffer,"SUCCESS\r\n");
			}
			USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,usb_TX_buffer,64);
			DataSendAlready = 0;
		}
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USB;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only 
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;//RTC_OUTPUTSOURCE_ALARM;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
int fputc( int ch, FILE *f )
{
	HAL_UART_Transmit(&huart1,(uint8_t *)&ch,1,300);
  return ch;
}
//
void USB_RX_Interrupt(void)
{
	int NumberByteReceive = 0;
	int iRun = 0;
	USBD_CUSTOM_HID_HandleTypeDef *myusb=(USBD_CUSTOM_HID_HandleTypeDef *)hUsbDeviceFS.pClassData;
	
	//Clear arr
	for(iRun=0;iRun<64;iRun++)
	{
		usb_RX_buffer[iRun]=0;
	}
	//myusb->Report_buf[0]= numbers of byte data
	//for(i=0;i<myusb->Report_buf[0];i++)
	NumberByteReceive = myusb->Report_buf[0];
	if(NumberByteReceive > 64)
	{
		NumberByteReceive = 64;
	}
	//
	if(NumberByteReceive > 0)
	{
		for(iRun=0;iRun<64;iRun++)
		{
			usb_RX_buffer[iRun]=myusb->Report_buf[iRun+1];
		}
		DataReceiveAlready = 1;
	}
}
// Ngat timer systick
int clearWatchDogTime = 0;
void SystickTimerInterrup(void)
{
	clearWatchDogTime++;
	if(clearWatchDogTime > 1000)
	{
		clearWatchDogTime = 0;
		HAL_IWDG_Refresh(&hiwdg);
		HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_9);
	}
}
//
void jump_to_app(void)
{
	/* If Program has been written */
	//if (((*(__IO uint32_t*)ApplicationAddress) & 0x2FFE0000 ) == 0x20000000)
	{
		// Disable all interrupts
		NVIC->ICER[0] = 0xFFFFFFFF;
		NVIC->ICER[1] = 0xFFFFFFFF;
		NVIC->ICER[2] = 0xFFFFFFFF;
		//
		// Clear pendings 
		NVIC->ICPR[0] = 0xFFFFFFFF;
		NVIC->ICPR[1] = 0xFFFFFFFF;
		NVIC->ICPR[2] = 0xFFFFFFFF;
		//
//		__HAL_RCC_GPIOC_CLK_DISABLE();
//		__HAL_RCC_GPIOD_CLK_DISABLE();
//		__HAL_RCC_GPIOA_CLK_DISABLE();
//		__HAL_RCC_GPIOB_CLK_DISABLE();
//		HAL_GPIO_DeInit(GPIOC,GPIO_PIN_13);
//		HAL_GPIO_DeInit(GPIOA,GPIO_PIN_9);
//		//
//		HAL_RTC_DeInit(&hrtc);
//		HAL_UART_DeInit(&huart1);
//		MX_USB_DEVICE_DeInit();
		//
		/* Disable Systick interrupt */
		SysTick->CTRL = 0;
		/* Initialize user application's Stack Pointer & Jump to user application */
		JumpToApplication = (pFunction) (*(__IO uint32_t*) (FLASH_APP_START + 4));
		__set_MSP(*(__IO uint32_t*) FLASH_APP_START);
		JumpToApplication();
	}
}
//
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
