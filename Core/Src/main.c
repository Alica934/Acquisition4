/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "can.h"
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <math.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define WINDOW 5
uint32_t ADC_VAL[3];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t time_ms = 0;

uint16_t Ready, Emergency, Ignition, Inercia;
uint16_t adc_st_angle;
uint16_t adcLeftSuspension;
uint16_t adcRightSuspension;
uint16_t angleCAN, leftSusCAN, rightSusCAN;
float tensao;

//Media movel
uint32_t buffer[3][WINDOW] = {0};
uint8_t  idx[3] = {0};



//CAN
CAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8];
uint32_t TxMailbox;

//Functions
void execute_immediate_tasks(void);
void execute_10ms_tasks(void);
void execute_50ms_tasks(void);
void execute_100ms_tasks(void);


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

int _write(int file, char *data, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t*) data, len, HAL_MAX_DELAY);
	return len;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) // necessario?------------------------------------------------------------------

{
  // Process the data
}

float MeasureSuspensionPosition(uint16_t bits);
float MeasureSteeringAngle(uint16_t bits);

uint32_t moving_average(uint32_t new_sample, uint8_t channel);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){
	if (htim->Instance == TIM7) {
	time_ms++;
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_DMA_Init();
  MX_CAN1_Init();
  MX_CAN2_Init();
  MX_USART1_UART_Init();
  MX_TIM7_Init();
  MX_IWDG_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim7);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  HAL_ADC_Start_DMA(&hadc1, ADC_VAL, 3);
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		//Execute immediate tasks
		execute_immediate_tasks();

		static uint32_t previus_tick_10ms = 0;
		static uint32_t previus_tick_50ms = 0;
		static uint32_t previus_tick_100ms = 0;

		//Execute 10ms Tasks
		if (time_ms - previus_tick_10ms >= 10) {
			execute_10ms_tasks();
			previus_tick_10ms = time_ms;
		}

		//Execute 50ms Tasks
		if (time_ms - previus_tick_50ms >= 50) {
			execute_50ms_tasks();
			previus_tick_50ms = time_ms;
		}

		//Execute 100ms Tasks
		if(time_ms - previus_tick_100ms >= 100){
			execute_100ms_tasks();
			previus_tick_100ms = time_ms;
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
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
}

/* USER CODE BEGIN 4 */
void execute_immediate_tasks() {

}
void execute_10ms_tasks() {
	HAL_IWDG_Refresh(&hiwdg);

	 	  Ready = HAL_GPIO_ReadPin(Ready_To_Drive_GPIO_Port, Ready_To_Drive_Pin);
		  Emergency = HAL_GPIO_ReadPin(Emergency_Button_GPIO_Port, Emergency_Button_Pin);
		  Inercia = HAL_GPIO_ReadPin(Inertia_Switch_GPIO_Port, Inertia_Switch_Pin);
		  Ignition = HAL_GPIO_ReadPin(Ignition_GPIO_Port, Ignition_Pin);
		  adc_st_angle = ADC_VAL[0];
		  adcLeftSuspension = ADC_VAL[1];
		  adcRightSuspension = ADC_VAL[2];
}
void execute_50ms_tasks() {

	//medias moveis
	angleCAN = moving_average(adc_st_angle, 0) * 10;
	leftSusCAN = moving_average(adcLeftSuspension, 1) * 10;
	rightSusCAN = moving_average(adcRightSuspension, 2) * 10;

	//CAN Message
	TxHeader.IDE = CAN_ID_STD;
	TxHeader.StdId = 0x710;
	TxHeader.RTR = CAN_RTR_DATA;
	TxHeader.DLC = 7;

	TxData[0] = angleCAN & 0xFF;              // Least significant byte first
	TxData[1] = (angleCAN >> 8) & 0xFF;             // Most significant byte
	TxData[2] = leftSusCAN & 0xFF;
	TxData[3] = (leftSusCAN >> 8) & 0xFF;
	TxData[4] = rightSusCAN & 0xFF;
	TxData[5] = (rightSusCAN >> 8) & 0xFF;
	TxData[6] =((Inercia & 0x1) | ((Emergency & 0x1) << 1)) & 0xFF;

	if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
		Error_Handler();
	}

	TxHeader.DLC = 2;
	TxData[0] = Ignition;
	TxData[1] = Ready;

	if (HAL_CAN_AddTxMessage(&hcan2, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
		Error_Handler();
	}


}
void execute_100ms_tasks() {
	HAL_GPIO_TogglePin(HEARTBEAT_GPIO_Port, HEARTBEAT_Pin); //HEARTBEAT
	//teste USART
	printf("Esta merda funfa\n");

	printf("Ignition %u\n", Ignition);
	printf("Ready to drive: %u\n", Ready);
	printf("Emergency %u\n", Emergency);
	printf("Inertia %u\n", Inercia);
	printf("Angle %.2f\n", MeasureSteeringAngle(adc_st_angle));
	printf("LeftSus %.2f\n", MeasureSuspensionPosition(adcLeftSuspension));
	printf("RightSus %.2f\n", MeasureSuspensionPosition(adcRightSuspension));

}

//---------------------------------------------------------Funcoes de medida----------------------------------------------//
float MeasureSuspensionPosition(uint16_t bits)
{
  // Constants and Variables
  float V_SUSP;                                    // Voltage Signal from sensor
  float sensor_voltage = 5;                       // MAX Voltage level from sensor
  float MCU_voltage = 3.3;                        // MAX MCU voltage level
  float Electrical_stroke = 75;                   // mm
  float SUSPENSION_POSITION;                      // Suspension level in mm
  float Conversion_Factor = MCU_voltage / sensor_voltage;
  float volts;                                    // converted voltage
  // Calculate Voltage from ADC
  V_SUSP = (MCU_voltage * bits) / 4095;
  volts = V_SUSP / Conversion_Factor;
  // Calculate Position
  SUSPENSION_POSITION = (Electrical_stroke * volts) / sensor_voltage;

  return SUSPENSION_POSITION;                     // return suspension level in millimeters
}

float MeasureSteeringAngle(uint16_t bits)
{
// Constants and variables
  const float ADC_MAX = 4095.0f;
  const float MCU_VREF = 3.3f;                    // MCU reference voltage
  const float SENSOR_VREF_Max = 4.5f;             // Maximum sensor reference voltage
  const float SENSOR_VREF_Min = 0.5f;             // Minimum sensor reference voltage
  const float Resolution = 180.0f;                // Resolution of sensor -180 to 180
  const float OFFSET = 0.0f;                    // In case of mechanical problems, put offset angle

  // Calculate Function Slope
  float max_v = SENSOR_VREF_Max * (2.0f/3.0f);
  float min_v = SENSOR_VREF_Min * (2.0f/3.0f);
  float inclination = 360.0f / (max_v - min_v);

  // Calculate Steering Angle
  float V_STA = (MCU_VREF * bits) / ADC_MAX;
  float ST_Angle = inclination * V_STA - 45 - Resolution;

  // If necessary add OFFSET
  ST_Angle += OFFSET;
  return ST_Angle;
}


uint32_t moving_average(uint32_t new_sample, uint8_t channel) {
    buffer[channel][idx[channel]] = new_sample;
    idx[channel] = (idx[channel] + 1) % WINDOW;

    uint32_t sum = 0;
    for (uint8_t i = 0; i < WINDOW; i++)
        sum += buffer[channel][i];

    return sum / WINDOW;
}






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
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
