/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "diskio.h"
#include "ff.h"
#include "ffconf.h"
#include "ff_gen_drv.h"
#include "stdio.h"
#include "epd1in54.h"
#include "epdif.h"
#include "epdpaint.h"
#include "imagedata.h"
#include <stdlib.h>
#include "DS3231.h"
#include "bq25890.h"
#include "calc_country_ID.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define COLORED      0
#define UNCOLORED    1

#define U_TRESHOLD_3	3550	//25%
#define U_TRESHOLD_2	3720	//50%
#define U_TRESHOLD_1	3930	//75%

#define RUN	1
#define TURNING_OFF	5
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

IWDG_HandleTypeDef hiwdg;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim17;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart5;

/* USER CODE BEGIN PV */
uint8_t RX_data [30];
uint8_t RX_Time [16];
uint64_t pow_const [] = {1, 16,	256, 4096, 65536, 1048576, 16777216, 268435456, 4294967296, 68719476736};
uint8_t rx_complete = 0;
uint8_t String_TAG_ID[30];
uint8_t BT_flag = 0;		/////////////////////////////////////////////////////////////////////////////////////////// 1 only for debug
uint8_t SD_flag = 0;
uint8_t TAG_ID [6][15];
uint8_t TAG_pointer = 0;
uint8_t disp_refresh = 1;
uint64_t sheep_ID;
uint32_t country_ID;
uint8_t LED_flag = 0;
uint8_t SD_error_flag = 0;

uint32_t timestamp_button = 0;
uint32_t timestamp_time = 0;
uint32_t timestamp_adc = 0;
uint32_t timestamp_bt_led = 0;
uint32_t timestamp_bt_state = 0;
uint32_t timestamp_LED = 0;


/****************************DISP*****************************************/
unsigned char* frame_buffer[5000]; // = (unsigned char*)malloc(EPD_WIDTH * EPD_HEIGHT / 8);
EPD epd;
Paint paint;
/****************************DISP*****************************************/
/****************************SD*******************************************/
Diskio_drvTypeDef SD_Driver;
FATFS SDFatFs;  /* File system object for SD card logical drive */
FIL MyFile;     /* File object */
char SDPath[4]; /* SD card logical drive path */
/****************************SD*******************************************/
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM17_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART5_UART_Init(void);
static void MX_IWDG_Init(void);
/* USER CODE BEGIN PFP */
uint8_t ascii_hex_to_dec (uint8_t znak)
{
	if ((znak > 47) && (znak < 58))
	{
		return (znak - 48);
	}
	if ((znak > 64) && (znak < 71))
	{
		return (znak - 55);
	}
}
void calc_sheep_id ()
{
	uint8_t p;
	uint64_t ID = 0;
	uint32_t country = 0;
	if (HAL_GPIO_ReadPin(READER_TYPE_GPIO_Port, READER_TYPE_Pin) == GPIO_PIN_SET)
	{	//small antenna reader
		if ((RX_data[0] == 2) && (RX_data[17] == 3))
		{
			for(p=0; p<10; p++)
			{
				ID = ID + ((ascii_hex_to_dec(RX_data[5+p]))*pow_const[9-p]);
			}
			sheep_ID = ID;
			//return number;
			for (p=0; p<4; p++)
			{
				country = country + ((ascii_hex_to_dec(RX_data[1+p]))*pow_const[3-p]);
			}
			country_ID = country;
		}
	}
	else
	{	//big antenna reader
		if ((RX_data[0] == 2) && (RX_data[29] == 3))
		{
			for(p=0; p<10; p++)
			{
				ID = ID + ((ascii_hex_to_dec(RX_data[10-p]))*pow_const[9-p]);
			}
			sheep_ID = ID;
			//return number;
			for (p=0; p<4; p++)
			{
				country = country + ((ascii_hex_to_dec(RX_data[14-p]))*pow_const[3-p]);
			}
			country_ID = country;
		}
	}
	//return 0;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	if (UartHandle->Instance == huart2.Instance)
	{
		//HAL_GPIO_TogglePin(LED_B_GPIO_Port, LED_B_Pin);
		rx_complete = 1;
	    if (HAL_GPIO_ReadPin(READER_TYPE_GPIO_Port, READER_TYPE_Pin) == GPIO_PIN_SET)
	    	HAL_UART_Receive_IT(&huart2, RX_data, 18);					//small antenna reader
	    else
	    	HAL_UART_Receive_IT(&huart2, RX_data, 30);					//big antenna reader
	}
	if (UartHandle->Instance == huart5.Instance)
	{	//Thh:mm_dd.mm.yy
		if ((RX_Time[0] == 'T') && (RX_Time[3]==':') && (RX_Time[9]=='.'))
		RealTime.seconds=0;
		RealTime.hours = ((RX_Time[1]-48)*10)+(RX_Time[2]-48);
		RealTime.minutes = ((RX_Time[4]-48)*10)+(RX_Time[5]-48);
		RealTime.date = ((RX_Time[7]-48)*10)+(RX_Time[8]-48);
		RealTime.month = ((RX_Time[10]-48)*10)+(RX_Time[11]-48);
		RealTime.year = ((RX_Time[13]-48)*10)+(RX_Time[14]-48);
		time_set(&hi2c1);
		time_alarm_reset(&hi2c1);
		time_oscilator_stop_flag_reset(&hi2c1);
		HAL_UART_Receive_IT(&huart5, RX_Time, 15);
	}
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == BUTTON_Pin)
	{
		timestamp_button = HAL_GetTick();
		HAL_GPIO_TogglePin(BACKLIGHT_GPIO_Port, BACKLIGHT_Pin);
	}
	if (GPIO_Pin == SD_CD_Pin)
	{
		disp_refresh = 1;
		if (HAL_GPIO_ReadPin(SD_CD_GPIO_Port, SD_CD_Pin) == GPIO_PIN_SET)
			SD_flag = 0;
		else
		{
			SD_flag = 1;
		}
	}
}
uint8_t number_to_string(uint64_t ID, uint32_t country_ID, uint8_t* string)
{
	uint8_t n, p, zero = 0;
	uint64_t z = 100000000000;

	//n = sprintf(string,"%d-%d-%d;", 1, 1, 1);
	put_country_ID(country_ID, string);
	n=2;

	for(p=0; p<12 ;p++)
	{
		string[n] = (ID/z)+48;
		if ((string[n] != 48))
			zero = 1;
		if (zero)
			n++;
		if (ID/z)
		{
			ID = ID%z;
		}

		z = z/10;
	}
	string[n] = 0;
	return n++; //pocet znakov
}
void Refresh_display()
{
	char string[12];
	uint16_t v_bat = 0;
	uint8_t h_pos = 0;

	//BT_Icon - 9x14
	//SD_Icon - 12x15
	//BAT0_Icon 31x15
	//BAT1_Icon  31x15
	//BAT2_Icon  31x15
	//BAT3_Icon 31x15

	timestamp_time = HAL_GetTick();
	v_bat = BQ25890_read_bat_voltage(&hi2c1);
	time_read(&hi2c1);

	Paint_Init(&paint, frame_buffer, epd.width, epd.height);
	Paint_Clear(&paint, UNCOLORED);
	Paint_DrawHorizontalLine(&paint, 0, 16, 200, COLORED);	//prva z hora horizontalna
	Paint_DrawHorizontalLine(&paint, 0, 182, 200, COLORED); //prva z dola horizontalna

	sprintf(string, "%d:%02d", RealTime.hours, RealTime.minutes);
	if (RealTime.hours < 10)
		h_pos = h_pos +11;
	Paint_DrawStringAt(&paint, 142+h_pos, 2, string, &Font16, COLORED);
	Paint_DrawStringAt(&paint, 6, 186, "V1.0", &Font12, COLORED);
	sprintf(string, "%d.%d.20%d", RealTime.date, RealTime.month, RealTime.year);
	h_pos = 0;
	if (RealTime.date < 10)
		h_pos = h_pos +11;
	if (RealTime.month < 10)
		h_pos = h_pos +11;
	Paint_DrawStringAt(&paint, 90+h_pos, 185, string, &Font16, COLORED);

	if (v_bat >= U_TRESHOLD_1)
		Paint_ImageFromMemory(&paint, BAT3_Icon, 3, 0, 31, 15);
	if ((v_bat >= U_TRESHOLD_2) && ((v_bat < U_TRESHOLD_1)))
		Paint_ImageFromMemory(&paint, BAT2_Icon, 3, 0, 31, 15);
	if ((v_bat >= U_TRESHOLD_3) && ((v_bat < U_TRESHOLD_2)))
		Paint_ImageFromMemory(&paint, BAT1_Icon, 3, 0, 31, 15);
	if (v_bat < U_TRESHOLD_3)
		Paint_ImageFromMemory(&paint, BAT0_Icon, 3, 0, 31, 15);
	if (BT_flag)
		Paint_ImageFromMemory(&paint, BT_Icon, 43, 0, 9, 14);
	if (SD_flag)
		Paint_ImageFromMemory(&paint, SD_Icon, 60, 0, 12, 15);
	if (SD_error_flag)
		Paint_DrawCharAt(&paint, 70, 0, '!', &Font20, COLORED);

	Paint_DrawStringAt(&paint, 0, 26, TAG_ID[TAG_pointer], &Font20, COLORED);
	Paint_DrawStringAt(&paint, 0, 52, TAG_ID[(TAG_pointer+5)%6], &Font20, COLORED);
	Paint_DrawStringAt(&paint, 0, 78, TAG_ID[(TAG_pointer+4)%6], &Font20, COLORED);
	Paint_DrawStringAt(&paint, 0, 104, TAG_ID[(TAG_pointer+3)%6], &Font20, COLORED);
	Paint_DrawStringAt(&paint, 0, 130, TAG_ID[(TAG_pointer+2)%6], &Font20, COLORED);
	Paint_DrawStringAt(&paint, 0, 156, TAG_ID[(TAG_pointer+1)%6], &Font20, COLORED);

	EPD_SetFrameMemory(&epd, frame_buffer, 0, 0, Paint_GetWidth(&paint), Paint_GetHeight(&paint));
	EPD_DisplayFrame(&epd);

}
void Set_Turn_Off_display()
{
		//sheep 200x152
		Paint_Init(&paint, frame_buffer, epd.width, epd.height);
		Paint_Clear(&paint, UNCOLORED);

		Paint_ImageFromMemory(&paint, Sheep, 0, 24, 200, 152);

		EPD_SetFrameMemory(&epd, frame_buffer, 0, 0, Paint_GetWidth(&paint), Paint_GetHeight(&paint));
		EPD_DisplayFrame(&epd);
		EPD_WaitUntilIdle(&epd);
}
void Turn_off_device()
{
	HAL_GPIO_WritePin(HOLD_EN_GPIO_Port, HOLD_EN_Pin, GPIO_PIN_RESET);	//hold power on
	HAL_Delay(500);														//wait for flip flop goes down
	HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);		//turn off power RED LED
	Set_Turn_Off_display();												//show power off display
	HAL_GPIO_WritePin(HOLD_EN_GPIO_Port, HOLD_EN_Pin, GPIO_PIN_SET);		//turn off step up
	HAL_Delay(100);
	NVIC_SystemReset();													//in case does not switch off, reset MCU
}
void SD_Write()
{
	FRESULT res;
	uint8_t write[27];//={"data, data\r\n"};//12:45:10;SK1234589ABC\r\n
	uint8_t write_end[]={"\r\n"};
	uint8_t x;
	char file_name[13];
	DWORD FileSize;
	uint32_t byteswritten;
	res = FATFS_LinkDriver(&SD_Driver, SDPath);			//Links a diskio driver and increments the number of active linked drivers
	res = f_mount(&SDFatFs, (TCHAR const*)SDPath, 1);

	time_read(&hi2c1);
	sprintf(file_name, "%d-%d-%d.csv", RealTime.date, RealTime.month, RealTime.year);
	x = sprintf(write, "%d:%d:%d;", RealTime.hours, RealTime.minutes, RealTime.seconds);

	res = f_open(&MyFile, file_name, FA_WRITE | FA_OPEN_ALWAYS);
	res = f_sync(&MyFile);
	FileSize = f_size(&MyFile);
	res = f_lseek(&MyFile,FileSize);
	res = f_write(&MyFile, write, x, (void *)&byteswritten);
	res = f_write(&MyFile, TAG_ID[TAG_pointer], strlen(TAG_ID[TAG_pointer]), (void *)&byteswritten);
	res = f_write(&MyFile, write_end, 2, (void *)&byteswritten);
	f_close(&MyFile);
	//res = f_mount(0, "", 0);
	if (res != FR_OK)
		SD_error_flag = 1;
}
void Turn_on_LED_buzzer()
{
	LED_flag = 1;
	HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
	timestamp_LED = HAL_GetTick();
	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
	//PWM start
}
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
	int8_t BT_flag_old=0;
	FRESULT res;
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
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM17_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART5_UART_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  __HAL_DBGMCU_FREEZE_IWDG();
  TAG_ID[0][0] = 0;			//set zero for clear screen
    TAG_ID[1][0] = 0;
    TAG_ID[2][0] = 0;
    TAG_ID[3][0] = 0;
    TAG_ID[4][0] = 0;
    TAG_ID[5][0] = 0;
    HAL_GPIO_WritePin(HOLD_EN_GPIO_Port, HOLD_EN_Pin, GPIO_PIN_SET);	//hold power off
    HAL_GPIO_WritePin(BT_KEY_GPIO_Port, BT_KEY_Pin, GPIO_PIN_SET);	//set BT to comunication mode
    BQ25890_start_ADC(&hi2c1);										//start ADC bat voltage
    HAL_Delay(100);													//wait for conversion stop
    HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);		//turn on power RED LED
    EPD_Init(&epd, lut_full_update);									//init disp
    HAL_UART_DeInit(&huart2);										//deinit uart2
    MX_USART2_UART_Init();										//init uart2   nejaky impicment, bez toho to nejde
    if (HAL_GPIO_ReadPin(READER_TYPE_GPIO_Port, READER_TYPE_Pin) == GPIO_PIN_SET)
    	HAL_UART_Receive_IT(&huart2, RX_data, 18);					//receive data from RFID reader over IT (small antenna reader)
    else
    	HAL_UART_Receive_IT(&huart2, RX_data, 30);					//big antenna reader
    HAL_UART_Receive_IT(&huart5, RX_Time, 15);					//receive data from BT over IT - time settings  //Thh:mm_dd.mm.yy
    if (HAL_GPIO_ReadPin(SD_CD_GPIO_Port, SD_CD_Pin) == GPIO_PIN_RESET)	//is the SD card inserted?
  	  SD_flag = 1;
    //HAL_IWDG_Refresh(&hiwdg);								//reset IWDG
    //HAL_Delay(500);
    //Refresh_display();										//refresh display

    //SD_Write();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
     {
    	  HAL_IWDG_Refresh(&hiwdg);								//reset IWDG
		  if ((HAL_GetTick() - timestamp_bt_led)>3000)									//every 2s check BT LED // old 50ms
		  {
			  timestamp_bt_led = HAL_GetTick();
			  if (HAL_GPIO_ReadPin(BT_STATE_GPIO_Port, BT_STATE_Pin)==GPIO_PIN_RESET)	// if turn off, set BT flag to 0
			  {
				  BT_flag = 0;
				  timestamp_bt_state = HAL_GetTick();
			  }
			  if ((HAL_GetTick() - timestamp_bt_state)>500)								//if 100s is on
				  BT_flag = 1;															//set BT flag to 1
		  }
		  if (BT_flag != BT_flag_old)													//in case of BT flag change, refresh display
		  {
			  disp_refresh = 1;
			  BT_flag_old = BT_flag;
		  }
		  if (rx_complete)							//rx complete
		  {
			  Turn_on_LED_buzzer();							//turn on signal LED and buzzer
			  rx_complete = 0;								//reset flag
			  TAG_pointer = (TAG_pointer+1)%6;				//shift circular buffer
			  calc_sheep_id();													//convert input data to int
			  number_to_string(sheep_ID, country_ID, TAG_ID[TAG_pointer]);	// convert int to string
			  HAL_UART_Transmit(&huart5, TAG_ID[TAG_pointer], strlen(TAG_ID[TAG_pointer]), 50);			//send TAG ID on BT
			  HAL_UART_Transmit(&huart5, "\n", 1, 50);													//send termination \n to BT
			  disp_refresh = 1;											//refresh display
			  if(SD_flag)												//if SD is inserted
				  SD_Write();											//sd write
		  }

		  if (HAL_GPIO_ReadPin(BUTTON_GPIO_Port, BUTTON_Pin)==GPIO_PIN_SET)
		  {
			  if ((HAL_GetTick() - timestamp_button)>2000)								//if power button is hold for 2 seconds
			  {
				  //HAL_IWDG_Refresh(&hiwdg);												//reset IWDG
				  Turn_off_device();													//turn off device
			  }
		  }
		  if ((HAL_GetTick() - timestamp_time)>300000)									//in case of 5 min of display inactivity
		  {
			  disp_refresh = 1;															//refresh display
			  timestamp_time = HAL_GetTick();
		  }

		  if ((HAL_GetTick() - timestamp_adc)>1000)										//start bat voltage ADC every 1 second
		  {
			  BQ25890_start_ADC(&hi2c1);
			  timestamp_adc = HAL_GetTick();
		  }
		  if (disp_refresh)
		  {
			  if (HAL_GPIO_ReadPin(BUSY_GPIO_Port, BUSY_Pin)==GPIO_PIN_RESET)			//do not refresh display until busy
			  {
				  disp_refresh = 0;
				  Refresh_display();
			  }
		  }
		  if (LED_flag && ((HAL_GetTick() - timestamp_LED)>300))						//after 300ms turn off signal LED and buzzer
		  {
			  LED_flag = 0;
			  HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
			  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
			  //PWM stop
		  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Window = 3124;
  hiwdg.Init.Reload = 3124;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM17 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM17_Init(void)
{

  /* USER CODE BEGIN TIM17_Init 0 */

  /* USER CODE END TIM17_Init 0 */

  /* USER CODE BEGIN TIM17_Init 1 */

  /* USER CODE END TIM17_Init 1 */
  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 0;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 50000;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM17_Init 2 */

  /* USER CODE END TIM17_Init 2 */

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
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART5_UART_Init(void)
{

  /* USER CODE BEGIN USART5_Init 0 */

  /* USER CODE END USART5_Init 0 */

  /* USER CODE BEGIN USART5_Init 1 */

  /* USER CODE END USART5_Init 1 */
  huart5.Instance = USART5;
  huart5.Init.BaudRate = 9600;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART5_Init 2 */

  /* USER CODE END USART5_Init 2 */

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(EEPROM_WC_GPIO_Port, EEPROM_WC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, HOLD_EN_Pin|LED_B_Pin|SD_CS_Pin|RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SPI1_CS_Pin|DC_Pin|LED_R_Pin|BT_KEY_Pin
                          |BACKLIGHT_Pin|BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : EEPROM_WC_Pin */
  GPIO_InitStruct.Pin = EEPROM_WC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EEPROM_WC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : HOLD_EN_Pin LED_B_Pin SD_CS_Pin RST_Pin */
  GPIO_InitStruct.Pin = HOLD_EN_Pin|LED_B_Pin|SD_CS_Pin|RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_Pin */
  GPIO_InitStruct.Pin = BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SPI1_CS_Pin DC_Pin LED_R_Pin BT_KEY_Pin
                           BACKLIGHT_Pin BUZZER_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin|DC_Pin|LED_R_Pin|BT_KEY_Pin
                          |BACKLIGHT_Pin|BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : BUSY_Pin BT_STATE_Pin */
  GPIO_InitStruct.Pin = BUSY_Pin|BT_STATE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : READER_TYPE_Pin */
  GPIO_InitStruct.Pin = READER_TYPE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(READER_TYPE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CD_Pin */
  GPIO_InitStruct.Pin = SD_CD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SD_CD_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
