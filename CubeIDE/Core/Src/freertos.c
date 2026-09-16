/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Inicialización y elementos asociados a FreeRTOS.
  *
  * Este archivo contiene:
  *
  *   - Definición y creación de las tareas del sistema.
  *   - Variables globales de diagnóstico y telemetría.
  *   - Parámetros generales del FloatSat.
  *   - Funciones auxiliares para controlar el motor.
  *   - Función de recuperación del bus I2C1.
  *
  * Las secciones delimitadas por USER CODE BEGIN / END son conservadas
  * por STM32CubeMX cuando se vuelve a generar el código.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

static const floatsat_handles_t *handles;


/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Telemetry */
osThreadId_t Task_TelemetryHandle;
const osThreadAttr_t Task_Telemetry_attributes = {
  .name = "Task_Telemetry",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Telecmd */
osThreadId_t Task_TelecmdHandle;
const osThreadAttr_t Task_Telecmd_attributes = {
  .name = "Task_Telecmd",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Control */
osThreadId_t Task_ControlHandle;
const osThreadAttr_t Task_Control_attributes = {
  .name = "Task_Control",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Core */
osThreadId_t Task_CoreHandle;
const osThreadAttr_t Task_Core_attributes = {
  .name = "Task_Core",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Task_Log */
osThreadId_t Task_LogHandle;
const osThreadAttr_t Task_Log_attributes = {
  .name = "Task_Log",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
extern void Task_TelemetryFn(void *argument);
extern void Task_TelecmdFn(void *argument);
extern void Task_ControlFn(void *argument);
extern void Task_CoreFn(void *argument);
extern void Task_LogFn(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  handles = FloatSat_GetHandles();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of Task_Telemetry */
  Task_TelemetryHandle = osThreadNew(Task_TelemetryFn, (void*) handles->telemetry_handle, &Task_Telemetry_attributes);

  /* creation of Task_Telecmd */
  Task_TelecmdHandle = osThreadNew(Task_TelecmdFn, (void*) handles->cmd_manager_handle, &Task_Telecmd_attributes);

  /* creation of Task_Control */
  Task_ControlHandle = osThreadNew(Task_ControlFn, (void*) handles->control_handle, &Task_Control_attributes);

  /* creation of Task_Core */
  Task_CoreHandle = osThreadNew(Task_CoreFn, (void*) handles->core_handle, &Task_Core_attributes);

  /* creation of Task_Log */
  Task_LogHandle = osThreadNew(Task_LogFn, (void*) handles->telemetry_handle, &Task_Log_attributes);

  /* USER CODE BEGIN RTOS_THREADS */




  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

