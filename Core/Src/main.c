/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Punto de entrada principal de la aplicación.
  *
  * Este archivo realiza la secuencia general de arranque:
  *
  *   1. Inicializa la biblioteca HAL.
  *   2. Configura el reloj del microcontrolador.
  *   3. Inicializa GPIO, I2C, temporizadores y UART.
  *   4. Inicializa el kernel de FreeRTOS.
  *   5. Crea las tareas y objetos del sistema.
  *   6. Inicia el planificador de FreeRTOS.
  *
  * Después de iniciar el scheduler, la ejecución normal queda repartida
  * entre las tareas definidas en freertos.c.
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
 * main.h contiene:
 *
 *   - Definiciones generales del proyecto.
 *   - Nombres de pines y puertos GPIO.
 *   - Prototipo de Error_Handler().
 *   - Inclusión principal de la biblioteca HAL.
 */
#include "main.h"

/*
 * cmsis_os.h proporciona la API CMSIS-RTOS utilizada para:
 *
 *   - Inicializar el kernel.
 *   - Crear tareas.
 *   - Iniciar el scheduler.
 *   - Aplicar retardos desde las tareas.
 */
#include "cmsis_os.h"

/*
 * Declaraciones de:
 *
 *   hi2c1
 *   MX_I2C1_Init()
 *
 * I2C1 se utiliza para comunicarse con la IMU.
 */
#include "i2c.h"

/*
 * Declaraciones de los temporizadores configurados.
 *
 * En este proyecto:
 *
 *   TIM1 se utiliza para generar PWM del motor.
 *   TIM3 se utiliza para la lectura del encoder.
 */
#include "tim.h"

/*
 * Declaraciones del periférico UART.
 *
 * USART1 se utiliza para comunicación serial, por ejemplo con el HC-05
 * o para transmisión de telemetría.
 */
#include "usart.h"

/*
 * Declaración de MX_GPIO_Init() y definiciones de los GPIO utilizados.
 */
#include "gpio.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/*
 * Espacio reservado para incluir librerías adicionales.
 *
 * El contenido colocado aquí será conservado por STM32CubeMX.
 */

/* USER CODE END Includes */


/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/*
 * Espacio reservado para declarar tipos privados:
 *
 *   typedef struct
 *   typedef enum
 *   alias de tipos
 */

/* USER CODE END PTD */


/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * Espacio reservado para constantes privadas definidas mediante #define.
 */

/* USER CODE END PD */


/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/*
 * Espacio reservado para macros privadas.
 */

/* USER CODE END PM */


/* Private variables ---------------------------------------------------------*/

/*
 * No existen variables privadas generadas directamente en este archivo.
 *
 * Las variables principales del sistema se encuentran en otros módulos,
 * especialmente freertos.c y los drivers correspondientes.
 */

/* USER CODE BEGIN PV */

/*
 * Espacio reservado para variables privadas del usuario.
 */

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/

/*
 * Configura los osciladores, PLL y divisores de los buses.
 */
void SystemClock_Config(void);

/*
 * Inicializa los objetos de FreeRTOS y crea las tareas.
 *
 * Su implementación está en freertos.c.
 */
void MX_FREERTOS_Init(void);

/* USER CODE BEGIN PFP */

/*
 * Espacio reservado para prototipos de funciones privadas adicionales.
 */

/* USER CODE END PFP */


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * Espacio reservado para funciones auxiliares o código privado
 * colocado antes de main().
 */

/* USER CODE END 0 */


/**
  * @brief Punto de entrada principal de la aplicación.
  *
  * La función main() solamente realiza la configuración inicial.
  * Después de iniciar FreeRTOS, la lógica permanente se ejecuta
  * dentro de las diferentes tareas.
  *
  * @retval int
  *         En funcionamiento normal esta función nunca retorna.
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /*
   * Espacio reservado para variables locales o instrucciones que deban
   * ejecutarse antes de HAL_Init().
   *
   * En esta etapa todavía no se han inicializado los periféricos.
   */

  /* USER CODE END 1 */


  /* MCU Configuration--------------------------------------------------------*/


  /*
   * Inicializar la biblioteca HAL.
   *
   * HAL_Init() realiza tareas básicas como:
   *
   *   - Reinicializar el estado de los periféricos HAL.
   *   - Configurar la base de tiempo utilizada por HAL.
   *   - Configurar prioridades iniciales del sistema.
   *   - Inicializar aspectos básicos del controlador de Flash.
   *
   * En este proyecto, el incremento del tick HAL se realiza mediante TIM6,
   * como se observa en HAL_TIM_PeriodElapsedCallback().
   */
  HAL_Init();


  /* USER CODE BEGIN Init */

  /*
   * Espacio reservado para código inmediatamente posterior a HAL_Init()
   * y anterior a la configuración del reloj principal.
   */

  /* USER CODE END Init */


  /*
   * Configurar el reloj del microcontrolador.
   *
   * Esta función selecciona:
   *
   *   - Fuente HSE externa en modo bypass.
   *   - PLL como multiplicador principal.
   *   - Divisores de AHB, APB1 y APB2.
   */
  SystemClock_Config();


  /* USER CODE BEGIN SysInit */

  /*
   * Espacio reservado para código posterior a la configuración del reloj
   * y anterior a la inicialización de los periféricos.
   */

  /* USER CODE END SysInit */


  /* Initialize all configured peripherals */


  /*
   * Inicializar todos los GPIO:
   *
   *   - LEDs.
   *   - Botón.
   *   - USB.
   *   - Pines de audio.
   *   - Pines con funciones alternativas.
   */
  MX_GPIO_Init();


  /*
   * Inicializar I2C1:
   *
   *   PB8 -> SCL.
   *   PB9 -> SDA.
   *   Frecuencia -> 100 kHz.
   *
   * Se utiliza para la comunicación con la IMU.
   */
  MX_I2C1_Init();


  /*
   * Inicializar TIM1.
   *
   * En el proyecto FloatSat, TIM1 proporciona los dos canales PWM
   * utilizados para controlar los sentidos del motor.
   *
   * Esta función configura el temporizador, pero el inicio de los canales
   * PWM suele realizarse posteriormente mediante HAL_TIM_PWM_Start().
   */
  MX_TIM1_Init();


  /*
   * Inicializar TIM3.
   *
   * En el proyecto se utiliza como interfaz de encoder.
   *
   * Esta función configura el periférico, pero el conteo debe iniciarse
   * posteriormente con la función HAL correspondiente.
   */
  MX_TIM3_Init();


  /*
   * Inicializar USART1.
   *
   * Se utiliza para comunicación serial y telemetría.
   */
  MX_USART1_UART_Init();


  /* USER CODE BEGIN 2 */

  /*
   * Espacio reservado para código posterior a la inicialización
   * de todos los periféricos y anterior a FreeRTOS.
   *
   * En esta sección podrían iniciarse periféricos que deban estar activos
   * antes del scheduler, siempre que no se utilicen funciones RTOS.
   */

  /* USER CODE END 2 */


  /* Init scheduler */


  /*
   * Inicializar el kernel CMSIS-RTOS.
   *
   * Después de esta llamada pueden crearse tareas, colas, mutexes
   * y otros objetos del sistema operativo.
   *
   * El scheduler todavía no comienza a ejecutar tareas.
   */
  osKernelInitialize();


  /*
   * Crear los objetos de FreeRTOS definidos en freertos.c.
   *
   * En el código mostrado anteriormente esta función crea:
   *
   *   - SensorTask.
   *   - ControlTask.
   *   - SupervisorTask.
   *   - MissionTask.
   *   - TelemetryTask.
   */
  MX_FREERTOS_Init();


  /* Start scheduler */


  /*
   * Iniciar el scheduler de FreeRTOS.
   *
   * A partir de este punto, el kernel selecciona qué tarea debe ejecutarse
   * según su prioridad y estado.
   *
   * En funcionamiento normal, osKernelStart() no retorna.
   */
  osKernelStart();


  /*
   * Si la ejecución llega hasta aquí, significa que el scheduler:
   *
   *   - No pudo iniciar.
   *   - Terminó de forma inesperada.
   *   - Se produjo algún problema grave de configuración.
   */


  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  /*
   * Bucle de seguridad.
   *
   * No debería ejecutarse durante el funcionamiento normal porque
   * el control debería permanecer en FreeRTOS.
   */
  while (1)
  {
    /* USER CODE END WHILE */


    /* USER CODE BEGIN 3 */

    /*
     * Espacio reservado para código dentro del bucle de seguridad.
     *
     * No es recomendable colocar aquí lógica normal de la aplicación,
     * porque no se ejecutará mientras el scheduler funcione correctamente.
     */

  }

  /* USER CODE END 3 */
}


/* -------------------------------------------------------------------------- */
/* Configuración del reloj del sistema                                        */
/* -------------------------------------------------------------------------- */

/**
  * @brief Configura el reloj principal del STM32.
  *
  * La configuración utiliza:
  *
  *   - HSE externo en modo bypass.
  *   - PLL activado.
  *   - PLL como fuente del reloj del sistema.
  *
  * Parámetros:
  *
  *   PLLM = 8
  *   PLLN = 336
  *   PLLP = 2
  *   PLLQ = 7
  *
  * Si la entrada HSE es de 8 MHz:
  *
  *   PLL entrada = 8 MHz / 8 = 1 MHz
  *   PLL VCO     = 1 MHz × 336 = 336 MHz
  *   SYSCLK      = 336 MHz / 2 = 168 MHz
  *   USB clock   = 336 MHz / 7 = 48 MHz
  *
  * @retval None
  */
void SystemClock_Config(void)
{
  /*
   * Estructura que contiene la configuración de los osciladores
   * y del PLL.
   */
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /*
   * Estructura que contiene la fuente del reloj del sistema
   * y los divisores de los buses.
   */
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};


  /*
   * Habilitar el reloj del bloque de alimentación PWR.
   *
   * Es necesario para configurar la escala de voltaje interna.
   */
  __HAL_RCC_PWR_CLK_ENABLE();


  /*
   * Seleccionar Voltage Scale 1.
   *
   * Esta escala permite operar el STM32F4 a las frecuencias más altas
   * previstas por esta configuración.
   */
  __HAL_PWR_VOLTAGESCALING_CONFIG(
      PWR_REGULATOR_VOLTAGE_SCALE1);


  /*
   * Seleccionar el oscilador externo de alta velocidad HSE.
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSE;


  /*
   * Configurar HSE en modo bypass.
   *
   * En este modo el STM32 espera recibir una señal de reloj externa
   * ya generada, en lugar de controlar directamente un cristal.
   */
  RCC_OscInitStruct.HSEState =
      RCC_HSE_BYPASS;


  /*
   * Activar el PLL.
   */
  RCC_OscInitStruct.PLL.PLLState =
      RCC_PLL_ON;


  /*
   * Utilizar HSE como fuente de entrada del PLL.
   */
  RCC_OscInitStruct.PLL.PLLSource =
      RCC_PLLSOURCE_HSE;


  /*
   * Dividir la frecuencia de entrada HSE entre 8 antes de entrar al PLL.
   */
  RCC_OscInitStruct.PLL.PLLM = 8;


  /*
   * Multiplicar la frecuencia resultante por 336.
   */
  RCC_OscInitStruct.PLL.PLLN = 336;


  /*
   * Dividir la salida principal del PLL entre 2 para producir SYSCLK.
   */
  RCC_OscInitStruct.PLL.PLLP =
      RCC_PLLP_DIV2;


  /*
   * Dividir la salida VCO del PLL entre 7.
   *
   * Este reloj se utiliza para periféricos que requieren 48 MHz,
   * como USB OTG FS.
   */
  RCC_OscInitStruct.PLL.PLLQ = 7;


  /*
   * Aplicar la configuración de osciladores y PLL.
   */
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /*
     * Si el oscilador o PLL no pueden configurarse, detener el sistema.
     */
    Error_Handler();
  }


  /*
   * Seleccionar qué relojes serán configurados:
   *
   *   SYSCLK -> reloj principal.
   *   HCLK   -> bus AHB y CPU.
   *   PCLK1  -> bus APB1.
   *   PCLK2  -> bus APB2.
   */
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;


  /*
   * Utilizar la salida del PLL como reloj principal del sistema.
   */
  RCC_ClkInitStruct.SYSCLKSource =
      RCC_SYSCLKSOURCE_PLLCLK;


  /*
   * AHB sin división:
   *
   *   HCLK = SYSCLK
   *
   * Con SYSCLK de 168 MHz:
   *
   *   HCLK = 168 MHz
   */
  RCC_ClkInitStruct.AHBCLKDivider =
      RCC_SYSCLK_DIV1;


  /*
   * Dividir HCLK entre 4 para APB1:
   *
   *   PCLK1 = 168 MHz / 4 = 42 MHz
   *
   * APB1 alimenta periféricos como:
   *
   *   - I2C1.
   *   - TIM3.
   *   - USART2/3.
   */
  RCC_ClkInitStruct.APB1CLKDivider =
      RCC_HCLK_DIV4;


  /*
   * Dividir HCLK entre 2 para APB2:
   *
   *   PCLK2 = 168 MHz / 2 = 84 MHz
   *
   * APB2 alimenta periféricos como:
   *
   *   - TIM1.
   *   - USART1.
   *   - SPI1.
   */
  RCC_ClkInitStruct.APB2CLKDivider =
      RCC_HCLK_DIV2;


  /*
   * Aplicar la configuración de relojes.
   *
   * FLASH_LATENCY_5 configura cinco estados de espera de Flash,
   * necesarios para trabajar a la frecuencia seleccionada.
   */
  if (HAL_RCC_ClockConfig(
          &RCC_ClkInitStruct,
          FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}


/* USER CODE BEGIN 4 */

/*
 * Espacio reservado para funciones adicionales del usuario.
 *
 * El contenido será conservado por STM32CubeMX.
 */

/* USER CODE END 4 */


/* -------------------------------------------------------------------------- */
/* Callback de la base de tiempo HAL                                          */
/* -------------------------------------------------------------------------- */

/**
  * @brief Callback ejecutado al completarse un periodo de temporizador.
  *
  * Esta función es llamada desde HAL_TIM_IRQHandler().
  *
  * En este proyecto, TIM6 se utiliza como base de tiempo de HAL.
  * Cada interrupción de TIM6 incrementa la variable global uwTick.
  *
  * uwTick es utilizada por funciones como:
  *
  *   HAL_GetTick()
  *   HAL_Delay()
  *   timeouts de HAL_I2C
  *   timeouts de HAL_UART
  *
  * @param htim Puntero al manejador del temporizador que generó
  *             la interrupción.
  *
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /*
   * Espacio reservado para código que deba ejecutarse antes
   * de la lógica generada.
   *
   * Este callback se ejecuta en contexto de interrupción.
   * No deben utilizarse aquí funciones bloqueantes como osDelay().
   */

  /* USER CODE END Callback 0 */


  /*
   * Comprobar que la interrupción proviene específicamente de TIM6.
   */
  if (htim->Instance == TIM6)
  {
    /*
     * Incrementar el contador de tiempo de HAL en un tick.
     *
     * Normalmente el tick está configurado para representar 1 ms.
     */
    HAL_IncTick();
  }


  /* USER CODE BEGIN Callback 1 */

  /*
   * Espacio reservado para código posterior al incremento del tick.
   *
   * También se ejecuta en contexto de interrupción.
   */

  /* USER CODE END Callback 1 */
}


/* -------------------------------------------------------------------------- */
/* Manejador general de errores                                               */
/* -------------------------------------------------------------------------- */

/**
  * @brief Se ejecuta cuando ocurre un error crítico de inicialización.
  *
  * La implementación actual:
  *
  *   1. Deshabilita todas las interrupciones.
  *   2. Entra en un bucle infinito.
  *
  * Esto detiene completamente la ejecución del sistema para evitar
  * continuar con una configuración inválida.
  *
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

  /*
   * Deshabilitar globalmente las interrupciones.
   *
   * Después de esta instrucción:
   *
   *   - FreeRTOS no puede realizar cambios de tarea.
   *   - Los temporizadores no ejecutan callbacks.
   *   - La comunicación por interrupciones queda detenida.
   */
  __disable_irq();


  /*
   * Permanecer bloqueado indefinidamente.
   */
  while (1)
  {
    /*
     * Aquí podrían agregarse señales de diagnóstico, aunque con las
     * interrupciones deshabilitadas solo deberían usarse operaciones
     * directas y no bloqueantes.
     */
  }

  /* USER CODE END Error_Handler_Debug */
}


/* -------------------------------------------------------------------------- */
/* Diagnóstico de aserciones HAL                                              */
/* -------------------------------------------------------------------------- */

#ifdef USE_FULL_ASSERT

/**
  * @brief Informa dónde ocurrió una aserción inválida.
  *
  * Esta función solo se compila cuando USE_FULL_ASSERT está definido.
  *
  * Las macros assert_param() de HAL pueden llamar esta función cuando
  * reciben un parámetro fuera del rango permitido.
  *
  * @param file Nombre del archivo fuente donde ocurrió el error.
  * @param line Número de línea donde ocurrió el error.
  *
  * @retval None
  */
void assert_failed(uint8_t *file,
                   uint32_t line)
{
  /* USER CODE BEGIN 6 */

  /*
   * Espacio reservado para reportar el error.
   *
   * Ejemplo:
   *
   * printf("Parametro incorrecto: archivo %s, linea %lu\r\n",
   *        file,
   *        line);
   *
   * En la implementación actual no se realiza ninguna acción.
   */

  /*
   * Los parámetros no se utilizan actualmente.
   *
   * Dependiendo de las opciones del compilador, esto podría generar
   * advertencias por parámetros sin usar.
   */

  /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
