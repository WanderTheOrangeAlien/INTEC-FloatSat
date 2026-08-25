/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    i2c.c
  * @brief   Configuración del periférico I2C1.
  *
  * Este archivo es generado principalmente por STM32CubeMX.
  *
  * En este proyecto, I2C1 se utiliza para comunicarse con la IMU LSM9DS1
  * mediante:
  *
  *   PB8  -> I2C1_SCL, línea de reloj.
  *   PB9  -> I2C1_SDA, línea de datos.
  *
  * La comunicación está configurada a 100 kHz y utiliza direcciones
  * de esclavo de 7 bits.
  *
  * Las secciones USER CODE BEGIN / USER CODE END se conservan cuando
  * STM32CubeMX vuelve a generar el código.
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
#include "i2c.h"

/* USER CODE BEGIN 0 */

/*
 * Espacio reservado para:
 *
 *   - Variables privadas relacionadas con I2C.
 *   - Funciones auxiliares.
 *   - Callbacks personalizados.
 *   - Macros adicionales.
 *
 * El contenido colocado aquí será conservado por CubeMX.
 */

/* USER CODE END 0 */

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /*
   * Espacio para código que deba ejecutarse antes de comenzar
   * a llenar la estructura hi2c1.
   */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /*
   * Segundo espacio reservado antes de HAL_I2C_Init().
   */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /*
   * Espacio para código posterior a una inicialización correcta.
   *
   * Aquí podrían agregarse, por ejemplo:
   *
   *   - Comprobaciones adicionales.
   *   - Activación de filtros I2C.
   *   - Variables de diagnóstico.
   */

  /* USER CODE END I2C1_Init 2 */

}

void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspInit 0 */

    /*
     * Espacio para código que deba ejecutarse antes de configurar
     * físicamente I2C1.
     */

  /* USER CODE END I2C1_MspInit 0 */

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C1 clock enable */
    __HAL_RCC_I2C1_CLK_ENABLE();
  /* USER CODE BEGIN I2C1_MspInit 1 */

    /*
     * Espacio para código posterior a la configuración de bajo nivel.
     *
     * Aquí podrían configurarse:
     *
     *   - Interrupciones NVIC.
     *   - DMA.
     *   - Variables de diagnóstico.
     */

  /* USER CODE END I2C1_MspInit 1 */
  }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{

  if(i2cHandle->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */

    /*
     * Espacio reservado antes de desactivar el periférico.
     */

  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();

    /**I2C1 GPIO Configuration
    PB8     ------> I2C1_SCL
    PB9     ------> I2C1_SDA
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_9);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

    /*
     * Espacio reservado para código posterior a la desinicialización.
     */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/*
 * Espacio reservado para funciones adicionales relacionadas con I2C.
 *
 * Ejemplos posibles:
 *
 *   - Función para escanear direcciones.
 *   - Función para registrar errores.
 *   - Callback de finalización.
 *   - Diagnóstico del estado del bus.
 */

/* USER CODE END 1 */

