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

/*
 * main.h proporciona:
 *
 *   - Tipos y funciones generales de HAL.
 *   - Definiciones del microcontrolador.
 *   - Declaraciones comunes del proyecto.
 */
#include "main.h"

/*
 * stm32f4xx_it.h declara los prototipos de los manejadores
 * de interrupción definidos en este archivo.
 */
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

/*
 * Manejador de TIM6 definido en:
 *
 *   stm32f4xx_hal_timebase_tim.c
 *
 * Se declara extern porque este archivo necesita entregarlo a
 * HAL_TIM_IRQHandler(), pero no reserva nuevamente su memoria.
 */
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
  * @brief Maneja la interrupción no enmascarable NMI.
  *
  * NMI significa Non-Maskable Interrupt.
  *
  * Es una excepción de prioridad muy alta que no puede deshabilitarse
  * mediante el enmascaramiento normal de interrupciones.
  *
  * Puede estar asociada, por ejemplo, a fallos graves del reloj cuando
  * se utiliza el Clock Security System.
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
  * @brief Maneja una excepción HardFault.
  *
  * HardFault representa un fallo grave que no pudo manejarse mediante
  * una excepción más específica, o un fallo ocurrido mientras otra
  * excepción ya estaba siendo atendida.
  *
  * Causas frecuentes:
  *
  *   - Acceso a una dirección de memoria inválida.
  *   - Puntero nulo utilizado como dirección.
  *   - Corrupción de pila.
  *   - Retorno a una dirección incorrecta.
  *   - Error escalado desde MemManage, BusFault o UsageFault.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /*
   * Aquí puede agregarse código para guardar registros del procesador
   * o leer los registros de diagnóstico SCB.
   */

  /* USER CODE END HardFault_IRQn 0 */


  /*
   * El sistema queda detenido permanentemente.
   */
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
  * @brief Maneja fallos de protección o administración de memoria.
  *
  * MemManage puede generarse por:
  *
  *   - Violación de una región configurada por la MPU.
  *   - Ejecución desde una región no ejecutable.
  *   - Acceso prohibido a una zona de memoria.
  *
  * La MPU debe estar configurada y habilitada para detectar muchas
  * de estas condiciones.
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
  * @brief Maneja fallos de acceso al bus.
  *
  * BusFault puede producirse por:
  *
  *   - Acceso a una dirección inexistente.
  *   - Error durante lectura o escritura de memoria.
  *   - Acceso inválido a un periférico.
  *   - Fallo durante la búsqueda de una instrucción.
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
  * @brief Maneja instrucciones inválidas o estados de ejecución ilegales.
  *
  * UsageFault puede generarse por:
  *
  *   - Instrucción indefinida.
  *   - Estado inválido del procesador.
  *   - Retorno de excepción incorrecto.
  *   - División entre cero, si su detección está habilitada.
  *   - Acceso no alineado, si su detección está habilitada.
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
  * @brief Maneja la excepción de monitor de depuración.
  *
  * DebugMon puede ser utilizado por herramientas de depuración,
  * watchpoints y otras funciones del bloque de depuración ARM.
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
/*                                                                            */
/* Aquí se colocan los manejadores de los periféricos habilitados en NVIC.    */
/* Los nombres deben coincidir con los definidos en startup_stm32f4xx.s.      */
/******************************************************************************/


/**
  * @brief Maneja la interrupción global compartida de TIM6 y el DAC.
  *
  * En este proyecto TIM6 funciona como base temporal de HAL y genera
  * una interrupción cada 1 ms.
  *
  * La línea NVIC se llama TIM6_DAC_IRQn porque comparte el vector con
  * determinados eventos de error del periférico DAC.
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


  /*
   * Manejador general HAL de temporizadores.
   *
   * Esta función:
   *
   *   1. Comprueba las banderas de TIM6.
   *   2. Limpia la bandera de interrupción.
   *   3. Ejecuta el callback correspondiente.
   *
   * En este proyecto termina llamando a:
   *
   *   HAL_TIM_PeriodElapsedCallback(&htim6)
   *
   * y ese callback llama a:
   *
   *   HAL_IncTick()
   */
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
