/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32f4xx_hal_msp.c
  * @brief        Inicialización global de bajo nivel del microcontrolador.
  *
  * MSP significa:
  *
  *   MCU Support Package
  *
  * Este archivo contiene la inicialización global requerida por la
  * biblioteca HAL antes de configurar los periféricos concretos.
  *
  * En este proyecto realiza principalmente:
  *
  *   - Habilitación del reloj de SYSCFG.
  *   - Habilitación del reloj del bloque de alimentación PWR.
  *   - Configuración de la prioridad de la excepción PendSV.
  *
  * La configuración específica de GPIO, I2C, temporizadores y UART está
  * distribuida en otros archivos generados, por ejemplo:
  *
  *   gpio.c
  *   i2c.c
  *   tim.c
  *   usart.c
  *
  * Las regiones USER CODE BEGIN / USER CODE END son conservadas por
  * STM32CubeMX al regenerar el proyecto.
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
/* USER CODE BEGIN Includes */

/*
 * Espacio reservado para incluir librerías adicionales necesarias
 * únicamente en este archivo.
 */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/*
 * Espacio reservado para tipos privados.
 *
 * Ejemplos:
 *
 *   typedef struct
 *   typedef enum
 */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/*
 * Espacio reservado para constantes privadas definidas con #define.
 */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/*
 * Espacio reservado para macros privadas.
 */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/*
 * Espacio reservado para variables privadas de este módulo.
 */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/*
 * Espacio reservado para prototipos de funciones privadas.
 */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/*
 * Espacio reservado para declarar funciones implementadas en otros archivos
 * que deban utilizarse dentro de este módulo.
 */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/*
 * Espacio reservado para funciones auxiliares o código privado anterior
 * a HAL_MspInit().
 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /*
   * Espacio reservado para código que deba ejecutarse antes de la
   * inicialización global generada por STM32CubeMX.
   *
   * Esta función se ejecuta durante HAL_Init(), antes de iniciar FreeRTOS.
   * Por tanto, aquí no deben utilizarse funciones como osDelay().
   */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  /* System interrupt init*/
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

  /* USER CODE BEGIN MspInit 1 */

  /*
   * Espacio reservado para código posterior a la inicialización global.
   *
   * Aquí podrían configurarse otras prioridades globales o recursos
   * comunes del sistema.
   */

  /* USER CODE END MspInit 1 */
}

/* USER CODE BEGIN 1 */

/*
 * Espacio reservado para funciones adicionales.
 */

/* USER CODE END 1 */
