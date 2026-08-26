#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <cstdio>
#include <cstring>
#include "main.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;

namespace {

    constexpr uint16_t kAdsAddr = 0x48 << 1;
    constexpr uint8_t  kRegConv = 0x00;
    constexpr uint8_t  kRegCfg  = 0x01;


    // Значення 0xC383	означає для датчика наступне ADS1115 
    // [15] OS	1	Start Single-Conversion: Наказ «Зроби вимірювання зараз!»
    // ** MUX**	100	Канал AIN0: Вимірювати напругу між піном AIN0 та GND
    // [11:9] PGA	001	Шкала ±4.096V: Максимальна напруга, яку може зчитати АЦП
    // [8] MODE	1	Single-shot mode: Зробити 1 вимірювання й заснути (для економії)
    // [7:5] DR	100	128 SPS: Швидкість оцифровки (128 вибірок на секунду)
    // [4:0] COMP	00011	Disable comparator: Вимкнути апаратний компаратор тривоги

    constexpr uint16_t kCfgSingleA0 = 0xC383;

    bool adsReadOnce(int16_t& raw) {
        uint8_t cfg[2] = { uint8_t(kCfgSingleA0 >> 8), uint8_t(kCfgSingleA0) };
        
        // тут ми записали що нам треба взяти і яким чином
        if (HAL_I2C_Mem_Write(&hi2c1, kAdsAddr, kRegCfg, I2C_MEMADD_SIZE_8BIT,
                            cfg, 2, 100) != HAL_OK) return false;

        // тут робимо затримку в 10 мс перетворюючи на тіки                    
        vTaskDelay(pdMS_TO_TICKS(10));

        // створюємо масив щоб записати параметри
        uint8_t d[2];

        // читаємо параметр в масив d
        if (HAL_I2C_Mem_Read(&hi2c1, kAdsAddr, kRegConv, I2C_MEMADD_SIZE_8BIT,
                            d, 2, 100) != HAL_OK) return false;

        // записуємо в raw регістри в одне 16 бітне число                  
        raw = int16_t((d[0] << 8) | d[1]);
        return true;
    }

    struct Sample { uint32_t t_ms; int16_t raw; };

    QueueHandle_t q;
    TaskHandle_t btnTaskH;
    // включаємо чи виключаємо відправку
    volatile bool txEnabled = true;

    SemaphoreHandle_t uartMtx;

    void uartSend(const char* s, size_t n) {
        if (xSemaphoreTake(uartMtx, pdMS_TO_TICKS(100)) == pdTRUE) {
            HAL_UART_Transmit(&huart1, (uint8_t*)s, n, 100);
            xSemaphoreGive(uartMtx);
        }
    }

    void heartbeatTask(void*) {
        TickType_t wake = xTaskGetTickCount();
        uint32_t n = 0;
        char line[40];
        for (;;) {
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(1000));
            const int len = snprintf(line, sizeof line, "hb #%lu t=%lu ms\r\n",
                                    (unsigned long)++n,
                                    (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
            uartSend(line, len);
        }
    }


	// ############# падіння на STM32  ##############
	// все що нижче вже прописано по файлах
	// __attribute__((section(".noinit")))
	// volatile uint32_t fault_code;

	// void HardFault_Handler(void) {
	//     fault_code = SCB->CFSR;
	//     NVIC_SystemReset();
	// }

	// на старті: чому ми перезапустились?
	// if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
	//    log_reset("watchdog", 0);
	// else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
	//    log_reset("fault", fault_code);
	//__HAL_RCC_CLEAR_RESET_FLAGS();
        //
	// #############################################


        // ############# падіння на ESP32  ##############
        // Guru Meditation — панічний звіт: яке ядро, тип помилки, ланцюжок викликів
        // idf.py monitor декодує адреси за ELF: імена функцій і рядки замість чисел
        // esp_reset_reason() — аналог прапорців STM32: журналювати на кожному старті
        // core dump у Flash (опція menuconfig): падіння в полі розбирається після повернення пристрою
	// brownout-детектор увімкнений типово: «випадкові ресети» на слабкому живленні — шукати тут

	// падіння друкує в порт розбір:
	// Guru Meditation Error: Core 1 panic'ed
	// (LoadProhibited)
	// Backtrace: 0x400d1e42:0x3ffb4f60 ...
	// idf.py monitor перетворює адреси
	// на імена функцій і рядки коду

	esp_reset_reason_t r = esp_reset_reason();
	switch (r) {
		case ESP_RST_PANIC:     // впали
		case ESP_RST_TASK_WDT:  // задача зависла
		case ESP_RST_BROWNOUT:  // просадка живлення
		case ESP_RST_DEEPSLEEP: // штатне пробудження
	    break;              // журнал за причиною
	}
	// ##############################################

	// #### сон у коді STM32 ###########

	HAL_PWR_EnterSTOPMode(
	    PWR_LOWPOWERREGULATOR_ON,
	    PWR_STOPENTRY_WFI);

	// виконання продовжується зВІДСИ,
	// але тактування впало на внутрішній
	// генератор — повернути швидкість:
	SystemClock_Config();

	// Standby: прокидання = перезапуск main
	HAL_PWR_EnterSTANDBYMode();

        // ############# ESP32 ###########

	ESP32 (deep sleep; будить таймер, пін):

	RTC_DATA_ATTR int boot_count;

	boot_count++;
	esp_sleep_enable_timer_wakeup(
	    60 * 1000000ULL);    // 60 с, у мкс
	esp_deep_sleep_start();
	// сюди виконання НЕ повертається:
	// пробудження = перезапуск app_main

	// light sleep — з продовженням:
	//   esp_light_sleep_start();

	// ################################

	// ############# память без купи ################
	// використання статичної памяті замість динамічної
	static StaticTask_t sensor_tcb;
	static StackType_t  sensor_stack[512];

	TaskHandle_t h = xTaskCreateStatic(
	    sensor_task, "sensor",
	    512, NULL, 4,
	    sensor_stack,
	    &sensor_tcb);

	static StaticQueue_t q_ctrl;
	static uint8_t q_buf[8 * sizeof(Sample)];
	QueueHandle_t q = xQueueCreateStatic(
	    8, sizeof(Sample), q_buf, &q_ctrl);
	// ############################################


	// потрібна опція:
	//   configSUPPORT_STATIC_ALLOCATION = 1



	#if LECTURE_SUPERVISE

	constexpr uint32_t kBitSensor = 1u << 0;
	constexpr uint32_t kBitComms  = 1u << 1;
	constexpr uint32_t kBitHb     = 1u << 2;
	constexpr uint32_t kBitStats  = 1u << 3;
	constexpr uint32_t kBitLink   = 1u << 4;
	constexpr uint32_t kBitAll    = 0x1F;

	volatile uint32_t aliveBits = 0;



	void superviseTask(void*) {
	    TickType_t wake = xTaskGetTickCount();
	    char buf[48];
	    for (;;) {
	        vTaskDelayUntil(&wake, pdMS_TO_TICKS(2000));
	        const uint32_t seen = aliveBits;
	        aliveBits = 0;
	        if ((seen & kBitAll) != kBitAll) {
	            const int n = snprintf(buf, sizeof buf,
	                "dbg wdg: stuck, missing mask=0x%02lX\r\n",
	                (unsigned long)(kBitAll & ~seen));
	            uartSend(buf, n);
	#if SUPERVISE_DO_RESET
	            NVIC_SystemReset();
	#endif
	        }
	    }
	}
	#endif

        // апаратний сторож для ESP є метод а для STM32 по маскам як в методі superviseTask

	// ESP32 — задачний сторож (TWDT):
	esp_task_wdt_add(NULL);
	for (;;) {
	    vTaskDelayUntil(&wake, PERIOD);
	    do_work();
	    esp_task_wdt_reset();
	}
	// не відмітилась за тайм-аут -> звіт з
	// ім'ям задачі; перезапуск — опція
	// CONFIG_ESP_TASK_WDT_PANIC

	// STM32 — той самий патєрн руками.
	// Кожна задача наприкінці циклу:
	aliveBits |= kBitSensor;

	// Наглядач (superviseTask), раз на 2 с:
	uint32_t seen = aliveBits;
	aliveBits = 0;
	if ((seen & kBitAll) == kBitAll)
	    HAL_IWDG_Refresh(&hiwdg);
	// інакше: біт назвав завислу задачу


	void sensorTask_new(void*) {
	    TickType_t wake = xTaskGetTickCount();
	#if LECTURE_MEASURE
	    uint32_t prevWake = DWT->CYCCNT;
	#endif
	    for (;;) {
	        vTaskDelayUntil(&wake, pdMS_TO_TICKS(100));
	#if LECTURE_MEASURE
	        const uint32_t nowCyc = DWT->CYCCNT;
	        const uint32_t dtUs = (nowCyc - prevWake) / (SystemCoreClock / 1000000u);
	        prevWake = nowCyc;
	        if (dtUs < jitMinUs) jitMinUs = dtUs;
	        if (dtUs > jitMaxUs) jitMaxUs = dtUs;
	        const uint32_t t0 = meterBegin();
	#endif
	        Sample s;
	        s.t_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
	        if (adsReadOnce(s.raw))
	            xQueueSend(q, &s, 0);
	#if LECTURE_MEASURE
	        meterEnd(mSensor, t0);
	#endif
	#if LECTURE_SUPERVISE
	        aliveBits |= kBitSensor;
	#endif
	    }
	}

    void sensorTask(void*) {
        TickType_t wake = xTaskGetTickCount();
        for (;;) {
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(100));
            Sample s;
            s.t_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (adsReadOnce(s.raw))
                xQueueSend(q, &s, 0);
        }
    }

    void commsTask(void*) {
        Sample s;
        char line[48];
        for (;;) {
            xQueueReceive(q, &s, portMAX_DELAY);
            if (!txEnabled) continue;
            const int32_t mv = int32_t(s.raw) * 125 / 1000;
            const int n = snprintf(line, sizeof line,
                                "t=%lu ms  raw=%d  U=%ld mV\r\n",
                                (unsigned long)s.t_ms, s.raw, (long)mv);
            uartSend(line, n);
        }
    }

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
            
            uartSend(msg, strlen(msg));
        }
    }


extern "C" {
	__attribute__((section(".noinit"))) volatile uint32_t g_faultCode;

	void appRecordFault(unsigned long code) {
	    g_faultCode = code;
	}
}

#if LECTURE_FAULT
    // тут код для  дебагу через uart коли нам треба знайти причину перезагрузки 
    // Це функція на C (STM32 HAL), яка визначає причину останнього перезавантаження мікроконтролера та відправляє відповідне діагностичне повідомлення через UART1.
    // Основна логіка роботи:

    // Перевірка Watchdog: __HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) перевіряє, чи відбувся скид через Independent Watchdog (сторожовий таймер зависання). Якщо так, у буфер записується "dbg boot: watchdog reset\r\n".
    // Перевірка програмного скиду / Fault: __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST) перевіряє прапорець Software Reset. Якщо це було програмне перезавантаження (наприклад, після Soft Fault / HardFault), 
    // форматується рядок із кодом помилки з глобальної змінної g_faultCode (CFSR значення).
    // Power On / Звичайний старт: Якщо жоден із прапорців скиду помилок не активний, вважається, що відбулося подане живлення (Power On або зовнішній Reset).
    // Відправка даних: HAL_UART_Transmit надсилає згенерований текстовий рядок через периферію huart1 з таймаутом 100 мс.
    // Очищення: Скидається глобальний код помилки (g_faultCode = 0) та очищаються прапорці ресету в регістрі RCC за допомогою __HAL_RCC_CLEAR_RESET_FLAGS(), щоб при наступному перезапуску не прочитати старі прапорці.

    // тут йде мова про регістрі в памяті які зберігають причину перезагрузки мікроконтлорела. Вони не очищаються!!!
	void reportBootReason() {
	    char buf[48];
	    int n;
	    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
	        n = snprintf(buf, sizeof buf, "dbg boot: watchdog reset\r\n");
	    } else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
	        n = snprintf(buf, sizeof buf,
	            "dbg boot: fault reset, CFSR=0x%08lX\r\n",
	            (unsigned long)g_faultCode);
	    } else {
	        n = snprintf(buf, sizeof buf, "dbg boot: power on\r\n");
	    }
	    HAL_UART_Transmit(&huart1, (uint8_t*)buf, n, 100);
	    g_faultCode = 0;
	    __HAL_RCC_CLEAR_RESET_FLAGS();
	}
}
#endif


// ось як тут міряти стеки задач
UBaseType_t rest =
    uxTaskGetStackHighWaterMark(NULL);

printf("sensor stack left: %u\n", rest);

// STM32 (слова): rest = 31 -> 124 байти
// ESP32 (байти): rest = 640

#if configCHECK_FOR_STACK_OVERFLOW > 0
void vApplicationStackOverflowHook(
        TaskHandle_t t, char* name) {
    log_fault("stack", name);
    NVIC_SystemReset();
}
#endif


// тут міряємо завантаження процесора
char buf[512];
vTaskGetRunTimeStats(buf);
printf("%s", buf);

// задача        час, тіки    частка
// sensor         12034        12%
// comms           8120         8%
// button             3        <1%
// IDLE           79151        79%

// увімкнення статистики:
//   ESP32:  menuconfig -> Component config
//     -> FreeRTOS -> Generate run time stats
//   STM32:  CubeMX -> FREERTOS -> Config
//     -> GENERATE_RUN_TIME_STATS


// Аналіз коду:
// Джерело високої точності: Для вимірювання часу використовується функція micros(). 
// На ESP32 це esp_timer_get_time(), а на STM32 (наприклад, F103 на 72 МГц) — лічильник DWT (DWT->CYCCNT),
// поділений на частоту процесора.
// Точна затримка: Використовується vTaskDelayUntil замість vTaskDelay, що гарантує виклики задачі з фіксованим періодом
// у 100 мс (100 000 мкс), незалежно від тривалості виконання do_work().
// Замір джитера: У кожній ітерації розраховується дельта dt і передається в статистику stats_add(dt).
// Результат: Звіт показує реальне відхилення таймінгу FreeRTOS (у даному прикладі джитер становить до ±140 мкс 
// від ідеальних 100 000 мкс, що зумовлено тактом SysTick та пріоритетами переривань/контекст-світчу).


// мікросекундний годинник:
//   ESP32:  esp_timer_get_time()
//   STM32:  DWT->CYCCNT / 72  (лічильник
//           тактів ядра, вмикається раз)

int64_t prev = micros();
for (;;) {
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(100));
    int64_t now = micros();
    int64_t dt  = now - prev;
    prev = now;
    stats_add(dt);
    do_work();
}
// звіт: period 100000 мкс,
// min 99980,  max 100140  ->  ±140 мкс



extern "C" void app_start(void) {
    q = xQueueCreate(8, sizeof(Sample));

    printf("App started\r\n");

    uartMtx = xSemaphoreCreateMutex();
    xTaskCreate(sensorTask, "sensor", 256, nullptr, 4, nullptr);
    xTaskCreate(commsTask, "comms", 256, nullptr, 3, nullptr);
    xTaskCreate(buttonTask, "button", 128, nullptr, 2, &btnTaskH);
    xTaskCreate(heartbeatTask, "hb", 192, nullptr, 1, nullptr);
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_0) {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(btnTaskH, &woken);
        portYIELD_FROM_ISR(woken);
    }
}
