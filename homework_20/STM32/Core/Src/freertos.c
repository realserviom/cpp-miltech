/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uart_link.h"

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
struct For_stm currentFor_stm;
SemaphoreHandle_t uartMtx;

// включаємо чи виключаємо відправку
volatile bool txEnabled = true;

/* USER CODE END Variables */

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

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
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
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

// void commsTask(void*) {
//     Sample s;
//     char line[48];
//     for (;;) {
//         xQueueReceive(q, &s, portMAX_DELAY);
//         if (!txEnabled) continue;
//         const int32_t mv = int32_t(s.raw) * 125 / 1000;
//         const int n = snprintf(line, sizeof line,
//                             "t=%lu ms  raw=%d  U=%ld mV\r\n",
//                             (unsigned long)s.t_ms, s.raw, (long)mv);
//         uartSend(line, n);
//     }
// }

void buttonTask(void*) {
    TickType_t last = 0;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        const TickType_t now = xTaskGetTickCount();
        if (now - last < pdMS_TO_TICKS(50)) continue;
        last = now;
        txEnabled = !txEnabled;
        const char* msg = txEnabled ? "tx: ON\r\n" : "tx: OFF\r\n";
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        // Виводимо статус через printf (який піде в USB CDC)
        if (txEnabled) {
            printf("tx: ON\r\n");
        } else {
            printf("tx: OFF\r\n");
        }
        
        sendAnswer(msg);
    }
}


void heartbeatTask(void* pvParameters) {
    TickType_t wake = xTaskGetTickCount();
    uint32_t n = 0;
    char line[40];
    
    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(1000));
        
        snprintf(line, sizeof(line), "hb #%lu t=%lu ms",
                 (unsigned long)++n,
                 (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
                 
        // Відправляємо через запакований протокол
        sendAnswer(line); 
    }
}

void uartSend(const uint8_t* data, size_t len) {
    if (xSemaphoreTake(uartMtx, pdMS_TO_TICKS(100)) == pdTRUE) {
        HAL_UART_Transmit(&huart1, (uint8_t*)data, len, 100);
        xSemaphoreGive(uartMtx);
    }
}


void uartReceiveTask(void* pvParameters)
{
    uart_parser_t parser;
    parser_init(&parser);

    uint8_t byte;
    uint8_t outType;
    uint8_t outPayload[260];
    uint8_t outLen;

    while (1) {
        // Читаємо 1 байт. Таймаут 10 мс дозволяє FreeRTOS перемикати контекст.
        if (HAL_UART_Receive(&huart1, &byte, 1, 10) == HAL_OK) {
            
            if (parser_feed(&parser, byte, &outType, outPayload, &outLen)) {
                
                if (outType == PKT_FOR_STM && outLen == sizeof(struct For_stm)) {
                    memcpy(&currentFor_stm, outPayload, sizeof(struct For_stm));
                    printf("GET PKT_FOR_STM!\n"); // Переконайтеся, що printf перевизначено через _write
                }
                else {
                    printf("NO NAME! outType: %d\n", outType);
                }
            }
        } else {
            // Якщо байтів у буфері немає, віддаємо ресурс іншим таскам
            vTaskDelay(pdMS_TO_TICKS(1)); 
        }
    }
}

/* USER CODE END Application */

