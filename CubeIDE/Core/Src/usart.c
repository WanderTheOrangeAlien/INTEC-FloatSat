/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   Configuración del periférico USART1 en modo UART.
  *
  * En este proyecto, USART1 se utiliza como interfaz serial bidireccional:
  *
  *   PB6 -> USART1_TX: datos enviados por el STM32.
  *   PB7 -> USART1_RX: datos recibidos por el STM32.
  *
  * La comunicación queda configurada a:
  *
  *   - 9600 bits por segundo.
  *   - 8 bits de datos.
  *   - Sin paridad.
  *   - 1 bit de parada.
  *   - Transmisión y recepción habilitadas.
  *   - Sin control de flujo por hardware.
  *
  * Esta configuración suele representarse como:
  *
  *   9600, 8N1
  *
  * Las regiones USER CODE BEGIN / USER CODE END son preservadas por
  * STM32CubeMX cuando se vuelve a generar el código.
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
#include "usart.h"

/* USER CODE BEGIN 0 */

/*
 * Espacio reservado para:
 *
 *   - Funciones auxiliares de transmisión o recepción.
 *   - Buffers privados.
 *   - Variables relacionadas con comunicación serial.
 *
 * CubeMX conservará el contenido escrito dentro de esta región.
 */

/* USER CODE END 0 */

UART_HandleTypeDef huart1;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /*
   * Espacio reservado para código que deba ejecutarse antes de configurar
   * el manejador huart1.
   */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /*
   * Espacio reservado para variables o configuraciones personalizadas
   * anteriores a HAL_UART_Init().
   */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /*
   * Espacio reservado para código posterior a la inicialización.
   *
   * En este punto USART1 ya puede utilizarse con funciones como:
   *
   *   HAL_UART_Transmit().
   *   HAL_UART_Receive().
   */

  /* USER CODE END USART1_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

    /*
     * Espacio reservado para código anterior a la habilitación
     * del periférico.
     */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN USART1_MspInit 1 */

    /*
     * Espacio reservado para configuraciones posteriores.
     *
     * Aquí podrían habilitarse, por ejemplo:
     *
     *   - Interrupciones USART1.
     *   - Prioridad NVIC.
     *   - DMA para transmisión o recepción.
     */

  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

    /*
     * Espacio reservado antes de liberar USART1.
     */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

  /* USER CODE BEGIN USART1_MspDeInit 1 */

    /*
     * Espacio reservado para liberar recursos adicionales,
     * como DMA o interrupciones.
     */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/*
 * Espacio reservado para funciones adicionales de comunicación.
 *
 * Por ejemplo:
 *
 *   - Envío de cadenas.
 *   - Recepción de comandos.
 *   - Redirección de printf().
 *   - Procesamiento de paquetes de telemetría.
 */

/* USER CODE END 1 */

