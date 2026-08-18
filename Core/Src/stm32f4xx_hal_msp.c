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

/*
 * main.h proporciona:
 *
 *   - Definiciones generales de la biblioteca HAL.
 *   - Tipos del STM32F4.
 *   - Macros de control de reloj RCC.
 *   - Funciones de configuración NVIC.
 *   - Definiciones comunes del proyecto.
 */
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
  * @brief Inicializa los recursos MSP globales del STM32.
  *
  * Esta función es llamada automáticamente por HAL_Init(), al comienzo
  * de la ejecución del programa.
  *
  * No debe confundirse con las funciones específicas de periférico:
  *
  *   HAL_I2C_MspInit()
  *   HAL_TIM_PWM_MspInit()
  *   HAL_TIM_Encoder_MspInit()
  *   HAL_UART_MspInit()
  *
  * Esas funciones configuran los GPIO, relojes e interrupciones de cada
  * periférico individual.
  *
  * @retval None
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


  /* ------------------------------------------------------------------------ */
  /* Habilitación del reloj de SYSCFG                                        */
  /* ------------------------------------------------------------------------ */

  /*
   * Habilitar el reloj del bloque System Configuration Controller.
   *
   * SYSCFG participa en funciones como:
   *
   *   - Selección de fuentes para líneas EXTI.
   *   - Configuraciones especiales del sistema.
   *   - Mapeo de memoria.
   *   - Algunas opciones de compensación y conectividad.
   *
   * Es necesario, por ejemplo, para asociar un pin GPIO con una línea
   * de interrupción externa.
   */
  __HAL_RCC_SYSCFG_CLK_ENABLE();


  /* ------------------------------------------------------------------------ */
  /* Habilitación del reloj del bloque de alimentación                       */
  /* ------------------------------------------------------------------------ */

  /*
   * Habilitar el reloj del periférico PWR.
   *
   * El bloque PWR controla funciones relacionadas con:
   *
   *   - Regulador interno.
   *   - Modos de bajo consumo.
   *   - Wake-up.
   *   - Backup domain.
   *   - Escala de voltaje del regulador.
   *
   * En SystemClock_Config() se utiliza posteriormente para configurar:
   *
   *   PWR_REGULATOR_VOLTAGE_SCALE1
   */
  __HAL_RCC_PWR_CLK_ENABLE();


  /* ------------------------------------------------------------------------ */
  /* Configuración global de interrupciones                                  */
  /* ------------------------------------------------------------------------ */

  /*
   * PendSV es una excepción interna del núcleo ARM Cortex-M.
   *
   * FreeRTOS utiliza PendSV para realizar el cambio de contexto entre tareas.
   *
   * En términos simplificados:
   *
   *   1. El kernel decide que debe ejecutarse otra tarea.
   *   2. Se activa PendSV.
   *   3. PendSV guarda el contexto de la tarea actual.
   *   4. Restaura el contexto de la siguiente tarea.
   */

  /*
   * Configurar PendSV con prioridad 15.
   *
   * En el Cortex-M4 del STM32F407, un número de prioridad mayor representa
   * una prioridad lógica menor.
   *
   * Por tanto:
   *
   *   prioridad 0  = prioridad más alta;
   *   prioridad 15 = prioridad más baja.
   *
   * FreeRTOS necesita que PendSV tenga una prioridad muy baja para que
   * las interrupciones de periféricos puedan ejecutarse antes que el
   * cambio de contexto.
   *
   * Parámetros:
   *
   *   PendSV_IRQn : excepción que se configura.
   *   15          : prioridad de preempción.
   *   0           : subprioridad.
   */
  HAL_NVIC_SetPriority(PendSV_IRQn,
                       15,
                       0);


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
