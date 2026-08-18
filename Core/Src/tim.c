/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.c
  * @brief   Configuración de los temporizadores TIM1 y TIM3.
  *
  * En este proyecto:
  *
  *   TIM1:
  *     - Genera dos señales PWM.
  *     - Canal 1: PE9.
  *     - Canal 2: PE11.
  *     - Se utiliza para controlar los dos sentidos del motor.
  *
  *   TIM3:
  *     - Funciona en modo encoder incremental en cuadratura.
  *     - Canal 1: PB4.
  *     - Canal 2: PB5.
  *     - Permite conocer posición, sentido y velocidad de la rueda.
  *
  * Las regiones USER CODE BEGIN / USER CODE END se conservan cuando
  * STM32CubeMX vuelve a generar el proyecto.
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
 * tim.h declara:
 *
 *   - htim1.
 *   - htim3.
 *   - MX_TIM1_Init().
 *   - MX_TIM3_Init().
 *
 * También proporciona los tipos y funciones HAL asociados a temporizadores.
 */
#include "tim.h"


/* USER CODE BEGIN 0 */

/*
 * Espacio reservado para:
 *
 *   - Variables privadas.
 *   - Funciones auxiliares.
 *   - Macros relacionadas con los temporizadores.
 *
 * CubeMX conservará el código colocado dentro de esta región.
 */

/* USER CODE END 0 */


/* -------------------------------------------------------------------------- */
/* Manejadores globales                                                       */
/* -------------------------------------------------------------------------- */

/*
 * Manejador de TIM1.
 *
 * Contiene la configuración y estado del temporizador utilizado para PWM.
 */
TIM_HandleTypeDef htim1;

/*
 * Manejador de TIM3.
 *
 * Contiene la configuración y estado del temporizador utilizado
 * para leer el encoder.
 */
TIM_HandleTypeDef htim3;


/* -------------------------------------------------------------------------- */
/* Inicialización de TIM1                                                     */
/* -------------------------------------------------------------------------- */

/**
  * @brief Inicializa TIM1 para generar dos señales PWM.
  *
  * Configuración principal:
  *
  *   - Fuente de reloj interna.
  *   - Conteo ascendente.
  *   - Prescaler = 0.
  *   - Periodo = 8399.
  *   - PWM modo 1.
  *   - Canal 1 en PE9.
  *   - Canal 2 en PE11.
  *
  * Con un reloj de TIM1 de 168 MHz:
  *
  *   frecuencia PWM =
  *       168 000 000 / ((0 + 1) × (8399 + 1))
  *
  *   frecuencia PWM = 20 000 Hz
  *
  * Por tanto, ambos canales generan PWM a aproximadamente 20 kHz.
  */
void MX_TIM1_Init(void)
{
  /* USER CODE BEGIN TIM1_Init 0 */

  /*
   * Espacio reservado para código anterior a la creación
   * de las estructuras de configuración.
   */

  /* USER CODE END TIM1_Init 0 */


  /*
   * Configuración de la fuente de reloj de TIM1.
   */
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};

  /*
   * Configuración de sincronización maestro-esclavo.
   */
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /*
   * Configuración de los canales de Output Compare/PWM.
   */
  TIM_OC_InitTypeDef sConfigOC = {0};

  /*
   * Configuración de funciones avanzadas de TIM1:
   *
   *   - Break.
   *   - Dead time.
   *   - Estados de salida.
   *   - Bloqueo de configuración.
   */
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};


  /* USER CODE BEGIN TIM1_Init 1 */

  /*
   * Espacio reservado para código anterior a la configuración
   * del manejador htim1.
   */

  /* USER CODE END TIM1_Init 1 */


  /*
   * Asociar el manejador con la instancia física TIM1.
   */
  htim1.Instance = TIM1;


  /*
   * Prescaler del contador.
   *
   * El reloj del temporizador se divide entre:
   *
   *   Prescaler + 1
   *
   * Con Prescaler = 0 no se aplica división:
   *
   *   reloj del contador = reloj TIM1
   */
  htim1.Init.Prescaler = 0;


  /*
   * Contador ascendente:
   *
   *   0, 1, 2, ..., 8399, 0, 1, ...
   */
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;


  /*
   * Valor de autorrecarga ARR.
   *
   * El temporizador cuenta 8400 valores:
   *
   *   desde 0 hasta 8399.
   */
  htim1.Init.Period = 8399;


  /*
   * Sin división adicional del reloj interno del temporizador.
   *
   * Esta opción se relaciona principalmente con filtros digitales
   * y generación de tiempo muerto, no con el prescaler principal.
   */
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;


  /*
   * TIM1 es un temporizador avanzado y posee contador de repetición.
   *
   * Con valor 0 se genera un evento de actualización en cada periodo.
   */
  htim1.Init.RepetitionCounter = 0;


  /*
   * Habilitar preload del registro ARR.
   *
   * Los cambios de periodo se cargan en un registro temporal y se aplican
   * en el siguiente evento de actualización, evitando cambios abruptos
   * a mitad de un ciclo PWM.
   */
  htim1.Init.AutoReloadPreload =
      TIM_AUTORELOAD_PRELOAD_ENABLE;


  /*
   * Inicializar TIM1 como temporizador base.
   *
   * Durante esta llamada HAL utiliza HAL_TIM_Base_MspInit(), que habilita
   * el reloj del periférico TIM1.
   */
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }


  /*
   * Seleccionar el reloj interno del microcontrolador como fuente
   * del contador de TIM1.
   */
  sClockSourceConfig.ClockSource =
      TIM_CLOCKSOURCE_INTERNAL;


  /*
   * Aplicar la fuente de reloj.
   */
  if (HAL_TIM_ConfigClockSource(
          &htim1,
          &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }


  /*
   * Inicializar las funciones PWM de TIM1.
   */
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }


  /* ------------------------------------------------------------------------ */
  /* Configuración maestro-esclavo                                            */
  /* ------------------------------------------------------------------------ */

  /*
   * TIM_TRGO_RESET:
   *
   * TIM1 no genera una señal de disparo útil para otros periféricos.
   */
  sMasterConfig.MasterOutputTrigger =
      TIM_TRGO_RESET;


  /*
   * TIM1 no se sincroniza como maestro con otros temporizadores.
   */
  sMasterConfig.MasterSlaveMode =
      TIM_MASTERSLAVEMODE_DISABLE;


  /*
   * Aplicar la configuración de sincronización.
   */
  if (HAL_TIMEx_MasterConfigSynchronization(
          &htim1,
          &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }


  /* ------------------------------------------------------------------------ */
  /* Configuración PWM común para canales 1 y 2                               */
  /* ------------------------------------------------------------------------ */

  /*
   * PWM modo 1:
   *
   * Durante conteo ascendente, la salida permanece activa mientras:
   *
   *   CNT < CCRx
   *
   * y pasa a inactiva cuando:
   *
   *   CNT >= CCRx
   */
  sConfigOC.OCMode =
      TIM_OCMODE_PWM1;


  /*
   * Duty inicial igual a cero.
   *
   * CCR1 y CCR2 comienzan en 0, por lo que las dos salidas PWM
   * permanecen inactivas.
   */
  sConfigOC.Pulse = 0;


  /*
   * Polaridad activa alta para la salida principal.
   *
   * El intervalo activo del PWM se representa mediante nivel alto.
   */
  sConfigOC.OCPolarity =
      TIM_OCPOLARITY_HIGH;


  /*
   * Polaridad activa alta para la salida complementaria.
   *
   * En este proyecto no se utilizan las salidas complementarias CH1N/CH2N,
   * pero CubeMX mantiene este parámetro en la estructura.
   */
  sConfigOC.OCNPolarity =
      TIM_OCNPOLARITY_HIGH;


  /*
   * Deshabilitar el modo rápido de Output Compare.
   */
  sConfigOC.OCFastMode =
      TIM_OCFAST_DISABLE;


  /*
   * Cuando la salida principal queda inactiva, utilizar nivel bajo.
   */
  sConfigOC.OCIdleState =
      TIM_OCIDLESTATE_RESET;


  /*
   * Estado inactivo bajo para la salida complementaria.
   */
  sConfigOC.OCNIdleState =
      TIM_OCNIDLESTATE_RESET;


  /*
   * Aplicar la configuración PWM al canal 1.
   *
   * Este canal sale físicamente por PE9.
   */
  if (HAL_TIM_PWM_ConfigChannel(
          &htim1,
          &sConfigOC,
          TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }


  /*
   * Aplicar la misma configuración PWM al canal 2.
   *
   * Este canal sale físicamente por PE11.
   */
  if (HAL_TIM_PWM_ConfigChannel(
          &htim1,
          &sConfigOC,
          TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }


  /* ------------------------------------------------------------------------ */
  /* Configuración avanzada de TIM1                                           */
  /* ------------------------------------------------------------------------ */

  /*
   * No mantener activas las salidas cuando el temporizador deja
   * de funcionar en modo Run.
   */
  sBreakDeadTimeConfig.OffStateRunMode =
      TIM_OSSR_DISABLE;


  /*
   * No mantener activas las salidas durante estados Idle.
   */
  sBreakDeadTimeConfig.OffStateIDLEMode =
      TIM_OSSI_DISABLE;


  /*
   * No bloquear los registros de configuración.
   *
   * Permanecen modificables mediante software.
   */
  sBreakDeadTimeConfig.LockLevel =
      TIM_LOCKLEVEL_OFF;


  /*
   * No aplicar tiempo muerto entre salidas.
   *
   * El dead time suele utilizarse en puentes de potencia con señales
   * complementarias para evitar conducción simultánea.
   */
  sBreakDeadTimeConfig.DeadTime = 0;


  /*
   * Deshabilitar la entrada Break.
   *
   * No existe una entrada hardware configurada para apagar
   * automáticamente las salidas ante una condición de fallo.
   */
  sBreakDeadTimeConfig.BreakState =
      TIM_BREAK_DISABLE;


  /*
   * Si Break estuviera habilitado, sería activo en nivel alto.
   */
  sBreakDeadTimeConfig.BreakPolarity =
      TIM_BREAKPOLARITY_HIGH;


  /*
   * Las salidas no se habilitan automáticamente después
   * de una condición Break.
   */
  sBreakDeadTimeConfig.AutomaticOutput =
      TIM_AUTOMATICOUTPUT_DISABLE;


  /*
   * Aplicar la configuración avanzada.
   */
  if (HAL_TIMEx_ConfigBreakDeadTime(
          &htim1,
          &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }


  /* USER CODE BEGIN TIM1_Init 2 */

  /*
   * Espacio reservado para código posterior a la configuración lógica
   * de TIM1.
   *
   * En este punto todavía no es obligatorio que los canales PWM
   * estén iniciados.
   */

  /* USER CODE END TIM1_Init 2 */


  /*
   * Configurar físicamente los GPIO de salida de TIM1:
   *
   *   PE9  -> TIM1_CH1.
   *   PE11 -> TIM1_CH2.
   */
  HAL_TIM_MspPostInit(&htim1);
}


/* -------------------------------------------------------------------------- */
/* Inicialización de TIM3                                                     */
/* -------------------------------------------------------------------------- */

/**
  * @brief Inicializa TIM3 en modo encoder incremental.
  *
  * Configuración:
  *
  *   - PB4 como TIM3_CH1.
  *   - PB5 como TIM3_CH2.
  *   - Modo encoder TI12.
  *   - Contador de 16 bits: 0 a 65535.
  *   - Conteo en ambos canales.
  *
  * El hardware del temporizador determina automáticamente:
  *
  *   - Sentido de giro.
  *   - Incremento o decremento del contador.
  *   - Posición relativa del encoder.
  */
void MX_TIM3_Init(void)
{
  /* USER CODE BEGIN TIM3_Init 0 */

  /*
   * Espacio reservado para código anterior a la configuración.
   */

  /* USER CODE END TIM3_Init 0 */


  /*
   * Estructura específica para configurar el modo encoder.
   */
  TIM_Encoder_InitTypeDef sConfig = {0};

  /*
   * Configuración maestro-esclavo de TIM3.
   */
  TIM_MasterConfigTypeDef sMasterConfig = {0};


  /* USER CODE BEGIN TIM3_Init 1 */

  /*
   * Espacio reservado antes de llenar htim3.
   */

  /* USER CODE END TIM3_Init 1 */


  /*
   * Asociar el manejador con TIM3.
   */
  htim3.Instance = TIM3;


  /*
   * Sin división del reloj de entrada.
   *
   * En modo encoder, las señales externas de CH1 y CH2 determinan
   * los eventos de conteo.
   */
  htim3.Init.Prescaler = 0;


  /*
   * CubeMX conserva el modo ascendente como configuración base.
   *
   * En modo encoder, el hardware cambia dinámicamente el sentido
   * del contador según la secuencia de los canales A y B.
   */
  htim3.Init.CounterMode =
      TIM_COUNTERMODE_UP;


  /*
   * Periodo máximo de un contador de 16 bits:
   *
   *   0 a 65535.
   *
   * Después de 65535 el contador vuelve a 0.
   * Al decrementar desde 0 vuelve a 65535.
   */
  htim3.Init.Period = 65535;


  /*
   * Sin división adicional del reloj interno.
   */
  htim3.Init.ClockDivision =
      TIM_CLOCKDIVISION_DIV1;


  /*
   * Los cambios del registro ARR se aplican directamente.
   */
  htim3.Init.AutoReloadPreload =
      TIM_AUTORELOAD_PRELOAD_DISABLE;


  /* ------------------------------------------------------------------------ */
  /* Configuración del modo encoder                                           */
  /* ------------------------------------------------------------------------ */

  /*
   * TIM_ENCODERMODE_TI12 utiliza los dos canales:
   *
   *   TI1 y TI2.
   *
   * El temporizador cuenta transiciones de ambas señales y determina
   * el sentido mediante el desfase entre ellas.
   */
  sConfig.EncoderMode =
      TIM_ENCODERMODE_TI12;


  /*
   * Canal 1 activo en flanco ascendente.
   */
  sConfig.IC1Polarity =
      TIM_ICPOLARITY_RISING;


  /*
   * Conectar directamente la entrada física TI1 al canal de captura 1.
   */
  sConfig.IC1Selection =
      TIM_ICSELECTION_DIRECTTI;


  /*
   * No dividir eventos del canal 1.
   *
   * Cada evento válido llega al detector del encoder.
   */
  sConfig.IC1Prescaler =
      TIM_ICPSC_DIV1;


  /*
   * Filtro digital del canal 1 con valor 4.
   *
   * Ayuda a rechazar pulsos breves y ruido eléctrico antes de que sean
   * interpretados como transiciones válidas del encoder.
   */
  sConfig.IC1Filter = 4;


  /*
   * Canal 2 activo en flanco ascendente.
   */
  sConfig.IC2Polarity =
      TIM_ICPOLARITY_RISING;


  /*
   * Conectar directamente TI2 al canal de captura 2.
   */
  sConfig.IC2Selection =
      TIM_ICSELECTION_DIRECTTI;


  /*
   * No dividir eventos del canal 2.
   */
  sConfig.IC2Prescaler =
      TIM_ICPSC_DIV1;


  /*
   * Canal 2 sin filtro digital.
   *
   * Cualquier transición válida llega directamente a la lógica del encoder.
   */
  sConfig.IC2Filter = 0;


  /*
   * Inicializar TIM3 en modo encoder.
   *
   * Esta llamada también ejecuta HAL_TIM_Encoder_MspInit(), que:
   *
   *   - Habilita el reloj de TIM3.
   *   - Configura PB4.
   *   - Configura PB5.
   */
  if (HAL_TIM_Encoder_Init(
          &htim3,
          &sConfig) != HAL_OK)
  {
    Error_Handler();
  }


  /* ------------------------------------------------------------------------ */
  /* Configuración maestro-esclavo                                            */
  /* ------------------------------------------------------------------------ */

  /*
   * TIM3 no genera una señal TRGO para sincronizar otros periféricos.
   */
  sMasterConfig.MasterOutputTrigger =
      TIM_TRGO_RESET;


  /*
   * Deshabilitar el modo maestro-esclavo.
   */
  sMasterConfig.MasterSlaveMode =
      TIM_MASTERSLAVEMODE_DISABLE;


  /*
   * Aplicar la configuración.
   */
  if (HAL_TIMEx_MasterConfigSynchronization(
          &htim3,
          &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }


  /* USER CODE BEGIN TIM3_Init 2 */

  /*
   * Espacio reservado para código posterior a la configuración.
   *
   * La lectura del encoder todavía debe iniciarse mediante:
   *
   *   HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
   */

  /* USER CODE END TIM3_Init 2 */
}


/* -------------------------------------------------------------------------- */
/* Inicialización MSP de TIM1                                                 */
/* -------------------------------------------------------------------------- */

/**
  * @brief Inicializa los recursos básicos de un temporizador base.
  *
  * Para TIM1 solamente habilita su reloj interno.
  *
  * @param tim_baseHandle Manejador del temporizador que se inicializa.
  */
void HAL_TIM_Base_MspInit(
    TIM_HandleTypeDef *tim_baseHandle)
{
  /*
   * Ejecutar esta sección únicamente para TIM1.
   */
  if (tim_baseHandle->Instance == TIM1)
  {
    /* USER CODE BEGIN TIM1_MspInit 0 */

    /*
     * Espacio reservado antes de habilitar TIM1.
     */

    /* USER CODE END TIM1_MspInit 0 */


    /*
     * Habilitar el reloj de TIM1.
     *
     * TIM1 pertenece al bus APB2.
     */
    __HAL_RCC_TIM1_CLK_ENABLE();


    /* USER CODE BEGIN TIM1_MspInit 1 */

    /*
     * Espacio reservado después de habilitar el reloj.
     */

    /* USER CODE END TIM1_MspInit 1 */
  }
}


/* -------------------------------------------------------------------------- */
/* Inicialización MSP de TIM3 como encoder                                    */
/* -------------------------------------------------------------------------- */

/**
  * @brief Configura el reloj y GPIO utilizados por TIM3 en modo encoder.
  *
  * @param tim_encoderHandle Manejador del temporizador.
  */
void HAL_TIM_Encoder_MspInit(
    TIM_HandleTypeDef *tim_encoderHandle)
{
  /*
   * Estructura temporal para configurar PB4 y PB5.
   */
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  /*
   * Ejecutar únicamente cuando se inicializa TIM3.
   */
  if (tim_encoderHandle->Instance == TIM3)
  {
    /* USER CODE BEGIN TIM3_MspInit 0 */

    /*
     * Espacio reservado antes de configurar TIM3.
     */

    /* USER CODE END TIM3_MspInit 0 */


    /*
     * Habilitar el reloj interno del periférico TIM3.
     *
     * TIM3 pertenece al bus APB1.
     */
    __HAL_RCC_TIM3_CLK_ENABLE();


    /*
     * Habilitar el reloj del puerto GPIOB.
     */
    __HAL_RCC_GPIOB_CLK_ENABLE();


    /*
     * Conexiones físicas:
     *
     *   PB4 -> TIM3_CH1 -> Encoder A.
     *   PB5 -> TIM3_CH2 -> Encoder B.
     */


    /*
     * Seleccionar PB4 y PB5.
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_4 |
        GPIO_PIN_5;


    /*
     * Función alternativa push-pull.
     *
     * Los pines quedan conectados al periférico TIM3 en lugar de ser
     * leídos directamente mediante HAL_GPIO_ReadPin().
     */
    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_PP;


    /*
     * No habilitar resistencias internas.
     *
     * Las señales deben estar definidas eléctricamente por el encoder,
     * el controlador o resistencias externas.
     */
    GPIO_InitStruct.Pull =
        GPIO_NOPULL;


    /*
     * Velocidad eléctrica baja.
     *
     * Este campo controla las características de conmutación del GPIO,
     * no la frecuencia máxima del contador interno.
     */
    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;


    /*
     * AF2 conecta PB4 y PB5 con TIM3.
     */
    GPIO_InitStruct.Alternate =
        GPIO_AF2_TIM3;


    /*
     * Aplicar la configuración al puerto B.
     */
    HAL_GPIO_Init(GPIOB,
                  &GPIO_InitStruct);


    /* USER CODE BEGIN TIM3_MspInit 1 */

    /*
     * Espacio reservado después de configurar TIM3 y sus GPIO.
     */

    /* USER CODE END TIM3_MspInit 1 */
  }
}


/* -------------------------------------------------------------------------- */
/* Configuración GPIO posterior de TIM1                                       */
/* -------------------------------------------------------------------------- */

/**
  * @brief Configura los pines físicos de salida PWM de TIM1.
  *
  * Se llama desde MX_TIM1_Init() después de configurar los canales.
  *
  * @param timHandle Manejador del temporizador.
  */
void HAL_TIM_MspPostInit(
    TIM_HandleTypeDef *timHandle)
{
  /*
   * Estructura temporal para los GPIO.
   */
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  /*
   * Ejecutar únicamente para TIM1.
   */
  if (timHandle->Instance == TIM1)
  {
    /* USER CODE BEGIN TIM1_MspPostInit 0 */

    /*
     * Espacio reservado antes de configurar los pines PWM.
     */

    /* USER CODE END TIM1_MspPostInit 0 */


    /*
     * Habilitar el reloj de GPIOE.
     */
    __HAL_RCC_GPIOE_CLK_ENABLE();


    /*
     * Conexiones:
     *
     *   PE9  -> TIM1_CH1.
     *   PE11 -> TIM1_CH2.
     */


    /*
     * Seleccionar PE9 y PE11.
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_9 |
        GPIO_PIN_11;


    /*
     * Conectar los pines a la función alternativa del temporizador.
     */
    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_PP;


    /*
     * Sin resistencias internas.
     */
    GPIO_InitStruct.Pull =
        GPIO_NOPULL;


    /*
     * Velocidad eléctrica baja.
     *
     * Aunque la señal PWM sea de 20 kHz, esta velocidad suele ser
     * suficiente para una señal de control digital de este tipo.
     */
    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_LOW;


    /*
     * AF1 conecta PE9 y PE11 con TIM1.
     */
    GPIO_InitStruct.Alternate =
        GPIO_AF1_TIM1;


    /*
     * Aplicar la configuración.
     */
    HAL_GPIO_Init(GPIOE,
                  &GPIO_InitStruct);


    /* USER CODE BEGIN TIM1_MspPostInit 1 */

    /*
     * Espacio reservado después de configurar las salidas PWM.
     */

    /* USER CODE END TIM1_MspPostInit 1 */
  }
}


/* -------------------------------------------------------------------------- */
/* Desinicialización de TIM1                                                  */
/* -------------------------------------------------------------------------- */

/**
  * @brief Libera los recursos básicos utilizados por TIM1.
  *
  * @param tim_baseHandle Manejador del temporizador.
  */
void HAL_TIM_Base_MspDeInit(
    TIM_HandleTypeDef *tim_baseHandle)
{
  if (tim_baseHandle->Instance == TIM1)
  {
    /* USER CODE BEGIN TIM1_MspDeInit 0 */

    /*
     * Espacio reservado antes de deshabilitar TIM1.
     */

    /* USER CODE END TIM1_MspDeInit 0 */


    /*
     * Deshabilitar el reloj de TIM1.
     */
    __HAL_RCC_TIM1_CLK_DISABLE();


    /* USER CODE BEGIN TIM1_MspDeInit 1 */

    /*
     * Espacio reservado después de deshabilitar TIM1.
     */

    /* USER CODE END TIM1_MspDeInit 1 */
  }
}


/* -------------------------------------------------------------------------- */
/* Desinicialización de TIM3                                                  */
/* -------------------------------------------------------------------------- */

/**
  * @brief Libera TIM3 y sus pines de encoder.
  *
  * @param tim_encoderHandle Manejador del temporizador.
  */
void HAL_TIM_Encoder_MspDeInit(
    TIM_HandleTypeDef *tim_encoderHandle)
{
  if (tim_encoderHandle->Instance == TIM3)
  {
    /* USER CODE BEGIN TIM3_MspDeInit 0 */

    /*
     * Espacio reservado antes de deshabilitar TIM3.
     */

    /* USER CODE END TIM3_MspDeInit 0 */


    /*
     * Deshabilitar el reloj de TIM3.
     */
    __HAL_RCC_TIM3_CLK_DISABLE();


    /*
     * Liberar PB4 y PB5:
     *
     *   PB4 -> TIM3_CH1.
     *   PB5 -> TIM3_CH2.
     */
    HAL_GPIO_DeInit(
        GPIOB,
        GPIO_PIN_4 |
        GPIO_PIN_5);


    /* USER CODE BEGIN TIM3_MspDeInit 1 */

    /*
     * Espacio reservado después de liberar los recursos.
     */

    /* USER CODE END TIM3_MspDeInit 1 */
  }
}


/* USER CODE BEGIN 1 */

/*
 * Espacio reservado para funciones adicionales relacionadas
 * con los temporizadores.
 */

/* USER CODE END 1 */
