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

/*
 * i2c.h contiene:
 *
 *   - La declaración global de hi2c1.
 *   - El prototipo de MX_I2C1_Init().
 *   - Tipos y funciones HAL relacionadas con I2C.
 *   - Referencias indirectas a main.h y al HAL del STM32.
 */
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


/* -------------------------------------------------------------------------- */
/* Manejador global de I2C1                                                   */
/* -------------------------------------------------------------------------- */

/*
 * Estructura principal utilizada por la biblioteca HAL para manejar I2C1.
 *
 * Contiene:
 *
 *   - La instancia física utilizada: I2C1.
 *   - La configuración de velocidad y direccionamiento.
 *   - El estado interno del periférico.
 *   - El último código de error.
 *   - Bloqueos internos utilizados por HAL.
 *
 * Este manejador se entrega a funciones como:
 *
 *   HAL_I2C_Init(&hi2c1);
 *   HAL_I2C_Mem_Read(&hi2c1, ...);
 *   HAL_I2C_Mem_Write(&hi2c1, ...);
 *   HAL_I2C_IsDeviceReady(&hi2c1, ...);
 */
I2C_HandleTypeDef hi2c1;


/* -------------------------------------------------------------------------- */
/* Inicialización lógica del periférico I2C1                                  */
/* -------------------------------------------------------------------------- */

/**
  * @brief Inicializa el periférico I2C1.
  *
  * Esta función configura:
  *
  *   - Velocidad del bus: 100 kHz.
  *   - Direccionamiento de 7 bits.
  *   - Modo de dirección única.
  *   - General Call deshabilitado.
  *   - Clock stretching permitido.
  *
  * HAL_I2C_Init() llama internamente a HAL_I2C_MspInit(), donde se
  * configuran los pines PB8 y PB9 y se habilita el reloj de I2C1.
  */
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


  /*
   * Seleccionar la instancia física I2C1 del STM32F407.
   */
  hi2c1.Instance = I2C1;


  /*
   * Configurar la frecuencia del bus en 100000 Hz:
   *
   *   100000 Hz = 100 kHz
   *
   * Esta frecuencia corresponde al modo estándar de I2C.
   */
  hi2c1.Init.ClockSpeed = 100000;


  /*
   * Configuración del ciclo útil del reloj.
   *
   * I2C_DUTYCYCLE_2 representa una relación aproximada de 2:1.
   *
   * Este parámetro tiene relevancia principalmente en Fast Mode.
   * Para la configuración actual de 100 kHz, el periférico trabaja
   * en Standard Mode.
   */
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;


  /*
   * Dirección propia del STM32 cuando funciona como esclavo.
   *
   * En este proyecto el STM32 funciona como maestro I2C, por lo que
   * no necesita una dirección propia y se utiliza cero.
   */
  hi2c1.Init.OwnAddress1 = 0;


  /*
   * Seleccionar direccionamiento I2C de 7 bits.
   *
   * Las direcciones del LSM9DS1 se manejan internamente como valores
   * de 7 bits y se desplazan una posición antes de pasarlas a HAL.
   */
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;


  /*
   * Deshabilitar el modo de doble dirección.
   *
   * Este modo solamente es útil cuando el STM32 trabaja como esclavo
   * y debe responder a dos direcciones distintas.
   */
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;


  /*
   * Segunda dirección propia.
   *
   * No se utiliza porque el modo de doble dirección está deshabilitado.
   */
  hi2c1.Init.OwnAddress2 = 0;


  /*
   * Deshabilitar la recepción de General Call.
   *
   * General Call corresponde a la dirección reservada 0x00 utilizada
   * para dirigirse simultáneamente a varios dispositivos.
   */
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;


  /*
   * Permitir clock stretching.
   *
   * Aunque el nombre pueda resultar confuso:
   *
   *   I2C_NOSTRETCH_DISABLE
   *
   * significa que el modo "sin estiramiento" está deshabilitado.
   * Por tanto, el periférico puede permitir que un esclavo mantenga
   * SCL en nivel bajo mientras termina una operación.
   */
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;


  /*
   * Inicializar el periférico con la configuración anterior.
   *
   * HAL_I2C_Init() también llama a HAL_I2C_MspInit(), encargada de:
   *
   *   - Habilitar el reloj de GPIOB.
   *   - Configurar PB8 y PB9.
   *   - Habilitar el reloj de I2C1.
   */
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    /*
     * Si la inicialización falla, ejecutar el manejador general
     * de errores del proyecto.
     *
     * Normalmente Error_Handler() detiene o bloquea el sistema para
     * evitar continuar con un periférico mal inicializado.
     */
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


/* -------------------------------------------------------------------------- */
/* Inicialización de bajo nivel: GPIO y reloj                                 */
/* -------------------------------------------------------------------------- */

/**
  * @brief Inicialización de bajo nivel del periférico I2C.
  *
  * Esta función es llamada automáticamente por HAL_I2C_Init().
  *
  * Para I2C1 realiza:
  *
  *   - Habilitación del reloj de GPIOB.
  *   - Configuración de PB8 como SCL.
  *   - Configuración de PB9 como SDA.
  *   - Selección de la función alternativa AF4.
  *   - Habilitación del reloj del periférico I2C1.
  *
  * @param i2cHandle Manejador del periférico que se está inicializando.
  */
void HAL_I2C_MspInit(I2C_HandleTypeDef* i2cHandle)
{
  /*
   * Estructura temporal utilizada para configurar los pines GPIO.
   *
   * {0} inicializa todos sus campos en cero antes de utilizarlos.
   */
  GPIO_InitTypeDef GPIO_InitStruct = {0};


  /*
   * Comprobar que la función fue llamada para la instancia I2C1.
   *
   * La misma función podría ampliarse en el futuro para manejar
   * I2C2 o I2C3.
   */
  if (i2cHandle->Instance == I2C1)
  {
    /* USER CODE BEGIN I2C1_MspInit 0 */

    /*
     * Espacio para código que deba ejecutarse antes de configurar
     * físicamente I2C1.
     */

    /* USER CODE END I2C1_MspInit 0 */


    /*
     * Habilitar el reloj del puerto GPIOB.
     *
     * Es obligatorio antes de acceder a los registros de PB8 y PB9.
     */
    __HAL_RCC_GPIOB_CLK_ENABLE();


    /*
     * Configuración física:
     *
     *   PB8 -> I2C1_SCL
     *   PB9 -> I2C1_SDA
     *
     * SCL transmite el reloj del bus.
     * SDA transporta los datos y las confirmaciones ACK/NACK.
     */


    /*
     * Seleccionar simultáneamente PB8 y PB9.
     */
    GPIO_InitStruct.Pin =
        GPIO_PIN_8 |
        GPIO_PIN_9;


    /*
     * Función alternativa open-drain:
     *
     *   GPIO_MODE_AF:
     *     el pin será controlado por un periférico interno.
     *
     *   OD, Open Drain:
     *     el pin puede forzar nivel bajo, pero no fuerza activamente
     *     el nivel alto.
     *
     * Las líneas I2C deben ser open-drain para permitir que varios
     * dispositivos compartan el mismo bus sin conflicto eléctrico.
     */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;


    /*
     * No habilitar resistencias pull-up internas.
     *
     * Como I2C utiliza salidas open-drain, el bus necesita resistencias
     * pull-up hacia la alimentación.
     *
     * Con GPIO_NOPULL se supone que existen pull-ups externos en la placa,
     * en el módulo de la IMU o en el circuito del FloatSat.
     */
    GPIO_InitStruct.Pull = GPIO_NOPULL;


    /*
     * Velocidad eléctrica o slew rate de los pines.
     *
     * GPIO_SPEED_FREQ_VERY_HIGH no establece la frecuencia del bus I2C.
     * La frecuencia real sigue siendo 100 kHz y está configurada mediante:
     *
     *   hi2c1.Init.ClockSpeed
     *
     * Este campo solamente controla la rapidez de transición del GPIO.
     */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;


    /*
     * Seleccionar la función alternativa número 4.
     *
     * En PB8 y PB9, AF4 conecta los pines internamente con I2C1.
     */
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;


    /*
     * Aplicar la configuración anterior a GPIOB.
     */
    HAL_GPIO_Init(GPIOB,
                  &GPIO_InitStruct);


    /*
     * Habilitar el reloj interno del periférico I2C1.
     *
     * Sin este reloj, I2C1 no puede ejecutar transacciones.
     */
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


/* -------------------------------------------------------------------------- */
/* Desinicialización de bajo nivel                                            */
/* -------------------------------------------------------------------------- */

/**
  * @brief Desinicializa los recursos de bajo nivel de I2C.
  *
  * Esta función es llamada automáticamente por HAL_I2C_DeInit().
  *
  * Para I2C1:
  *
  *   - Deshabilita el reloj del periférico.
  *   - Libera PB8.
  *   - Libera PB9.
  *
  * @param i2cHandle Manejador del periférico que se está desinicializando.
  */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* i2cHandle)
{
  /*
   * Ejecutar esta sección solamente para I2C1.
   */
  if (i2cHandle->Instance == I2C1)
  {
    /* USER CODE BEGIN I2C1_MspDeInit 0 */

    /*
     * Espacio reservado antes de desactivar el periférico.
     */

    /* USER CODE END I2C1_MspDeInit 0 */


    /*
     * Deshabilitar el reloj interno de I2C1.
     *
     * Después de esto, los registros del periférico dejan de operar
     * hasta que el reloj se habilite nuevamente.
     */
    __HAL_RCC_I2C1_CLK_DISABLE();


    /*
     * Liberar la configuración de PB8:
     *
     *   PB8 -> I2C1_SCL
     */
    HAL_GPIO_DeInit(GPIOB,
                    GPIO_PIN_8);


    /*
     * Liberar la configuración de PB9:
     *
     *   PB9 -> I2C1_SDA
     */
    HAL_GPIO_DeInit(GPIOB,
                    GPIO_PIN_9);


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
