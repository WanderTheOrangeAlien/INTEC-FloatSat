/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Rutinas de servicio de interrupciones y excepciones.
  *
  * Este archivo contiene:
  *
  *   - Excepciones internas del núcleo ARM Cortex-M4.
  *   - Interrupciones de los periféricos habilitados.
  *
  * En la configuración actual, la única interrupción de periférico
  * implementada aquí es TIM6, utilizado como base de tiempo de HAL.
  *
  * Las regiones USER CODE BEGIN / USER CODE END son conservadas
  * cuando STM32CubeMX regenera el proyecto.
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
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/*
 * Espacio reservado para cabeceras adicionales necesarias
 * dentro de las interrupciones.
 */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/*
 * Espacio reservado para tipos privados.
 */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * Espacio reservado para constantes privadas.
 */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/*
 * Espacio reservado para macros privadas.
 */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/*
 * Espacio reservado para variables privadas.
 *
 * Las variables compartidas con interrupciones deberían declararse
 * volatile cuando puedan cambiar fuera del flujo normal del programa.
 */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/*
 * Espacio reservado para prototipos de funciones privadas.
 */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * Espacio reservado para funciones auxiliares utilizadas por
 * los manejadores de interrupción.
 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim6;

/* USER CODE BEGIN EV */

/*
 * Espacio reservado para otras variables externas.
 */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /*
   * Espacio para registrar o diagnosticar la causa del NMI.
   */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /*
   * La implementación actual no intenta recuperarse.
   *
   * El microcontrolador permanece detenido dentro de este bucle para
   * permitir inspeccionar el estado mediante el depurador.
   */
  while (1)
  {
  }

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /*
   * Aquí puede agregarse código para guardar registros del procesador
   * o leer los registros de diagnóstico SCB.
   */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */

    /*
     * Espacio para señalización de diagnóstico.
     *
     * Debe evitarse utilizar funciones dependientes de interrupciones,
     * del scheduler o de periféricos cuyo estado sea desconocido.
     */

    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /*
   * Espacio para diagnóstico previo al bloqueo.
   */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */

    /*
     * El código permanece detenido para impedir continuar después
     * de una violación de memoria.
     */

    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /*
   * Espacio para capturar información del fallo.
   */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */

    /*
     * No se intenta recuperar la ejecución.
     */

    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /*
   * Espacio para diagnóstico.
   */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */

    /*
     * Mantener detenido el sistema después del fallo.
     */

    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /*
   * Código opcional previo al manejo generado.
   */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /*
   * La implementación actual no realiza ninguna acción.
   */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles TIM6 global interrupt, DAC1 and DAC2 underrun error interrupts.
  */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /*
   * Código que se ejecutaría antes del manejador HAL.
   *
   * Este bloque se ejecuta en contexto de interrupción:
   *
   *   - No debe usarse osDelay().
   *   - No deben realizarse operaciones I2C bloqueantes.
   *   - El tiempo de ejecución debe mantenerse corto.
   */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /*
   * Código que se ejecutaría después del manejador HAL.
   *
   * También permanece en contexto de interrupción.
   */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/*
 * Espacio reservado para manejadores adicionales o funciones auxiliares
 * que deban conservarse después de regenerar el código.
 */

/* USER CODE END 1 */
