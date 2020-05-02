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
#include "aes.h"
#include "aestst.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
	unsigned short TypeData; // 2 byte
	unsigned short Year;
	unsigned short Month;
	unsigned short Date;
	unsigned short Hour;
	unsigned short Minute;
	unsigned short Secon;
	unsigned short Week;
} DateTime ; // 16 byte
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

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
_Data_UserTypeDef mSettingData;
RTC_DateTypeDef RTC_DateStruct;
RTC_TimeTypeDef RTC_TimeStruct;
DateTime DatetimeData;
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
	uint8_t *DataPoint;
	uint16_t TimeDelay = 0;
	//AesCtx ctx;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_IWDG_Init();
  MX_RTC_Init();
  MX_USB_DEVICE_Init();
	MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_RTCEx_SetSecond_IT(&hrtc);
	printf("App Start\r\n");
	//
	DataPoint = (uint8_t *)&mSettingData;
	// read memory
	for(int i = 0; i < sizeof(mSettingData); i++)
	{
		*DataPoint = ReadFlashByte(FLASH_BOOT_COMMAND + i);
		DataPoint++;
	}
	mSettingData.nullChar = 0;
#if defined(DEBUG_MODE)
	printf("Pass save: %s\r\n",mSettingData.PasswordSave);
	printf("CPU ID: %s\r\n",mSettingData.CPUid);
	printf("PC NAME: %s\r\n",mSettingData.PCname);
	printf("mData.InstallDate = %lld\r\n",mSettingData.InstallDate);
	printf("mData.LastDateUsing = %lld\r\n",mSettingData.LastDateUsing);
#endif
	/* Init AES */
	printf("Start Test AES:\r\n");
	TestAES();
	// INIT SHA
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	sha256_Init(&sha256);
	sha256_Update(&sha256, "0123456799", 10);
	sha256_Final(&sha256,hash);
	//
	printf("SHA = ");
	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
			printf("%02X ",hash[i]);
	}
	//
	printf("\r\n");
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
				sprintf((char*)usb_TX_buffer,"KEY_MODE\r\n");
				USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,usb_TX_buffer,64);
			}
		}
		//
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
  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef DateToUpdate;
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
	sTime.Hours = 1;
  sTime.Minutes = 0;
  sTime.Seconds = 0;

  HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 1;
  DateToUpdate.Year = 20;

  HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN);
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
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
	}
}
//
void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc)
{
	HAL_RTC_GetTime(hrtc, &RTC_TimeStruct, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(hrtc, &RTC_DateStruct, RTC_FORMAT_BIN);
	DatetimeData.Year = RTC_DateStruct.Year + 2000;
	DatetimeData.Month = RTC_DateStruct.Month;
	DatetimeData.Date = RTC_DateStruct.Date;
	DatetimeData.Hour = RTC_TimeStruct.Hours;
	DatetimeData.Minute = RTC_TimeStruct.Minutes;
	DatetimeData.Secon = RTC_TimeStruct.Seconds;
	DatetimeData.Week = RTC_DateStruct.WeekDay;
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
