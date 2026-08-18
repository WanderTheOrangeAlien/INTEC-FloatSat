/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_hal_timebase_tim.c
  * @brief   Base de tiempo de la biblioteca HAL implementada con TIM6.
  *
  * La biblioteca HAL necesita un contador de tiempo global, normalmente
  * incrementado cada 1 ms. En este proyecto no se utiliza SysTick para esa
  * función; se utiliza el temporizador básico TIM6.
  *
  * TIM6 genera una interrupción cada milisegundo. La interrupción termina
  * llamando a HAL_IncTick(), desde HAL_TIM_PeriodElapsedCallback(), para
  * incrementar la variable global de tiempo de HAL.
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
 * Cabecera principal de la biblioteca HAL.
 *
 * Proporciona:
 *
 *   - Tipos RCC y NVIC.
 *   - HAL_StatusTypeDef.
 *   - Funciones de reloj.
 *   - Variables globales del tick HAL, como uwTickPrio.
 */
#include "stm32f4xx_hal.h"

/*
 * Cabecera específica de temporizadores.
 *
 * Proporciona:
 *
 *   - TIM_HandleTypeDef.
 *   - HAL_TIM_Base_Init().
 *   - HAL_TIM_Base_Start_IT().
 *   - Macros para habilitar y deshabilitar interrupciones de TIM.
 */
#include "stm32f4xx_hal_tim.h"


/* Private typedef -----------------------------------------------------------*/

/*
 * No se definen tipos privados adicionales.
 */


/* Private define ------------------------------------------------------------*/

/*
 * No se definen constantes privadas adicionales.
 */


/* Private macro -------------------------------------------------------------*/

/*
 * No se definen macros privadas adicionales.
 */


/* Private variables ---------------------------------------------------------*/

/*
 * Manejador de TIM6.
 *
 * Contiene:
 *
 *   - La instancia física TIM6.
 *   - Prescaler.
 *   - Periodo.
 *   - Modo de conteo.
 *   - Estado interno utilizado por HAL.
 *
 * Aunque aparece bajo "Private variables", no está declarado static,
 * por lo que posee enlace externo y puede ser referenciado desde otros
 * archivos mediante una declaración extern.
 */
TIM_HandleTypeDef htim6;


/* Private function prototypes -----------------------------------------------*/

/*
 * No existen prototipos privados adicionales.
 */


/* Private functions ---------------------------------------------------------*/


/**
  * @brief Configura TIM6 como fuente de tiempo de la biblioteca HAL.
  *
  * El temporizador queda configurado para generar una interrupción
  * periódica cada 1 ms.
  *
  * Esta función puede ser llamada automáticamente:
  *
  *   - Durante HAL_Init(), al comienzo del programa.
  *   - Desde HAL_RCC_ClockConfig(), cuando cambia el reloj del sistema.
  *
  * La segunda posibilidad permite recalcular el prescaler si cambia
  * la frecuencia que alimenta a TIM6.
  *
  * @param TickPriority Prioridad NVIC asignada a la interrupción de TIM6.
  *
  * @retval HAL_OK si la inicialización fue correcta.
  * @retval HAL_ERROR si ocurrió algún error o la prioridad no es válida.
  */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  /*
   * Estructura donde HAL_RCC_GetClockConfig() copiará la configuración
   * actual de los buses AHB y APB.
   */
  RCC_ClkInitTypeDef clkconfig;

  /*
   * uwTimclock:
   * frecuencia efectiva que alimenta al contador de TIM6.
   *
   * uwAPB1Prescaler:
   * divisor configurado para el bus APB1.
   */
  uint32_t uwTimclock;
  uint32_t uwAPB1Prescaler = 0U;

  /*
   * Valor que se escribirá en el prescaler de TIM6.
   */
  uint32_t uwPrescalerValue = 0U;

  /*
   * Latencia actual de la memoria Flash.
   *
   * HAL_RCC_GetClockConfig() necesita un puntero donde almacenarla,
   * aunque esta función no la utiliza posteriormente.
   */
  uint32_t pFLatency;

  /*
   * Resultado de las funciones HAL utilizadas durante la inicialización.
   */
  HAL_StatusTypeDef status;


  /* ------------------------------------------------------------------------ */
  /* Habilitación del reloj de TIM6                                           */
  /* ------------------------------------------------------------------------ */

  /*
   * Habilitar el reloj del periférico TIM6.
   *
   * TIM6 pertenece al bus APB1.
   */
  __HAL_RCC_TIM6_CLK_ENABLE();


  /* ------------------------------------------------------------------------ */
  /* Obtención de la configuración actual de relojes                          */
  /* ------------------------------------------------------------------------ */

  /*
   * Leer la configuración de SYSCLK, HCLK, PCLK1 y PCLK2.
   *
   * También se obtiene la latencia actual de Flash.
   */
  HAL_RCC_GetClockConfig(&clkconfig,
                         &pFLatency);


  /*
   * Guardar el divisor utilizado por APB1.
   */
  uwAPB1Prescaler =
      clkconfig.APB1CLKDivider;


  /* ------------------------------------------------------------------------ */
  /* Cálculo de la frecuencia real de TIM6                                    */
  /* ------------------------------------------------------------------------ */

  /*
   * En los STM32F4, los temporizadores de un bus APB reciben:
   *
   *   PCLKx          cuando el divisor APB es 1.
   *   2 × PCLKx      cuando el divisor APB es mayor que 1.
   *
   * TIM6 pertenece a APB1.
   */
  if (uwAPB1Prescaler == RCC_HCLK_DIV1)
  {
    /*
     * Sin división en APB1:
     *
     *   TIM6CLK = PCLK1
     */
    uwTimclock =
        HAL_RCC_GetPCLK1Freq();
  }
  else
  {
    /*
     * Con división en APB1:
     *
     *   TIM6CLK = 2 × PCLK1
     *
     * En la configuración actual:
     *
     *   PCLK1   = 42 MHz
     *   TIM6CLK = 84 MHz
     */
    uwTimclock =
        2UL * HAL_RCC_GetPCLK1Freq();
  }


  /* ------------------------------------------------------------------------ */
  /* Cálculo del prescaler                                                    */
  /* ------------------------------------------------------------------------ */

  /*
   * Se desea que el contador interno de TIM6 avance a 1 MHz:
   *
   *   frecuencia_contador =
   *       TIM6CLK / (Prescaler + 1)
   *
   * Despejando:
   *
   *   Prescaler =
   *       TIM6CLK / 1 000 000 - 1
   *
   * Para TIM6CLK = 84 MHz:
   *
   *   Prescaler = 84 - 1 = 83
   *
   * Cada cuenta representa entonces:
   *
   *   1 / 1 MHz = 1 microsegundo
   */
  uwPrescalerValue =
      (uint32_t)((uwTimclock / 1000000U) - 1U);


  /* ------------------------------------------------------------------------ */
  /* Configuración de TIM6                                                    */
  /* ------------------------------------------------------------------------ */

  /*
   * Asociar el manejador con la instancia física TIM6.
   */
  htim6.Instance = TIM6;


  /*
   * El temporizador contará desde 0 hasta 999:
   *
   *   Period = 1000 - 1 = 999
   *
   * Con una frecuencia de contador de 1 MHz:
   *
   *   1000 cuentas × 1 microsegundo = 1 milisegundo
   *
   * Por tanto, TIM6 genera 1000 eventos por segundo.
   */
  htim6.Init.Period =
      (1000000U / 1000U) - 1U;


  /*
   * Aplicar el prescaler calculado para obtener un contador de 1 MHz.
   */
  htim6.Init.Prescaler =
      uwPrescalerValue;


  /*
   * TIM6 es un temporizador básico y no necesita división adicional
   * para esta aplicación.
   */
  htim6.Init.ClockDivision = 0;


  /*
   * El contador avanza desde cero hasta el valor Period.
   */
  htim6.Init.CounterMode =
      TIM_COUNTERMODE_UP;


  /*
   * Deshabilitar el preload del registro de autorrecarga.
   *
   * Los cambios en ARR se aplican directamente, sin esperar un evento
   * de actualización posterior.
   */
  htim6.Init.AutoReloadPreload =
      TIM_AUTORELOAD_PRELOAD_DISABLE;


  /* ------------------------------------------------------------------------ */
  /* Inicialización e inicio del temporizador                                 */
  /* ------------------------------------------------------------------------ */

  /*
   * Inicializar TIM6 como temporizador básico.
   *
   * El resultado se guarda para propagar cualquier error.
   */
  status =
      HAL_TIM_Base_Init(&htim6);


  if (status == HAL_OK)
  {
    /*
     * Iniciar TIM6 y habilitar la generación de interrupciones
     * por evento de actualización.
     */
    status =
        HAL_TIM_Base_Start_IT(&htim6);


    if (status == HAL_OK)
    {
      /*
       * Habilitar en el NVIC la línea compartida por TIM6 y DAC.
       *
       * El nombre TIM6_DAC_IRQn indica que TIM6 y ciertos eventos
       * del DAC comparten el mismo vector de interrupción.
       */
      HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);


      /*
       * Comprobar que la prioridad solicitada cabe dentro del número
       * de bits de prioridad implementados por el Cortex-M.
       *
       * En el STM32F407 normalmente:
       *
       *   __NVIC_PRIO_BITS = 4
       *
       * Por tanto, las prioridades válidas son de 0 a 15.
       */
      if (TickPriority < (1UL << __NVIC_PRIO_BITS))
      {
        /*
         * Configurar la prioridad de la interrupción de TIM6.
         *
         * Un número menor representa una prioridad lógica mayor.
         */
        HAL_NVIC_SetPriority(TIM6_DAC_IRQn,
                             TickPriority,
                             0U);

        /*
         * Guardar la prioridad utilizada en la variable global de HAL.
         */
        uwTickPrio =
            TickPriority;
      }
      else
      {
        /*
         * La prioridad solicitada está fuera del rango disponible.
         */
        status = HAL_ERROR;
      }
    }
  }


  /*
   * Devolver el resultado final de la configuración.
   */
  return status;
}


/**
  * @brief Suspende el incremento del tick de HAL.
  *
  * No detiene físicamente el contador TIM6. Solamente deshabilita
  * su interrupción de actualización.
  *
  * Mientras el tick esté suspendido:
  *
  *   - HAL_GetTick() deja de avanzar.
  *   - Los timeouts basados en HAL pueden dejar de progresar.
  *   - HAL_Delay() puede bloquearse si depende de este tick.
  *
  * @retval None
  */
void HAL_SuspendTick(void)
{
  /*
   * Deshabilitar el bit de interrupción por actualización de TIM6.
   *
   * El contador puede continuar contando internamente.
   */
  __HAL_TIM_DISABLE_IT(&htim6,
                       TIM_IT_UPDATE);
}


/**
  * @brief Reanuda el incremento del tick de HAL.
  *
  * Vuelve a habilitar la interrupción periódica de actualización
  * de TIM6.
  *
  * @retval None
  */
void HAL_ResumeTick(void)
{
  /*
   * Habilitar nuevamente la interrupción por actualización de TIM6.
   */
  __HAL_TIM_ENABLE_IT(&htim6,
                      TIM_IT_UPDATE);
}
