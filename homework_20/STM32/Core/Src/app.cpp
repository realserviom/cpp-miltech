#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <cstdio>
#include <cstring>
#include "main.h"

// extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

namespace {

    // constexpr uint16_t kAdsAddr = 0x48 << 1;
    // constexpr uint8_t  kRegConv = 0x00;
    // constexpr uint8_t  kRegCfg  = 0x01;


    // Значення 0xC383	означає для датчика наступне ADS1115 
    // [15] OS	1	Start Single-Conversion: Наказ «Зроби вимірювання зараз!»
    // ** MUX**	100	Канал AIN0: Вимірювати напругу між піном AIN0 та GND
    // [11:9] PGA	001	Шкала ±4.096V: Максимальна напруга, яку може зчитати АЦП
    // [8] MODE	1	Single-shot mode: Зробити 1 вимірювання й заснути (для економії)
    // [7:5] DR	100	128 SPS: Швидкість оцифровки (128 вибірок на секунду)
    // [4:0] COMP	00011	Disable comparator: Вимкнути апаратний компаратор тривоги

    // constexpr uint16_t kCfgSingleA0 = 0xC383;

    // bool adsReadOnce(int16_t& raw) {
    //     uint8_t cfg[2] = { uint8_t(kCfgSingleA0 >> 8), uint8_t(kCfgSingleA0) };
        
    //     // тут ми записали що нам треба взяти і яким чином
    //     if (HAL_I2C_Mem_Write(&hi2c1, kAdsAddr, kRegCfg, I2C_MEMADD_SIZE_8BIT,
    //                         cfg, 2, 100) != HAL_OK) return false;

    //     // тут робимо затримку в 10 мс перетворюючи на тіки                    
    //     vTaskDelay(pdMS_TO_TICKS(10));

    //     // створюємо масив щоб записати параметри
    //     uint8_t d[2];

    //     // читаємо параметр в масив d
    //     if (HAL_I2C_Mem_Read(&hi2c1, kAdsAddr, kRegConv, I2C_MEMADD_SIZE_8BIT,
    //                         d, 2, 100) != HAL_OK) return false;

    //     // записуємо в raw регістри в одне 16 бітне число                  
    //     raw = int16_t((d[0] << 8) | d[1]);
    //     return true;
    // }

    struct Sample { uint32_t t_ms; int16_t raw; };

    //QueueHandle_t q;
    TaskHandle_t btnTaskH;
  
    // void sensorTask(void*) {
    //     TickType_t wake = xTaskGetTickCount();
    //     for (;;) {
    //         vTaskDelayUntil(&wake, pdMS_TO_TICKS(100));
    //         Sample s;
    //         s.t_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    //         if (adsReadOnce(s.raw))
    //             xQueueSend(q, &s, 0);
    //     }
    // }

  
}

// Повідомляємо C++ файлу app.cpp, що десь у C-файлах існує така глобальна змінна
extern "C" {
    extern SemaphoreHandle_t uartMtx;
}

// Якщо ви використовуєте задачі, які також лежать у freertos.c, їхні прототипи 
// теж бажано оголосити тут, щоб xTaskCreate знав, що це за функції:
#ifdef __cplusplus
extern "C" {
#endif

void buttonTask(void *argument);
void heartbeatTask(void *argument);
void uartReceiveTask(void *argument);

#ifdef __cplusplus
}
#endif

extern "C" void app_start(void) {
    // q = xQueueCreate(8, sizeof(Sample));

    printf("App started\r\n");

    uartMtx = xSemaphoreCreateMutex();

    //xTaskCreate(sensorTask, "sensor", 256, nullptr, 4, nullptr);
    //xTaskCreate(commsTask, "comms", 256, nullptr, 3, nullptr);
    //xTaskCreate(buttonTask, "button", 128, nullptr, 2, &btnTaskH);

    BaseType_t result = xTaskCreate(buttonTask, "button", 256, nullptr, 2, &btnTaskH);
    if (result != pdPASS) {
        // Помилка: недостатньо пам'яті у FreeRTOS Heap (configTOTAL_HEAP_SIZE)
        printf("ERROR 1: Failed to create buttonTask! Heap is full or stack is too large.\r\n");
        // Тут можна додати зупинку виконання або зациклення
        configASSERT(0);
    }

    if (btnTaskH == NULL) {
        // Помилка виділення пам'яті в купі (Heap)
        printf("ERROR 2: Failed to create buttonTask! Heap is full or stack is too large.\r\n");
        configASSERT(0);
    }

    xTaskCreate(heartbeatTask, "hb", 192, nullptr, 1, nullptr);
    xTaskCreate(uartReceiveTask, "uart_rx", 256, nullptr, 3, nullptr);
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_0) {
        // значення яке записується передане woken по посиланню відслідковує пріоритет задачі 
        // яку перервала кнопка
        // якщо пріоритет задачі що виконувалася вищий за задачу кнопки (наприклад 3 бо кнопка має 2 пріоритет)
        // тоді виконується задача з пріоритетом 3 і тоді знову вернемся до задачі із кнопкою
        BaseType_t woken = pdFALSE;

        vTaskNotifyGiveFromISR(btnTaskH, &woken);
        portYIELD_FROM_ISR(woken);
    }
}