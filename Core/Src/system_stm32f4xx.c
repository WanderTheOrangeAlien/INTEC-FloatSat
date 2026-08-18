/**
  ******************************************************************************
  * @file    system_stm32f4xx.c
  * @author  MCD Application Team
  * @brief   Inicialización básica del núcleo Cortex-M4 y cálculo del reloj.
  *
  * Este archivo forma parte de CMSIS y se ejecuta en una etapa muy temprana
  * del arranque del microcontrolador.
  *
  * Proporciona principalmente:
  *
  *   - SystemInit():
  *       llamada desde startup_stm32f4xx.s antes de llegar a main().
  *
  *   - SystemCoreClock:
  *       variable global que representa la frecuencia HCLK del núcleo.
  *
  *   - SystemCoreClockUpdate():
  *       recalcula SystemCoreClock leyendo los registros RCC.
  *
  * También contiene código opcional para inicializar SRAM o SDRAM externa.
  * En la configuración actual esas secciones están deshabilitadas.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


/**
  * @addtogroup CMSIS
  * @{
  *
  * Estas etiquetas forman grupos para la documentación generada por Doxygen.
  */


/**
  * @addtogroup stm32f4xx_system
  * @{
  */


/**
  * @addtogroup STM32F4xx_System_Private_Includes
  * @{
  */


/*
 * Cabecera CMSIS específica de la familia STM32F4.
 *
 * Proporciona:
 *
 *   - Estructuras de registros: RCC, SCB, GPIO, FMC, FSMC.
 *   - Direcciones base de Flash y SRAM.
 *   - Definiciones del núcleo Cortex-M4.
 *   - Macros asociadas al dispositivo seleccionado.
 */
#include "stm32f4xx.h"


/*
 * Frecuencia declarada del oscilador externo HSE.
 *
 * Este valor solamente se define aquí cuando no fue definido previamente
 * por el proyecto o por las opciones del compilador.
 *
 * Debe coincidir con la frecuencia real aplicada al pin HSE.
 */
#if !defined(HSE_VALUE)
  #define HSE_VALUE ((uint32_t)25000000)
#endif /* HSE_VALUE */


/*
 * Frecuencia nominal del oscilador interno HSI.
 *
 * En STM32F4 es nominalmente 16 MHz.
 */
#if !defined(HSI_VALUE)
  #define HSI_VALUE ((uint32_t)16000000)
#endif /* HSI_VALUE */


/**
  * @}
  */


/**
  * @addtogroup STM32F4xx_System_Private_TypesDefinitions
  * @{
  */

/*
 * No se definen tipos privados adicionales.
 */

/**
  * @}
  */


/**
  * @addtogroup STM32F4xx_System_Private_Defines
  * @{
  */


/************************* Miscellaneous Configuration ************************/

/*
 * Los bloques siguientes permiten utilizar memoria externa como memoria
 * de datos, heap o stack.
 *
 * En el proyecto actual las macros están comentadas, por lo que ninguna
 * inicialización de SRAM o SDRAM externa será compilada.
 */


/*
 * Para los dispositivos compatibles puede habilitarse SRAM externa
 * descomentando:
 *
 *   #define DATA_IN_ExtSRAM
 */
#if defined(STM32F405xx) || defined(STM32F415xx) || \
    defined(STM32F407xx) || defined(STM32F417xx) || \
    defined(STM32F427xx) || defined(STM32F437xx) || \
    defined(STM32F429xx) || defined(STM32F439xx) || \
    defined(STM32F469xx) || defined(STM32F479xx) || \
    defined(STM32F412Zx) || defined(STM32F412Vx)

/* #define DATA_IN_ExtSRAM */

#endif


/*
 * Para los dispositivos compatibles puede habilitarse SDRAM externa
 * descomentando:
 *
 *   #define DATA_IN_ExtSDRAM
 */
#if defined(STM32F427xx) || defined(STM32F437xx) || \
    defined(STM32F429xx) || defined(STM32F439xx) || \
    defined(STM32F446xx) || defined(STM32F469xx) || \
    defined(STM32F479xx)

/* #define DATA_IN_ExtSDRAM */

#endif


/*
 * La tabla de vectores contiene las direcciones de:
 *
 *   - Reset_Handler.
 *   - Excepciones del Cortex-M4.
 *   - Interrupciones de periféricos.
 *
 * Normalmente permanece en el inicio de Flash.
 *
 * USER_VECT_TAB_ADDRESS permite trasladarla a otra dirección.
 */

/* #define USER_VECT_TAB_ADDRESS */


#if defined(USER_VECT_TAB_ADDRESS)

/*
 * Al definir VECT_TAB_SRAM, la tabla se coloca en SRAM.
 * De lo contrario, se mantiene en Flash con un desplazamiento configurable.
 */

/* #define VECT_TAB_SRAM */


#if defined(VECT_TAB_SRAM)

/*
 * Dirección base de la tabla de vectores cuando se ubica en SRAM.
 *
 * La dirección debe estar alineada según los requisitos de VTOR.
 */
#define VECT_TAB_BASE_ADDRESS SRAM_BASE

#else

/*
 * Dirección base de la tabla de vectores cuando se ubica en Flash.
 */
#define VECT_TAB_BASE_ADDRESS FLASH_BASE

#endif /* VECT_TAB_SRAM */


#if !defined(VECT_TAB_OFFSET)

/*
 * Desplazamiento adicional dentro de Flash o SRAM.
 *
 * Debe ser múltiplo de 0x200 para esta familia.
 */
#define VECT_TAB_OFFSET 0x00000000U

#endif /* VECT_TAB_OFFSET */

#endif /* USER_VECT_TAB_ADDRESS */


/******************************************************************************/

/**
  * @}
  */


/**
  * @addtogroup STM32F4xx_System_Private_Macros
  * @{
  */

/*
 * No se definen macros privadas adicionales.
 */

/**
  * @}
  */


/**
  * @addtogroup STM32F4xx_System_Private_Variables
  * @{
  */


/*
 * Frecuencia actual del núcleo y bus AHB, expresada en Hz.
 *
 * Su valor inicial es 16 MHz porque después del reset el STM32 comienza
 * normalmente utilizando HSI.
 *
 * En este proyecto, HAL_RCC_ClockConfig() actualiza posteriormente esta
 * variable cuando SystemClock_Config() selecciona el PLL.
 */
uint32_t SystemCoreClock = 16000000;


/*
 * Tabla utilizada para interpretar el campo HPRE del registro RCC->CFGR.
 *
 * Los valores almacenados no son directamente los divisores, sino la
 * cantidad de desplazamientos binarios necesarios:
 *
 *   valor 0 -> dividir entre 1.
 *   valor 1 -> dividir entre 2.
 *   valor 2 -> dividir entre 4.
 *   valor 3 -> dividir entre 8.
 *   ...
 *
 * La operación final se realiza mediante:
 *
 *   SystemCoreClock >>= valor;
 */
const uint8_t AHBPrescTable[16] =
{
  0, 0, 0, 0,
  0, 0, 0, 0,
  1, 2, 3, 4,
  6, 7, 8, 9
};


/*
 * Tabla equivalente para los divisores de los buses APB.
 *
 * Aunque no se utiliza directamente en SystemCoreClockUpdate(), puede ser
 * utilizada por otras partes de CMSIS o HAL.
 */
const uint8_t APBPrescTable[8] =
{
  0, 0, 0, 0,
  1, 2, 3, 4
};


/**
  * @}
  */


/**
  * @addtogroup STM32F4xx_System_Private_FunctionPrototypes
  * @{
  */


/*
 * La función de memoria externa solamente se declara cuando alguna de las
 * opciones correspondientes está habilitada.
 */
#if defined(DATA_IN_ExtSRAM) || defined(DATA_IN_ExtSDRAM)

static void SystemInit_ExtMemCtl(void);

#endif


/**
  * @}
  */


/**
  * @addtogroup STM32F4xx_System_Private_Functions
  * @{
  */


/**
  * @brief Realiza la configuración inicial del núcleo.
  *
  * Esta función es llamada desde startup_stm32f4xx.s después del reset
  * y antes de:
  *
  *   - Inicializar las variables de la aplicación.
  *   - Ejecutar HAL_Init().
  *   - Entrar en main().
  *
  * Configura:
  *
  *   - Acceso a la FPU.
  *   - Memoria externa, cuando está habilitada.
  *   - Dirección de la tabla de vectores, cuando se solicita reubicación.
  *
  * @retval None
  */
void SystemInit(void)
{
  /* ------------------------------------------------------------------------ */
  /* Configuración de la unidad de punto flotante                             */
  /* ------------------------------------------------------------------------ */

  /*
   * __FPU_PRESENT indica que el microcontrolador incorpora FPU.
   *
   * __FPU_USED indica que el proyecto fue compilado para utilizarla.
   */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)

  /*
   * Habilitar acceso completo a los coprocesadores CP10 y CP11.
   *
   * La FPU del Cortex-M4 utiliza esos dos coprocesadores.
   *
   * CPACR:
   *
   *   bits 20-21 -> CP10.
   *   bits 22-23 -> CP11.
   *
   * El valor binario 11 concede acceso completo.
   */
  SCB->CPACR |=
      ((3UL << (10U * 2U)) |
       (3UL << (11U * 2U)));

#endif


  /* ------------------------------------------------------------------------ */
  /* Configuración opcional de memoria externa                                */
  /* ------------------------------------------------------------------------ */

#if defined(DATA_IN_ExtSRAM) || defined(DATA_IN_ExtSDRAM)

  /*
   * Configurar GPIO y controlador FSMC/FMC antes de que el código utilice
   * la memoria externa.
   */
  SystemInit_ExtMemCtl();

#endif


  /* ------------------------------------------------------------------------ */
  /* Configuración de la tabla de vectores                                    */
  /* ------------------------------------------------------------------------ */

#if defined(USER_VECT_TAB_ADDRESS)

  /*
   * VTOR contiene la dirección base de la tabla de vectores.
   *
   * La operación OR combina la dirección base con el desplazamiento.
   */
  SCB->VTOR =
      VECT_TAB_BASE_ADDRESS |
      VECT_TAB_OFFSET;

#endif
}


/**
  * @brief Actualiza SystemCoreClock leyendo la configuración RCC.
  *
  * El valor calculado corresponde a HCLK, es decir, la frecuencia después
  * de aplicar el divisor AHB.
  *
  * La función:
  *
  *   1. Determina la fuente de SYSCLK.
  *   2. Calcula la salida del PLL cuando corresponda.
  *   3. Aplica el divisor AHB.
  *
  * No mide físicamente la señal de reloj. El resultado depende de que
  * HSE_VALUE y HSI_VALUE representen las frecuencias reales.
  *
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  /*
   * tmp:
   * variable temporal para campos de registros y divisores.
   *
   * pllvco:
   * frecuencia interna del VCO del PLL.
   *
   * pllp:
   * divisor de salida principal del PLL.
   *
   * pllsource:
   * fuente del PLL: HSI o HSE.
   *
   * pllm:
   * divisor de entrada del PLL.
   */
  uint32_t tmp;
  uint32_t pllvco;
  uint32_t pllp;
  uint32_t pllsource;
  uint32_t pllm;


  /* ------------------------------------------------------------------------ */
  /* Determinar la fuente actual de SYSCLK                                    */
  /* ------------------------------------------------------------------------ */

  /*
   * SWS, System Clock Switch Status, indica cuál fuente está alimentando
   * realmente al sistema:
   *
   *   00 -> HSI.
   *   01 -> HSE.
   *   10 -> PLL.
   */
  tmp = RCC->CFGR & RCC_CFGR_SWS;


  switch (tmp)
  {
    case 0x00:
      /*
       * HSI utilizado directamente como SYSCLK.
       */
      SystemCoreClock = HSI_VALUE;
      break;


    case 0x04:
      /*
       * HSE utilizado directamente como SYSCLK.
       */
      SystemCoreClock = HSE_VALUE;
      break;


    case 0x08:
      /*
       * PLL utilizado como SYSCLK.
       *
       * Fórmulas:
       *
       *   PLL_VCO =
       *       (fuente_PLL / PLL_M) × PLL_N
       *
       *   SYSCLK =
       *       PLL_VCO / PLL_P
       */

      /*
       * Leer el bit PLLSRC.
       *
       *   0 -> HSI.
       *   1 -> HSE.
       */
      pllsource =
          (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;


      /*
       * Leer el divisor PLLM.
       */
      pllm =
          RCC->PLLCFGR & RCC_PLLCFGR_PLLM;


      if (pllsource != 0U)
      {
        /*
         * HSE alimenta al PLL.
         */
        pllvco =
            (HSE_VALUE / pllm) *
            ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      }
      else
      {
        /*
         * HSI alimenta al PLL.
         */
        pllvco =
            (HSI_VALUE / pllm) *
            ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      }


      /*
       * El campo PLLP codifica los divisores así:
       *
       *   00 -> 2.
       *   01 -> 4.
       *   10 -> 6.
       *   11 -> 8.
       *
       * La expresión convierte esa codificación al divisor real.
       */
      pllp =
          (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >> 16) + 1U) * 2U;


      /*
       * Calcular SYSCLK antes del divisor AHB.
       */
      SystemCoreClock =
          pllvco / pllp;

      break;


    default:
      /*
       * Estado no reconocido.
       *
       * Se utiliza HSI como valor seguro por defecto.
       */
      SystemCoreClock = HSI_VALUE;
      break;
  }


  /* ------------------------------------------------------------------------ */
  /* Aplicar el divisor del bus AHB                                           */
  /* ------------------------------------------------------------------------ */

  /*
   * Extraer el campo HPRE de RCC->CFGR y utilizarlo como índice
   * en AHBPrescTable.
   */
  tmp =
      AHBPrescTable[
          (RCC->CFGR & RCC_CFGR_HPRE) >> 4
      ];


  /*
   * Aplicar el divisor mediante desplazamiento a la derecha.
   *
   * Ejemplos:
   *
   *   tmp = 0 -> dividir entre 1.
   *   tmp = 1 -> dividir entre 2.
   *   tmp = 2 -> dividir entre 4.
   */
  SystemCoreClock >>= tmp;
}


/* ========================================================================== */
/* Inicialización opcional de SRAM y SDRAM externa                            */
/* ========================================================================== */

/*
 * Todo el código siguiente solamente se compila cuando se habilita memoria
 * externa mediante DATA_IN_ExtSRAM o DATA_IN_ExtSDRAM.
 *
 * En el proyecto FloatSat actual ambas macros están comentadas.
 */


#if defined(DATA_IN_ExtSRAM) && defined(DATA_IN_ExtSDRAM)


#if defined(STM32F427xx) || defined(STM32F437xx) || \
    defined(STM32F429xx) || defined(STM32F439xx) || \
    defined(STM32F469xx) || defined(STM32F479xx)


/**
  * @brief Configura simultáneamente SRAM y SDRAM externas.
  *
  * Esta variante solamente se compila para dispositivos con FMC compatible
  * con ambas memorias.
  *
  * Utiliza registros directamente porque se ejecuta antes de main() y antes
  * de la inicialización normal de HAL.
  *
  * @retval None
  */
void SystemInit_ExtMemCtl(void)
{
  /*
   * tmp se utiliza para forzar una lectura después de habilitar relojes.
   * Esa lectura introduce el retardo necesario antes de acceder al periférico.
   */
  __IO uint32_t tmp = 0x00;


  /*
   * tmpreg:
   * variable temporal para registros FMC.
   *
   * timeout:
   * evita permanecer indefinidamente esperando al controlador SDRAM.
   *
   * index:
   * utilizado para un retardo simple.
   */
  register uint32_t tmpreg = 0U;
  register uint32_t timeout = 0xFFFFU;
  register __IO uint32_t index;


  /*
   * Habilitar GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH y GPIOI.
   */
  RCC->AHB1ENR |= 0x000001F8U;


  /*
   * Lectura posterior a la habilitación del reloj.
   */
  tmp =
      READ_BIT(RCC->AHB1ENR,
               RCC_AHB1ENR_GPIOCEN);


  /* ------------------------------------------------------------------------ */
  /* Configuración de los GPIO asociados al FMC                               */
  /* ------------------------------------------------------------------------ */

  /*
   * Los valores hexadecimales configuran directamente:
   *
   *   - Función alternativa FMC.
   *   - Modo Alternate Function.
   *   - Velocidad de salida.
   *   - Tipo push-pull.
   *   - Sin resistencias internas.
   *
   * Son valores específicos del encapsulado y del hardware de referencia.
   */


  /* Pines de GPIOD conectados al FMC. */
  GPIOD->AFR[0]  = 0x00CCC0CCU;
  GPIOD->AFR[1]  = 0xCCCCCCCCU;
  GPIOD->MODER   = 0xAAAA0A8AU;
  GPIOD->OSPEEDR = 0xFFFF0FCFU;
  GPIOD->OTYPER  = 0x00000000U;
  GPIOD->PUPDR   = 0x00000000U;


  /* Pines de GPIOE conectados al FMC. */
  GPIOE->AFR[0]  = 0xC00CC0CCU;
  GPIOE->AFR[1]  = 0xCCCCCCCCU;
  GPIOE->MODER   = 0xAAAA828AU;
  GPIOE->OSPEEDR = 0xFFFFC3CFU;
  GPIOE->OTYPER  = 0x00000000U;
  GPIOE->PUPDR   = 0x00000000U;


  /* Pines de GPIOF conectados al FMC. */
  GPIOF->AFR[0]  = 0xCCCCCCCCU;
  GPIOF->AFR[1]  = 0xCCCCCCCCU;
  GPIOF->MODER   = 0xAA800AAAU;
  GPIOF->OSPEEDR = 0xAA800AAAU;
  GPIOF->OTYPER  = 0x00000000U;
  GPIOF->PUPDR   = 0x00000000U;


  /* Pines de GPIOG conectados al FMC. */
  GPIOG->AFR[0]  = 0xCCCCCCCCU;
  GPIOG->AFR[1]  = 0xCCCCCCCCU;
  GPIOG->MODER   = 0xAAAAAAAAU;
  GPIOG->OSPEEDR = 0xAAAAAAAAU;
  GPIOG->OTYPER  = 0x00000000U;
  GPIOG->PUPDR   = 0x00000000U;


  /* Pines de GPIOH conectados al FMC. */
  GPIOH->AFR[0]  = 0x00C0CC00U;
  GPIOH->AFR[1]  = 0xCCCCCCCCU;
  GPIOH->MODER   = 0xAAAA08A0U;
  GPIOH->OSPEEDR = 0xAAAA08A0U;
  GPIOH->OTYPER  = 0x00000000U;
  GPIOH->PUPDR   = 0x00000000U;


  /* Pines de GPIOI conectados al FMC. */
  GPIOI->AFR[0]  = 0xCCCCCCCCU;
  GPIOI->AFR[1]  = 0x00000CC0U;
  GPIOI->MODER   = 0x0028AAAAU;
  GPIOI->OSPEEDR = 0x0028AAAAU;
  GPIOI->OTYPER  = 0x00000000U;
  GPIOI->PUPDR   = 0x00000000U;


  /* ------------------------------------------------------------------------ */
  /* Configuración del controlador FMC                                        */
  /* ------------------------------------------------------------------------ */

  /*
   * Habilitar el reloj del FMC.
   */
  RCC->AHB3ENR |= 0x00000001U;


  /*
   * Lectura para asegurar que el reloj ya fue aplicado.
   */
  tmp =
      READ_BIT(RCC->AHB3ENR,
               RCC_AHB3ENR_FMCEN);


  /*
   * Configurar parámetros de control y temporización de SDRAM.
   *
   * Los valores contienen opciones como:
   *
   *   - Número de columnas y filas.
   *   - Ancho del bus.
   *   - Número de bancos.
   *   - Latencia CAS.
   *   - Temporizaciones TRCD, TRP, TRAS, etc.
   */
  FMC_Bank5_6->SDCR[0] = 0x000019E4U;
  FMC_Bank5_6->SDTR[0] = 0x01115351U;


  /* ------------------------------------------------------------------------ */
  /* Secuencia de inicialización de la SDRAM                                  */
  /* ------------------------------------------------------------------------ */

  /*
   * Habilitar el reloj de la SDRAM.
   */
  FMC_Bank5_6->SDCMR = 0x00000011U;


  /*
   * Esperar mientras el controlador esté ocupado.
   */
  tmpreg =
      FMC_Bank5_6->SDSR & 0x00000020U;

  while ((tmpreg != 0U) &&
         (timeout-- > 0U))
  {
    tmpreg =
        FMC_Bank5_6->SDSR & 0x00000020U;
  }


  /*
   * Retardo requerido después de habilitar el reloj SDRAM.
   */
  for (index = 0U;
       index < 1000U;
       index++)
  {
    /* Retardo intencional. */
  }


  /*
   * Comando PALL: precharge all.
   */
  FMC_Bank5_6->SDCMR = 0x00000012U;

  tmpreg =
      FMC_Bank5_6->SDSR & 0x00000020U;

  timeout = 0xFFFFU;

  while ((tmpreg != 0U) &&
         (timeout-- > 0U))
  {
    tmpreg =
        FMC_Bank5_6->SDSR & 0x00000020U;
  }


  /*
   * Comando de auto-refresh.
   */
  FMC_Bank5_6->SDCMR = 0x00000073U;

  tmpreg =
      FMC_Bank5_6->SDSR & 0x00000020U;

  timeout = 0xFFFFU;

  while ((tmpreg != 0U) &&
         (timeout-- > 0U))
  {
    tmpreg =
        FMC_Bank5_6->SDSR & 0x00000020U;
  }


  /*
   * Programar el Mode Register de la SDRAM.
   */
  FMC_Bank5_6->SDCMR = 0x00046014U;

  tmpreg =
      FMC_Bank5_6->SDSR & 0x00000020U;

  timeout = 0xFFFFU;

  while ((tmpreg != 0U) &&
         (timeout-- > 0U))
  {
    tmpreg =
        FMC_Bank5_6->SDSR & 0x00000020U;
  }


  /*
   * Configurar el periodo de refresco.
   */
  tmpreg =
      FMC_Bank5_6->SDRTR;

  FMC_Bank5_6->SDRTR =
      tmpreg |
      (0x0000027CU << 1);


  /*
   * Deshabilitar protección de escritura de SDRAM.
   */
  tmpreg =
      FMC_Bank5_6->SDCR[0];

  FMC_Bank5_6->SDCR[0] =
      tmpreg & 0xFFFFFDFFU;


#if defined(STM32F427xx) || defined(STM32F437xx) || \
    defined(STM32F429xx) || defined(STM32F439xx)

  /*
   * Configurar Bank1 SRAM2 para estas variantes.
   */
  FMC_Bank1->BTCR[2]  = 0x00001011U;
  FMC_Bank1->BTCR[3]  = 0x00000201U;
  FMC_Bank1E->BWTR[2] = 0x0FFFFFFFU;

#endif


#if defined(STM32F469xx) || defined(STM32F479xx)

  /*
   * Configuración alternativa de Bank1 SRAM2.
   */
  FMC_Bank1->BTCR[2]  = 0x00001091U;
  FMC_Bank1->BTCR[3]  = 0x00110212U;
  FMC_Bank1E->BWTR[2] = 0x0FFFFFFFU;

#endif


  /*
   * Evitar advertencia por variable asignada pero no utilizada.
   */
  (void)tmp;
}

#endif


#elif defined(DATA_IN_ExtSRAM) || defined(DATA_IN_ExtSDRAM)


/**
  * @brief Configura SRAM o SDRAM externa.
  *
  * Esta versión se compila cuando solamente una de las dos memorias
  * externas está habilitada.
  *
  * @retval None
  */
void SystemInit_ExtMemCtl(void)
{
  /*
   * Variable utilizada para introducir la lectura posterior a la
   * habilitación de relojes.
   */
  __IO uint32_t tmp = 0x00U;


  /* ======================================================================== */
  /* Configuración de SDRAM externa                                           */
  /* ======================================================================== */

#if defined(STM32F427xx) || defined(STM32F437xx) || \
    defined(STM32F429xx) || defined(STM32F439xx) || \
    defined(STM32F446xx) || defined(STM32F469xx) || \
    defined(STM32F479xx)


#if defined(DATA_IN_ExtSDRAM)

  register uint32_t tmpreg = 0U;
  register uint32_t timeout = 0xFFFFU;
  register __IO uint32_t index;


#if defined(STM32F446xx)

  /*
   * El STM32F446 utiliza una distribución de GPIO diferente.
   *
   * Habilitar GPIOA, GPIOC, GPIOD, GPIOE, GPIOF y GPIOG.
   */
  RCC->AHB1ENR |= 0x0000007DU;

#else

  /*
   * Habilitar GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH y GPIOI.
   */
  RCC->AHB1ENR |= 0x000001F8U;

#endif


  /*
   * Lectura posterior a la habilitación de los relojes GPIO.
   */
  tmp =
      READ_BIT(RCC->AHB1ENR,
               RCC_AHB1ENR_GPIOCEN);


#if defined(STM32F446xx)

  /*
   * Configuración adicional de GPIOA para FMC.
   */
  GPIOA->AFR[0]  |= 0xC0000000U;
  GPIOA->AFR[1]  |= 0x00000000U;
  GPIOA->MODER   |= 0x00008000U;
  GPIOA->OSPEEDR |= 0x00008000U;
  GPIOA->OTYPER  |= 0x00000000U;
  GPIOA->PUPDR   |= 0x00000000U;


  /*
   * Configuración adicional de GPIOC para FMC.
   */
  GPIOC->AFR[0]  |= 0x00CC0000U;
  GPIOC->AFR[1]  |= 0x00000000U;
  GPIOC->MODER   |= 0x00000A00U;
  GPIOC->OSPEEDR |= 0x00000A00U;
  GPIOC->OTYPER  |= 0x00000000U;
  GPIOC->PUPDR   |= 0x00000000U;

#endif


  /*
   * Configuración de GPIOD para señales FMC.
   */
  GPIOD->AFR[0]  = 0x000000CCU;
  GPIOD->AFR[1]  = 0xCC000CCCU;
  GPIOD->MODER   = 0xA02A000AU;
  GPIOD->OSPEEDR = 0xA02A000AU;
  GPIOD->OTYPER  = 0x00000000U;
  GPIOD->PUPDR   = 0x00000000U;


  /*
   * Configuración de GPIOE para señales FMC.
   */
  GPIOE->AFR[0]  = 0xC00000CCU;
  GPIOE->AFR[1]  = 0xCCCCCCCCU;
  GPIOE->MODER   = 0xAAAA800AU;
  GPIOE->OSPEEDR = 0xAAAA800AU;
  GPIOE->OTYPER  = 0x00000000U;
  GPIOE->PUPDR   = 0x00000000U;


  /*
   * Configuración de GPIOF para señales FMC.
   */
  GPIOF->AFR[0]  = 0xCCCCCCCCU;
  GPIOF->AFR[1]  = 0xCCCCCCCCU;
  GPIOF->MODER   = 0xAA800AAAU;
  GPIOF->OSPEEDR = 0xAA800AAAU;
  GPIOF->OTYPER  = 0x00000000U;
  GPIOF->PUPDR   = 0x00000000U;


  /*
   * Configuración de GPIOG para señales FMC.
   */
  GPIOG->AFR[0]  = 0xCCCCCCCCU;
  GPIOG->AFR[1]  = 0xCCCCCCCCU;
  GPIOG->MODER   = 0xAAAAAAAAU;
  GPIOG->OSPEEDR = 0xAAAAAAAAU;
  GPIOG->OTYPER  = 0x00000000U;
  GPIOG->PUPDR   = 0x00000000U;


#if defined(STM32F427xx) || defined(STM32F437xx) || \
    defined(STM32F429xx) || defined(STM32F439xx) || \
    defined(STM32F469xx) || defined(STM32F479xx)

  /*
   * Configuración de GPIOH para señales FMC.
   */
  GPIOH->AFR[0]  = 0x00C0CC00U;
  GPIOH->AFR[1]  = 0xCCCCCCCCU;
  GPIOH->MODER   = 0xAAAA08A0U;
  GPIOH->OSPEEDR = 0xAAAA08A0U;
  GPIOH->OTYPER  = 0x00000000U;
  GPIOH->PUPDR   = 0x00000000U;


  /*
   * Configuración de GPIOI para señales FMC.
   */
  GPIOI->AFR[0]  = 0xCCCCCCCCU;
  GPIOI->AFR[1]  = 0x00000CC0U;
  GPIOI->MODER   = 0x0028AAAAU;
  GPIOI->OSPEEDR = 0x0028AAAAU;
  GPIOI->OTYPER  = 0x00000000U;
  GPIOI->PUPDR   = 0x00000000U;

#endif


  /* ------------------------------------------------------------------------ */
  /* Configuración del FMC para SDRAM                                         */
  /* ------------------------------------------------------------------------ */

  /*
   * Habilitar reloj del controlador FMC.
   */
  RCC->AHB3ENR |= 0x00000001U;


  /*
   * Lectura posterior a la habilitación del reloj.
   */
  tmp =
      READ_BIT(RCC->AHB3ENR,
               RCC_AHB3ENR_FMCEN);


  /*
   * Configurar SDRAM Bank 1.
   */
#if defined(STM32F446xx)

  FMC_Bank5_6->SDCR[0] = 0x00001954U;

#else

  FMC_Bank5_6->SDCR[0] = 0x000019E4U;

#endif


  /*
   * Temporización de la SDRAM.
   */
  FMC_Bank5_6->SDTR[0] = 0x01115351U;


  /*
   * Comando para habilitar el reloj SDRAM.
   */
  FMC_Bank5_6->SDCMR = 0x00000011U;

  tmpreg =
      FMC_Bank5_6->SDSR & 0x00000020U;

  while ((tmpreg != 0U) &&
         (timeout-- > 0U))
  {
    tmpreg =
        FMC_Bank5_6->SDSR & 0x00000020U;
  }


  /*
   * Retardo inicial requerido por la memoria.
   */
  for (index = 0U;
       index < 1000U;
       index++)
  {
    /* Retardo intencional. */
  }


  /*
   * Comando Precharge All.
   */
  FMC_Bank5_6->SDCMR = 0x00000012U;

  tmpreg =
      FMC_Bank5_6->SDSR & 0x00000020U;

  timeout = 0xFFFFU;

  while ((tmpreg != 0U) &&
         (timeout-- > 0U))
  {
    tmpreg =
        FMC_Bank5_6->SDSR & 0x00000020U;
  }


  /*
   * Comando de auto-refresh.
   */
#if defined(STM32F446xx)

  FMC_Bank5_6->SDCMR = 0x000000F3U;

#else

  FMC_Bank5_6->SDCMR = 0x00000073U;

#endif


  tmpreg =
      FMC_Bank5_6->SDSR & 0x00000020U;

  timeout = 0xFFFFU;

  while ((tmpreg != 0U) &&
         (timeout-- > 0U))
  {
    tmpreg =
        FMC_Bank5_6->SDSR & 0x00000020U;
  }


  /*
   * Programar el Mode Register.
   */
#if defined(STM32F446xx)

  FMC_Bank5_6->SDCMR = 0x00044014U;

#else

  FMC_Bank5_6->SDCMR = 0x00046014U;

#endif


  tmpreg =
      FMC_Bank5_6->SDSR & 0x00000020U;

  timeout = 0xFFFFU;

  while ((tmpreg != 0U) &&
         (timeout-- > 0U))
  {
    tmpreg =
        FMC_Bank5_6->SDSR & 0x00000020U;
  }


  /*
   * Configurar contador de refresco.
   */
  tmpreg =
      FMC_Bank5_6->SDRTR;


#if defined(STM32F446xx)

  FMC_Bank5_6->SDRTR =
      tmpreg |
      (0x0000050CU << 1);

#else

  FMC_Bank5_6->SDRTR =
      tmpreg |
      (0x0000027CU << 1);

#endif


  /*
   * Deshabilitar la protección de escritura.
   */
  tmpreg =
      FMC_Bank5_6->SDCR[0];

  FMC_Bank5_6->SDCR[0] =
      tmpreg & 0xFFFFFDFFU;


#endif /* DATA_IN_ExtSDRAM */

#endif /* Dispositivos con SDRAM FMC */


  /* ======================================================================== */
  /* Configuración de SRAM externa                                            */
  /* ======================================================================== */

#if defined(STM32F405xx) || defined(STM32F415xx) || \
    defined(STM32F407xx) || defined(STM32F417xx) || \
    defined(STM32F427xx) || defined(STM32F437xx) || \
    defined(STM32F429xx) || defined(STM32F439xx) || \
    defined(STM32F469xx) || defined(STM32F479xx) || \
    defined(STM32F412Zx) || defined(STM32F412Vx)


#if defined(DATA_IN_ExtSRAM)

  /*
   * Habilitar GPIOD, GPIOE, GPIOF y GPIOG.
   */
  RCC->AHB1ENR |= 0x00000078U;


  /*
   * Lectura posterior a la habilitación de reloj.
   */
  tmp =
      READ_BIT(RCC->AHB1ENR,
               RCC_AHB1ENR_GPIODEN);


  /*
   * Configuración de GPIOD para FSMC/FMC.
   */
  GPIOD->AFR[0]  = 0x00CCC0CCU;
  GPIOD->AFR[1]  = 0xCCCCCCCCU;
  GPIOD->MODER   = 0xAAAA0A8AU;
  GPIOD->OSPEEDR = 0xFFFF0FCFU;
  GPIOD->OTYPER  = 0x00000000U;
  GPIOD->PUPDR   = 0x00000000U;


  /*
   * Configuración de GPIOE para FSMC/FMC.
   */
  GPIOE->AFR[0]  = 0xC00CC0CCU;
  GPIOE->AFR[1]  = 0xCCCCCCCCU;
  GPIOE->MODER   = 0xAAAA828AU;
  GPIOE->OSPEEDR = 0xFFFFC3CFU;
  GPIOE->OTYPER  = 0x00000000U;
  GPIOE->PUPDR   = 0x00000000U;


  /*
   * Configuración de GPIOF para FSMC/FMC.
   */
  GPIOF->AFR[0]  = 0x00CCCCCCU;
  GPIOF->AFR[1]  = 0xCCCC0000U;
  GPIOF->MODER   = 0xAA000AAAU;
  GPIOF->OSPEEDR = 0xFF000FFFU;
  GPIOF->OTYPER  = 0x00000000U;
  GPIOF->PUPDR   = 0x00000000U;


  /*
   * Configuración de GPIOG para FSMC/FMC.
   */
  GPIOG->AFR[0]  = 0x00CCCCCCU;
  GPIOG->AFR[1]  = 0x000000C0U;
  GPIOG->MODER   = 0x00085AAAU;
  GPIOG->OSPEEDR = 0x000CAFFFU;
  GPIOG->OTYPER  = 0x00000000U;
  GPIOG->PUPDR   = 0x00000000U;


  /*
   * Habilitar el reloj de FMC o FSMC.
   *
   * La posición del bit es compatible entre estas variantes.
   */
  RCC->AHB3ENR |= 0x00000001U;


#if defined(STM32F427xx) || defined(STM32F437xx) || \
    defined(STM32F429xx) || defined(STM32F439xx)

  /*
   * Esperar que el reloj FMC quede aplicado.
   */
  tmp =
      READ_BIT(RCC->AHB3ENR,
               RCC_AHB3ENR_FMCEN);


  /*
   * Configurar Bank1 SRAM2.
   */
  FMC_Bank1->BTCR[2]  = 0x00001011U;
  FMC_Bank1->BTCR[3]  = 0x00000201U;
  FMC_Bank1E->BWTR[2] = 0x0FFFFFFFU;

#endif


#if defined(STM32F469xx) || defined(STM32F479xx)

  tmp =
      READ_BIT(RCC->AHB3ENR,
               RCC_AHB3ENR_FMCEN);

  FMC_Bank1->BTCR[2]  = 0x00001091U;
  FMC_Bank1->BTCR[3]  = 0x00110212U;
  FMC_Bank1E->BWTR[2] = 0x0FFFFFFFU;

#endif


#if defined(STM32F405xx) || defined(STM32F415xx) || \
    defined(STM32F407xx) || defined(STM32F417xx) || \
    defined(STM32F412Zx) || defined(STM32F412Vx)

  /*
   * El STM32F407 utiliza FSMC en lugar del nombre FMC utilizado
   * por variantes posteriores.
   */
  tmp =
      READ_BIT(RCC->AHB3ENR,
               RCC_AHB3ENR_FSMCEN);


  /*
   * Configurar Bank1 SRAM2 del FSMC.
   */
  FSMC_Bank1->BTCR[2]  = 0x00001011U;
  FSMC_Bank1->BTCR[3]  = 0x00000201U;
  FSMC_Bank1E->BWTR[2] = 0x0FFFFFFFU;

#endif


#endif /* DATA_IN_ExtSRAM */

#endif /* Dispositivos compatibles con SRAM */


  /*
   * Evitar advertencia de variable no utilizada.
   */
  (void)tmp;
}

#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */


/**
  * @}
  */


/**
  * @}
  */


/**
  * @}
  */
