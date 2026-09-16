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
#include "semphr.h" 

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
struct Data currentData;
SemaphoreHandle_t uartMtx;

// включаємо чи виключаємо відправку
volatile bool txEnabled = true;
#define FLASH_USER_START_ADDR  0x08060000

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
void LoadConfigFromFlash(struct Data *config);
void SaveConfigToFlash(const struct Data *config);
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

  // Читаємо конфігурацію з Flash при стартові
  LoadConfigFromFlash(&currentData);

  // Якщо сектор порожній (новий) — записуємо дефолтні значення
  if ((uint8_t)currentData.name[0] == 0xFF || currentData.name[0] == '\0') {
      strcpy(currentData.name, "Default_STM");
      currentData.val = 42;
      strcpy(currentData.mode, "auto");
      
      SaveConfigToFlash(&currentData);
  }


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
        if (now - last < pdMS_TO_TICKS(200)) continue;
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

void espUartSend(const uint8_t* data, size_t len) {
    if (xSemaphoreTake(uartMtx, pdMS_TO_TICKS(100)) == pdTRUE) {
        HAL_UART_Transmit(&huart2, (uint8_t*)data, len, 100);
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
        if (HAL_UART_Receive(&huart2, &byte, 1, 10) == HAL_OK) {
            if (parser_feed(&parser, byte, &outType, outPayload, &outLen)) {
                if (outType == PKT_DATA && outLen == sizeof(struct Data)) {
                    // get packet from esp32 by uart and save it in currentData
                    memcpy(&currentData, outPayload, sizeof(struct Data));

                    const char* msg = "SAVE PKT_DATA!\r\n";
                    printf("%s", msg); // Переконайтеся, що printf перевизначено через _write

                    // Відправляємо через запакований протокол
                    //sendAnswer(msg);

                    // save currentData to flash
                    SaveConfigToFlash(&currentData);

                    // --- Блимаємо діодом на пів секунди (500 мс) ---
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // Вимкнути на пів секунди
                    HAL_Delay(500);
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // Увімкнути назад
                } else if (outType == PKT_GET_DATA) {
                    printf("GET PKT_GET_DATA requested by ESP32\n");
                    sendPacketToESP32(&currentData);
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

void SaveConfigToFlash(const struct Data *config) {
    HAL_FLASH_Unlock();

    // Очищуємо сектор перед записом
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;
    
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector      = FLASH_SECTOR_7;
    EraseInitStruct.NbSectors   = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
        printf("Помилки стирання 7 сектора із даними");
        HAL_FLASH_Lock();
        return;
    }

    // Записуємо структуру по 32-біта
    uint32_t address = FLASH_USER_START_ADDR;
    uint32_t *data_ptr = (uint32_t*)config;
    // Вирівнюємо по 32 біта (4 байти)
    size_t size_in_words = (sizeof(struct Data) + 3) / 4; 

    for (size_t i = 0; i < size_in_words; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data_ptr[i]) == HAL_OK) {
            address += 4;
        } else {
            printf("Помилки запису в 7 сектор");
            break;
        }
    }

    HAL_FLASH_Lock();
}

// Функція для читання структури з Flash
void LoadConfigFromFlash(struct Data *config) {
    memcpy(config, (void*)FLASH_USER_START_ADDR, sizeof(struct Data));
}


/* USER CODE END Application */

