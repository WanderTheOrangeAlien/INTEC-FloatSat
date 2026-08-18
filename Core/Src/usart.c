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

/*
 * usart.h declara:
 *
 *   - El manejador global huart1.
 *   - La función MX_USART1_UART_Init().
 *   - Tipos y funciones HAL relacionados con UART/USART.
 *
 * Normalmente también incluye indirectamente main.h, donde se declara
 * Error_Handler().
 */
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


/* Variables -----------------------------------------------------------------*/

/**
 * @brief Manejador global de USART1.
 *
 * La estructura almacena:
 *
 *   - La instancia física USART1.
 *   - Los parámetros de comunicación.
 *   - El estado de transmisión y recepción.
 *   - Punteros a buffers cuando se usan interrupciones o DMA.
 *   - Códigos de error de la HAL.
 *
 * Se declara globalmente para que otros archivos puedan utilizar USART1
 * mediante:
 *
 *   extern UART_HandleTypeDef huart1;
 */
UART_HandleTypeDef huart1;


/* USART1 init function ------------------------------------------------------*/

/**
 * @brief Inicializa USART1 en modo UART asíncrono.
 *
 * Configuración resultante:
 *
 *   Baud rate:          9600 bit/s.
 *   Longitud de palabra: 8 bits.
 *   Bits de parada:      1.
 *   Paridad:             ninguna.
 *   Dirección:           transmisión y recepción.
 *   Control de flujo:    ninguno.
 *   Sobremuestreo:       16 muestras por bit.
 *
 * La llamada HAL_UART_Init() ejecuta internamente HAL_UART_MspInit(),
 * donde se habilitan:
 *
 *   - El reloj del periférico USART1.
 *   - El reloj de GPIOB.
 *   - PB6 como USART1_TX.
 *   - PB7 como USART1_RX.
 */
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


  /*
   * Asociar el manejador huart1 con el periférico físico USART1.
   */
  huart1.Instance = USART1;


  /*
   * Velocidad nominal de comunicación:
   *
   *   9600 bits por segundo.
   *
   * El dispositivo conectado debe utilizar exactamente el mismo baud rate.
   */
  huart1.Init.BaudRate = 9600;


  /*
   * Cada trama contiene 8 bits de datos útiles.
   *
   * Como no se utiliza paridad, la palabra transmitida contiene
   * exactamente 8 bits de información.
   */
  huart1.Init.WordLength = UART_WORDLENGTH_8B;


  /*
   * Utilizar un único bit de parada al final de cada trama.
   */
  huart1.Init.StopBits = UART_STOPBITS_1;


  /*
   * No agregar ni comprobar bit de paridad.
   *
   * La letra "N" de 8N1 significa "No parity".
   */
  huart1.Init.Parity = UART_PARITY_NONE;


  /*
   * Habilitar las dos direcciones de comunicación:
   *
   *   TX -> transmisión.
   *   RX -> recepción.
   */
  huart1.Init.Mode = UART_MODE_TX_RX;


  /*
   * No utilizar líneas RTS ni CTS para control de flujo.
   *
   * La comunicación solamente emplea:
   *
   *   TX.
   *   RX.
   *   GND común.
   */
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;


  /*
   * Utilizar sobremuestreo por 16.
   *
   * El receptor toma múltiples muestras de cada bit para determinar
   * su valor lógico y mejorar la tolerancia a pequeñas diferencias
   * entre los relojes de ambos dispositivos.
   */
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;


  /*
   * Inicializar USART1 con los parámetros anteriores.
   *
   * Durante esta llamada, la HAL también ejecutará HAL_UART_MspInit()
   * para habilitar el reloj y configurar PB6 y PB7.
   */
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    /*
     * Cualquier fallo de inicialización lleva al manejador global
     * de errores del proyecto.
     */
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


/* Inicialización MSP --------------------------------------------------------*/

/**
 * @brief Configura los recursos hardware utilizados por USART1.
 *
 * MSP significa MCU Support Package.
 *
 * Esta función es llamada automáticamente por HAL_UART_Init().
 *
 * Configura:
 *
 *   - Reloj de USART1.
 *   - Reloj de GPIOB.
 *   - PB6 como USART1_TX.
 *   - PB7 como USART1_RX.
 *
 * @param uartHandle Manejador del periférico que se está inicializando.
 */
void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
  /*
   * Estructura temporal utilizada para configurar los pines GPIO.
   *
   * La inicialización con {0} evita que queden campos sin definir.
   */
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  /*
   * Comprobar que la inicialización solicitada corresponde a USART1.
   */
  if (uartHandle->Instance == USART1)
  {
    /* USER CODE BEGIN USART1_MspInit 0 */

    /*
     * Espacio reservado para código anterior a la habilitación
     * del periférico.
     */

    /* USER CODE END USART1_MspInit 0 */


    /*
     * Habilitar el reloj del periférico USART1.
     *
     * USART1 pertenece al bus APB2.
     *
     * Sin este reloj, los registros de USART1 no funcionan.
     */
    __HAL_RCC_USART1_CLK_ENABLE();


    /*
     * Habilitar el reloj del puerto GPIOB.
     *
     * Es necesario antes de configurar PB6 y PB7.
     */
    __HAL_RCC_GPIOB_CLK_ENABLE();


    /*
     * Configuración física:
     *
     *   PB6 -> USART1_TX.
     *   PB7 -> USART1_RX.
     */


    /*
     * Seleccionar simultáneamente PB6 y PB7.
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_6 |
        GPIO_PIN_7;


    /*
     * Configurar ambos pines en modo función alternativa.
     *
     * GPIO_MODE_AF_PP conecta los pines directamente con USART1.
     *
     * En PB6, el periférico controla la señal transmitida.
     * En PB7, USART1 recibe el estado presente en el pin.
     */
    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_PP;


    /*
     * No habilitar resistencias pull-up ni pull-down internas.
     *
     * El dispositivo conectado debe mantener niveles eléctricos definidos,
     * especialmente en la línea RX.
     */
    GPIO_InitStruct.Pull =
        GPIO_NOPULL;


    /*
     * Seleccionar velocidad eléctrica muy alta para los GPIO.
     *
     * Este parámetro controla la rapidez de transición de la salida y no
     * cambia el baud rate configurado en USART1.
     */
    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_VERY_HIGH;


    /*
     * AF7 conecta PB6 y PB7 con el periférico USART1.
     */
    GPIO_InitStruct.Alternate =
        GPIO_AF7_USART1;


    /*
     * Aplicar la configuración a PB6 y PB7.
     */
    HAL_GPIO_Init(
        GPIOB,
        &GPIO_InitStruct);


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


/* Desinicialización MSP -----------------------------------------------------*/

/**
 * @brief Libera los recursos hardware utilizados por USART1.
 *
 * Esta función puede ser llamada internamente por HAL_UART_DeInit().
 *
 * @param uartHandle Manejador del periférico que será desinicializado.
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle)
{
  /*
   * Ejecutar esta sección únicamente para USART1.
   */
  if (uartHandle->Instance == USART1)
  {
    /* USER CODE BEGIN USART1_MspDeInit 0 */

    /*
     * Espacio reservado antes de liberar USART1.
     */

    /* USER CODE END USART1_MspDeInit 0 */


    /*
     * Deshabilitar el reloj del periférico USART1.
     *
     * Después de esta instrucción USART1 deja de funcionar.
     */
    __HAL_RCC_USART1_CLK_DISABLE();


    /*
     * Liberar los GPIO utilizados por USART1:
     *
     *   PB6 -> USART1_TX.
     *   PB7 -> USART1_RX.
     *
     * Los pines vuelven al estado de desinicialización definido por HAL.
     */
    HAL_GPIO_DeInit(
        GPIOB,
        GPIO_PIN_6 |
        GPIO_PIN_7);


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
