/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   Configuración de los pines GPIO utilizados por la placa.
  *
  * Este archivo es generado principalmente por STM32CubeMX.
  *
  * Configura:
  *
  *   - Pines de salida digital.
  *   - Pines de entrada digital.
  *   - Pines asociados a interrupciones o eventos.
  *   - Pines con funciones alternativas, como SPI, I2S y USB.
  *
  * Las secciones USER CODE BEGIN / USER CODE END son conservadas
  * cuando CubeMX vuelve a generar el proyecto.
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
 * gpio.h contiene:
 *
 *   - Declaración de MX_GPIO_Init().
 *   - Definiciones de nombres de pines.
 *   - Definiciones de los puertos GPIO asociados.
 *
 * Ejemplos:
 *
 *   LD4_Pin
 *   LD4_GPIO_Port
 *   B1_Pin
 *   B1_GPIO_Port
 */
#include "gpio.h"


/* USER CODE BEGIN 0 */

/*
 * Espacio reservado para:
 *
 *   - Variables privadas.
 *   - Funciones auxiliares.
 *   - Macros relacionadas con GPIO.
 *
 * Todo lo colocado aquí será conservado por CubeMX.
 */

/* USER CODE END 0 */


/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/


/* USER CODE BEGIN 1 */

/*
 * Espacio reservado para código que deba ejecutarse o declararse
 * antes de la función MX_GPIO_Init().
 */

/* USER CODE END 1 */


/**
  * @brief Configura todos los pines GPIO seleccionados en CubeMX.
  *
  * La lista siguiente muestra algunos pines que utilizan funciones
  * alternativas en lugar de comportarse como entradas o salidas comunes.
  *
  * Una función alternativa conecta físicamente el pin a un periférico
  * interno del microcontrolador.
  *
  * Ejemplo:
  *
  *   PA6 configurado como SPI1_MISO
  *
  * significa que PA6 queda conectado internamente al periférico SPI1.
  *
  * Pines con función alternativa:
  *
  *     PC3   ------> I2S2_SD
  *     PA4   ------> I2S3_WS
  *     PA6   ------> SPI1_MISO
  *     PA7   ------> SPI1_MOSI
  *     PB10  ------> I2S2_CK
  *     PC7   ------> I2S3_MCK
  *     PA9   ------> USB_OTG_FS_VBUS
  *     PA10  ------> USB_OTG_FS_ID
  *     PA11  ------> USB_OTG_FS_DM
  *     PA12  ------> USB_OTG_FS_DP
  *     PC10  ------> I2S3_CK
  *     PC12  ------> I2S3_SD
  */
void MX_GPIO_Init(void)
{
  /*
   * Estructura utilizada por HAL_GPIO_Init().
   *
   * Antes de configurar cada grupo de pines, se llenan sus campos:
   *
   *   Pin
   *   Mode
   *   Pull
   *   Speed
   *   Alternate
   *
   * {0} inicializa todos los campos en cero.
   */
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  /* ------------------------------------------------------------------------ */
  /* Habilitación de relojes de los puertos GPIO                             */
  /* ------------------------------------------------------------------------ */

  /*
   * Cada puerto GPIO necesita que su reloj esté habilitado antes
   * de poder configurarse o utilizarse.
   *
   * Si el reloj de un puerto está apagado, sus registros no funcionan.
   */

  __HAL_RCC_GPIOE_CLK_ENABLE();  /* Habilitar reloj del puerto E. */
  __HAL_RCC_GPIOC_CLK_ENABLE();  /* Habilitar reloj del puerto C. */
  __HAL_RCC_GPIOH_CLK_ENABLE();  /* Habilitar reloj del puerto H. */
  __HAL_RCC_GPIOA_CLK_ENABLE();  /* Habilitar reloj del puerto A. */
  __HAL_RCC_GPIOB_CLK_ENABLE();  /* Habilitar reloj del puerto B. */
  __HAL_RCC_GPIOD_CLK_ENABLE();  /* Habilitar reloj del puerto D. */


  /* ------------------------------------------------------------------------ */
  /* Estados iniciales de las salidas                                        */
  /* ------------------------------------------------------------------------ */

  /*
   * Establecer el nivel inicial del pin CS_I2C_SPI.
   *
   * GPIO_PIN_RESET significa nivel lógico bajo.
   *
   * Este valor se escribe antes de configurar el pin como salida para
   * reducir la posibilidad de generar un pulso no deseado al arrancar.
   */
  HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port,
                    CS_I2C_SPI_Pin,
                    GPIO_PIN_RESET);


  /*
   * Activar inicialmente el interruptor de alimentación USB OTG.
   *
   * GPIO_PIN_SET significa nivel lógico alto.
   */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port,
                    OTG_FS_PowerSwitchOn_Pin,
                    GPIO_PIN_SET);


  /*
   * Colocar inicialmente en nivel bajo:
   *
   *   LD4
   *   LD3
   *   LD5
   *   LD6
   *   Audio_RST
   *
   * En la STM32F4 Discovery, los LEDs se encienden normalmente al
   * escribir nivel alto, por lo que este estado los deja apagados.
   */
  HAL_GPIO_WritePin(
      GPIOD,
      LD4_Pin |
      LD3_Pin |
      LD5_Pin |
      LD6_Pin |
      Audio_RST_Pin,
      GPIO_PIN_RESET);


  /* ------------------------------------------------------------------------ */
  /* Pin CS_I2C_SPI                                                          */
  /* ------------------------------------------------------------------------ */

  /*
   * Seleccionar el pin que será configurado.
   */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;

  /*
   * Salida push-pull:
   *
   * el STM32 puede forzar activamente tanto nivel alto como nivel bajo.
   */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

  /*
   * Sin resistencia pull-up ni pull-down interna.
   */
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  /*
   * Velocidad baja de transición.
   *
   * Es suficiente para señales digitales lentas y reduce ruido eléctrico.
   */
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /*
   * Aplicar la configuración al puerto correspondiente.
   */
  HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* Control de alimentación USB OTG                                         */
  /* ------------------------------------------------------------------------ */

  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* PDM_OUT / I2S2_SD                                                       */
  /* ------------------------------------------------------------------------ */

  /*
   * PDM_OUT no funciona como GPIO convencional.
   *
   * Se conecta al periférico SPI2/I2S2 mediante una función alternativa.
   */
  GPIO_InitStruct.Pin = PDM_OUT_Pin;

  /*
   * Alternate Function Push-Pull.
   *
   * El control del pin pasa del software GPIO al periférico SPI2.
   */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;

  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /*
   * AF5 selecciona la función SPI2/I2S2 asociada a ese pin.
   */
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;

  HAL_GPIO_Init(PDM_OUT_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* Botón de usuario B1                                                     */
  /* ------------------------------------------------------------------------ */

  GPIO_InitStruct.Pin = B1_Pin;

  /*
   * Entrada con interrupción por flanco ascendente.
   *
   * La interrupción se genera cuando el pin cambia:
   *
   *   nivel bajo -> nivel alto
   */
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;

  /*
   * No se habilita resistencia interna.
   *
   * La placa proporciona el circuito eléctrico necesario para el botón.
   */
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(B1_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* I2S3_WS                                                                 */
  /* ------------------------------------------------------------------------ */

  /*
   * Word Select del periférico I2S3.
   */
  GPIO_InitStruct.Pin = I2S3_WS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /*
   * AF6 conecta el pin al periférico SPI3/I2S3.
   */
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;

  HAL_GPIO_Init(I2S3_WS_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* SPI1: MISO y MOSI                                                       */
  /* ------------------------------------------------------------------------ */

  /*
   * Configurar simultáneamente:
   *
   *   SPI1_MISO
   *   SPI1_MOSI
   *
   * Ambos se encuentran en GPIOA.
   */
  GPIO_InitStruct.Pin =
      SPI1_MISO_Pin |
      SPI1_MOSI_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /*
   * AF5 selecciona SPI1.
   */
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;

  HAL_GPIO_Init(GPIOA,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* BOOT1                                                                   */
  /* ------------------------------------------------------------------------ */

  /*
   * BOOT1 se configura como entrada digital.
   *
   * Este pin puede participar en la selección del modo de arranque
   * del microcontrolador, dependiendo de la familia y configuración.
   */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(BOOT1_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* CLK_IN / I2S2_CK                                                       */
  /* ------------------------------------------------------------------------ */

  /*
   * Pin de reloj asociado a SPI2/I2S2.
   */
  GPIO_InitStruct.Pin = CLK_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;

  HAL_GPIO_Init(CLK_IN_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* LEDs y reset del circuito de audio                                      */
  /* ------------------------------------------------------------------------ */

  /*
   * Configurar simultáneamente como salidas digitales:
   *
   *   LD4
   *   LD3
   *   LD5
   *   LD6
   *   Audio_RST
   */
  GPIO_InitStruct.Pin =
      LD4_Pin |
      LD3_Pin |
      LD5_Pin |
      LD6_Pin |
      Audio_RST_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOD,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* I2S3: MCK, SCK y SD                                                     */
  /* ------------------------------------------------------------------------ */

  /*
   * Configurar tres señales del periférico I2S3:
   *
   *   MCK = Master Clock.
   *   SCK = Serial Clock.
   *   SD  = Serial Data.
   */
  GPIO_InitStruct.Pin =
      I2S3_MCK_Pin |
      I2S3_SCK_Pin |
      I2S3_SD_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;

  HAL_GPIO_Init(GPIOC,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* USB OTG FS VBUS                                                        */
  /* ------------------------------------------------------------------------ */

  /*
   * VBUS se configura como entrada.
   *
   * Permite detectar la presencia de alimentación USB.
   */
  GPIO_InitStruct.Pin = VBUS_FS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(VBUS_FS_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* USB OTG FS: ID, D- y D+                                                 */
  /* ------------------------------------------------------------------------ */

  /*
   * Configurar:
   *
   *   OTG_FS_ID
   *   OTG_FS_DM
   *   OTG_FS_DP
   *
   * como señales del periférico USB OTG FS.
   */
  GPIO_InitStruct.Pin =
      OTG_FS_ID_Pin |
      OTG_FS_DM_Pin |
      OTG_FS_DP_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  /*
   * AF10 conecta los pines al periférico USB OTG FS.
   */
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;

  HAL_GPIO_Init(GPIOA,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* Detección de sobrecorriente USB                                         */
  /* ------------------------------------------------------------------------ */

  /*
   * Entrada utilizada para detectar una condición de sobrecorriente
   * en la alimentación USB.
   */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port,
                &GPIO_InitStruct);


  /* ------------------------------------------------------------------------ */
  /* Interrupción o evento MEMS                                              */
  /* ------------------------------------------------------------------------ */

  GPIO_InitStruct.Pin = MEMS_INT2_Pin;

  /*
   * Entrada configurada como evento por flanco ascendente.
   *
   * GPIO_MODE_EVT_RISING genera un evento interno, pero no ejecuta
   * necesariamente una rutina de interrupción como GPIO_MODE_IT_RISING.
   *
   * Esto puede utilizarse, por ejemplo, para despertar el procesador
   * mediante una señal del sensor MEMS.
   */
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;

  GPIO_InitStruct.Pull = GPIO_NOPULL;

  HAL_GPIO_Init(MEMS_INT2_GPIO_Port,
                &GPIO_InitStruct);
}


/* USER CODE BEGIN 2 */

/*
 * Espacio reservado para código adicional que deba quedar asociado
 * a este archivo y conservarse después de regenerar el proyecto.
 */

/* USER CODE END 2 */
