/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "can.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "dvc_serialplot.h"
#include "dvc_motor.h"
#include "drv_math.h"

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

Class_Serialplot serialplot;
Class_Motor_C620 motor1;
Class_Motor_C620 motor2;
Class_Motor_C620 motor3;
Class_Motor_C620 motor4;
float Target_Angle, Now_Angle, Target_Omega, Now_Omega;
float torque=0,fx=0,target_torque=0,last_target_torque=0;

uint16_t Counter = 0;

static char Variable_Assignment_List[][SERIALPLOT_RX_VARIABLE_ASSIGNMENT_MAX_LENGTH] = {
    //电机调PID
	//角度
    "pa",
    "ia",
    "da",
	//速度环
    "po",
    "io",
    "do",
		"torque",
		"fx",
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief CAN报文回调函数
 *
 * @param Rx_Buffer CAN接收的信息结构体
 */
void CAN_Motor_Call_Back(Struct_CAN_Rx_Buffer *Rx_Buffer)
{
    switch (Rx_Buffer->Header.StdId)
    {
        case (0x201):
        {
            motor1.CAN_RxCpltCallback(Rx_Buffer->Data);
        }
				case (0x202):
        {
            motor2.CAN_RxCpltCallback(Rx_Buffer->Data);
        }
				case (0x203):
        {
            motor3.CAN_RxCpltCallback(Rx_Buffer->Data);
        }
				case (0x204):
        {
            motor4.CAN_RxCpltCallback(Rx_Buffer->Data);
        }
        break;
    }

}
//速度规划，让速度变化的平滑一点
//void torque_plan()
//{
//	torque=((target_torque-last_target_torque)/2*(sin(-PI/2+PI*Counter/50)+1)+last_target_torque);
//	if(Counter<=50)Counter++;
//}

void (*last_fxfunction)(float speed);//上次运行的状态

//前进
void move_front(float speed)
{
	motor1.Set_Target_Omega(speed);
	motor2.Set_Target_Omega(speed);
	motor3.Set_Target_Omega(-speed);
	motor4.Set_Target_Omega(-speed);
}

//后退
void move_back(float speed)
{
	motor1.Set_Target_Omega(speed);
	motor2.Set_Target_Omega(-speed);
	motor3.Set_Target_Omega(speed);
	motor4.Set_Target_Omega(speed);
}

//向左
void move_left(float speed)
{
	motor1.Set_Target_Omega(-speed);
	motor2.Set_Target_Omega(speed);
	motor3.Set_Target_Omega(speed);
	motor4.Set_Target_Omega(-speed);
}

//向右
void move_right(float speed)
{
	motor1.Set_Target_Omega(speed);
	motor2.Set_Target_Omega(-speed);
	motor3.Set_Target_Omega(-speed);
	motor4.Set_Target_Omega(speed);
}

//右转
void turn_right(float speed)
{
	motor1.Set_Target_Omega(speed);
	motor2.Set_Target_Omega(speed);
	motor3.Set_Target_Omega(speed);
	motor4.Set_Target_Omega(speed);
}

//左转
void turn_left(float speed)
{
	motor1.Set_Target_Omega(-speed);
	motor2.Set_Target_Omega(-speed);
	motor3.Set_Target_Omega(-speed);
	motor4.Set_Target_Omega(-speed);
}         

//停止
void stop(float speed)
{
	motor1.Set_Target_Omega(0);
	motor2.Set_Target_Omega(0);
	motor3.Set_Target_Omega(0);
	motor4.Set_Target_Omega(0);
}

//方向初始化
void Direction_Init(void)
{
	last_fxfunction=stop;
}

/**
 * @brief HAL库UART接收DMA空闲中断
 *
 * @param huart UART编号
 * @param Size 长度
 */
void UART_Serialplot_Call_Back(uint8_t *Buffer, uint16_t Length)
{
        serialplot.UART_RxCpltCallback(Buffer);
    switch (serialplot.Get_Variable_Index())
    {
        // 电机调PID
        case(0):
        {
            motor1.PID_Angle.Set_K_P(serialplot.Get_Variable_Value());
						motor2.PID_Angle.Set_K_P(serialplot.Get_Variable_Value());
						motor3.PID_Angle.Set_K_P(serialplot.Get_Variable_Value());
						motor4.PID_Angle.Set_K_P(serialplot.Get_Variable_Value());
					last_fxfunction(torque);
						
        }
        break;
        case(1):
        {
            motor1.PID_Angle.Set_K_I(serialplot.Get_Variable_Value());
						motor2.PID_Angle.Set_K_I(serialplot.Get_Variable_Value());
						motor3.PID_Angle.Set_K_I(serialplot.Get_Variable_Value());
						motor4.PID_Angle.Set_K_I(serialplot.Get_Variable_Value());
					last_fxfunction(torque);
        }
        break;
        case(2):
        {
            motor1.PID_Angle.Set_K_D(serialplot.Get_Variable_Value());
						motor2.PID_Angle.Set_K_D(serialplot.Get_Variable_Value());
						motor3.PID_Angle.Set_K_D(serialplot.Get_Variable_Value());
						motor4.PID_Angle.Set_K_D(serialplot.Get_Variable_Value());
					last_fxfunction(torque);
        }
        break;
        case(3):
        {
            motor1.PID_Omega.Set_K_P(serialplot.Get_Variable_Value());
						motor2.PID_Omega.Set_K_P(serialplot.Get_Variable_Value());
						motor3.PID_Omega.Set_K_P(serialplot.Get_Variable_Value());
						motor4.PID_Omega.Set_K_P(serialplot.Get_Variable_Value());
					last_fxfunction(torque);
					
        }
        break;
        case(4):
        {
            motor1.PID_Omega.Set_K_I(serialplot.Get_Variable_Value());
						motor2.PID_Omega.Set_K_I(serialplot.Get_Variable_Value());
						motor3.PID_Omega.Set_K_I(serialplot.Get_Variable_Value());
						motor4.PID_Omega.Set_K_I(serialplot.Get_Variable_Value());
					last_fxfunction(torque);
        }
        break;
        case(5):
        {
            motor1.PID_Omega.Set_K_D(serialplot.Get_Variable_Value());
						motor2.PID_Omega.Set_K_D(serialplot.Get_Variable_Value());
						motor3.PID_Omega.Set_K_D(serialplot.Get_Variable_Value());
						motor4.PID_Omega.Set_K_D(serialplot.Get_Variable_Value());
					last_fxfunction(torque);
        }
        break;
				case(6):
        {
//						Counter=0;
//						last_target_torque=target_torque;
//            target_torque=serialplot.Get_Variable_Value();
//						torque_plan();
					torque=serialplot.Get_Variable_Value();
						last_fxfunction(torque);
        }
        break;
				case(7):
        {
            fx=serialplot.Get_Variable_Value();
						if(fx==0.0f){stop(torque);last_fxfunction=stop;}
						else if(fx==1.0f){move_front(torque);		last_fxfunction=move_front;}
						else if(fx==2.0f){move_back(torque);		last_fxfunction=move_back;}
						else if(fx==3.0f){move_left(torque);		last_fxfunction=move_left;}
						else if(fx==4.0f){move_right(torque);		last_fxfunction=move_right;}
						else if(fx==5.0f){turn_left(torque);		last_fxfunction=turn_left;}
						else if(fx==6.0f){turn_right(torque);		last_fxfunction=turn_right;}
						else{last_fxfunction(torque);}
						
        }
        break;
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
    CAN_Init(&hcan1, CAN_Motor_Call_Back);
    UART_Init(&huart2, UART_Serialplot_Call_Back, SERIALPLOT_RX_VARIABLE_ASSIGNMENT_MAX_LENGTH);

    serialplot.Init(&huart2, 8, (char **)Variable_Assignment_List);
		
		Direction_Init();


    motor1.PID_Omega.Init(100.0f, 0.0f, 0.0f, 0.0f, 2500.0f, 2500.0f);
		motor2.PID_Omega.Init(100.0f, 0.0f, 0.0f, 0.0f, 2500.0f, 2500.0f);
		motor3.PID_Omega.Init(100.0f, 0.0f, 0.0f, 0.0f, 2500.0f, 2500.0f);
		motor4.PID_Omega.Init(100.0f, 0.0f, 0.0f, 0.0f, 2500.0f, 2500.0f);
		
    motor1.Init(&hcan1, CAN_Motor_ID_0x201, Control_Method_OMEGA, 1.0f);
		motor2.Init(&hcan1, CAN_Motor_ID_0x202, Control_Method_OMEGA, 1.0f);
		motor3.Init(&hcan1, CAN_Motor_ID_0x203, Control_Method_OMEGA, 1.0f);
		motor4.Init(&hcan1, CAN_Motor_ID_0x204, Control_Method_OMEGA, 1.0f);

//move_front(0);
last_fxfunction=move_front;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//				torque_plan();
//				last_fxfunction(torque);
				


        //串口绘图显示内容

//        Target_Angle = motor1.Get_Target_Angle();
//        Now_Angle = motor1.Get_Now_Angle();
//        Target_Omega = motor1.Get_Target_Omega();
//        Now_Omega = motor1.Get_Now_Omega();
//        serialplot.Set_Data(4, &Target_Angle, &Now_Angle, &Target_Omega, &Now_Omega);
//        serialplot.TIM_Write_PeriodElapsedCallback();

        //输出数据到电机
        motor1.TIM_PID_PeriodElapsedCallback();
				motor2.TIM_PID_PeriodElapsedCallback();
				motor3.TIM_PID_PeriodElapsedCallback();
				motor4.TIM_PID_PeriodElapsedCallback();

        //通信设备回调数据
        TIM_CAN_PeriodElapsedCallback();
        //TIM_UART_PeriodElapsedCallback();

        //延时1ms
        HAL_Delay(0);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
