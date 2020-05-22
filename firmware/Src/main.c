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
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <math.h>
#include <stdio.h>

#include "EMA_crush.h"
#include "statemachine.h"
#include "keyscan.h"
#include "button.h"
#include "ssd1306.h"
#include "crc16_modbus.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#ifdef _MSC_VER
    #pragma pack(push)
    #pragma pack(1)
    #define PACKED
#elif __GNUC__
    #define PACKED __attribute__((__packed__))
#else
    #error packed structs not implemented
#endif
typedef struct {
    uint16_t SOF; // 0x5555
    uint8_t version; // 0
    uint8_t type; // Packet type: 0
    uint16_t vfb; // ADC value
    uint32_t airflow; // Airflow, 1/100s of l/m
    uint32_t pressure; // Pressure, pascals (1/100s of mbar)
    uint32_t temperature; // Temperature, 1/100s of C
    uint16_t crc; // TODO
} PACKED message_packet_t;
long unsigned int gpktcount=0; 
long unsigned int gpktlasttimeticks=0; 
long unsigned int gpktlatency=0; 
#define kbuttoncount 2
#define kbutton_sel 0
#define kbutton_cal 1
keyscan_t buttonstates[ kbuttoncount];

void button_keyscan_init() {
	for (int i=0; i< kbuttoncount; i++) { keyscan_init(&buttonstates[i]); }
}
void button_keyscan_scan() {
	uint32 ticks = HAL_GetTick();
	keyscan(ticks,&buttonstates[kbutton_sel], ((GPIOA->IDR & 0x01) == 0) );
	keyscan(ticks,&buttonstates[kbutton_cal], ((GPIOA->IDR & 0x02) == 0) );
}

#ifdef _MSC_VER
    #pragma pack(pop)
#endif

float vfb = 0;
float temperature = 0;
float pressure = 0;
float airflow = 0;

void handle_byte(char data) {
    static char buf[30];  // must be >= sizeof(message_packet_t)
    static int pos = 0;

    switch(pos) {
        case 0: // SOF 1
            if(data == 0x55)
                buf[pos++] = data;
            else {
//                qDebug() << "SOF1 error";
                pos = 0;
            }
        break;
    case 1: // SOF 2
        if(data == 0x55)
            buf[pos++] = data;
        else {
//            qDebug() << "SOF2 error";
            pos = 0;
        }
        break;
    case 2: // version
        if(data == 0)
            buf[pos++] = data;
        else {
//            qDebug() << "Version error";
            pos = 0;
        }
        break;
    case 3: // packet type
        if(data == 0)
            buf[pos++] = data;
        else {
//            qDebug() << "Packet type error";
            pos = 0;
        }
        break;
    default:
        buf[pos++] = data;

        // Note: Packet type set in packet[3], but we only handle one type.
        if(pos == sizeof(message_packet_t)) {

            message_packet_t *packet = (message_packet_t*)buf;

            const uint16_t crc_received = packet->crc;
            packet->crc = 0;
            const uint16_t crc_calculated = crc16_modbus((uint8_t*)buf, sizeof(message_packet_t));

            if(crc_received != crc_calculated) {
//                qDebug() << "Bad CRC, got:" << crc_received << " expected:" << crc_calculated;
                pos = 0;
            }
            else {
		vfb = packet->vfb / 1000.0; // Voltage is in mv
                temperature = packet->temperature / 100.0; // temperature is in 1/100s of a C
                pressure = packet->pressure / 100.0; // pressure is pascals (1/100s of mbar)
                airflow = packet->airflow / 100.0; // Airflow is in 1/100s of l/m
                gpktcount ++;
                //EMA_crush((HAL_GetTick() - gpktlasttimeticks ) , gpktlatency, 5);
                gpktlatency = HAL_GetTick() - gpktlasttimeticks;
                gpktlasttimeticks = HAL_GetTick();


                pos = 0;
            }
        }
        else if(pos > sizeof(message_packet_t)) {
//            qDebug() << "oversize error";
            pos = 0;
        }

        break;
    }
}

/* USER CODE END 0 */


RTC_TimeTypeDef gtime; 
void clockinit () {
  gtime.Hours = 6;
  gtime.Minutes = 66;
  gtime.Seconds = 6;
  gtime.TimeFormat = RTC_HOURFORMAT12_PM;
  gtime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  gtime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &gtime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
	
}
void clockstuffage () {

    char buf[23];
    button_keyscan_scan();
    uint32 ticks=HAL_GetTick();
    snprintf(buf, sizeof(buf), "sel:%i, cal:%i hz:%lu", keyscan_duration(ticks,&buttonstates[kbutton_sel]),keyscan_duration(ticks,&buttonstates[kbutton_cal]),  gpktlatency);
    ssd1306_SetCursor(2, 48);
    ssd1306_WriteString(buf, Font_6x8, White);
}
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  char input;

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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  LL_USART_EnableIT_RXNE (USART1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  //init();
  ssd1306_Init();
  ssd1306_SetCursor(2, 0);
//  HAL_Delay(1000);
  ssd1306_WriteString("Hello", Font_11x18, White);
  ssd1306_UpdateScreen();
  clockinit();
  button_keyscan_init (); 

//  HAL_UART_RegisterCallback(&huart1, HAL_UART_RX_COMPLETE_CB_ID, HAL_UART_RxCpltCallback);
//  HAL_UART_Receive_IT(&huart1, rx_buf, sizeof(rx_buf));

  while (1)
  {
//    HAL_UART_Transmit(&huart1, msg, sizeof(msg), 100);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    input = get_key(); // read pushbuttons


    switch(input) {
    case KEY_SELECT:
    {
        // Cycle LED colors
        static int led_mode = 0;
        switch(led_mode) {
        case 0:
            GPIOB -> ODR &= ~LED_B_Pin;
            GPIOB -> ODR |= LED_R_Pin;
            break;
        case 1:
            GPIOB -> ODR &= ~LED_R_Pin;
            GPIOB -> ODR |= LED_G_Pin;
            break;
        case 2:
            GPIOB -> ODR &= ~LED_G_Pin;
            GPIOB -> ODR |= LED_B_Pin;
            break;
        case 3:
            GPIOB -> ODR |= LED_R_Pin;
            GPIOB -> ODR |= LED_G_Pin;
            GPIOB -> ODR |= LED_B_Pin;
            break;
        case 4:
            GPIOB -> ODR &= ~LED_R_Pin;
            GPIOB -> ODR &= ~LED_G_Pin;
            GPIOB -> ODR &= ~LED_B_Pin;
            break;
        }
        led_mode = (led_mode + 1) % 5;
    }
        break;
    case KEY_CAL:
    {
        // Buzzzzz
        for(unsigned long i = 0; i < 120; i++) {
            GPIOA -> ODR ^= PIEZO_Pin;
            HAL_Delay(1);
        }
        GPIOA -> ODR &= ~PIEZO_Pin;
    }
        break;
    }

//    char Rx_data[100];
//    const int received = HAL_UART_Receive (&huart1, &Rx_data, sizeof(Rx_data), 99999);


    // Update display
    ssd1306_Fill(Black);

    char buf[20];

    snprintf(buf, sizeof(buf), "Vfb:%1.3fV", vfb);
    ssd1306_SetCursor(2, 0);
    ssd1306_WriteString(buf, Font_7x10, White);

    snprintf(buf, sizeof(buf), "F:%1.1fl/m", airflow);
    ssd1306_SetCursor(2, 12);
    ssd1306_WriteString(buf, Font_7x10, White);

    snprintf(buf, sizeof(buf), "T:%1.1fC", temperature);
    ssd1306_SetCursor(2, 24);
    ssd1306_WriteString(buf, Font_7x10, White);

    snprintf(buf, sizeof(buf), "P:%1.3fmbar", pressure);
    ssd1306_SetCursor(2, 36);
    ssd1306_WriteString(buf, Font_7x10, White);

    
    snprintf(buf, sizeof(buf), "T:%1.3fmbar", pressure);
    ssd1306_SetCursor(2, 36);
    ssd1306_WriteString(buf, Font_7x10, White);
    clockstuffage (); 

    ssd1306_UpdateScreen();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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
