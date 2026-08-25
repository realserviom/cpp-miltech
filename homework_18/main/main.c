#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "SENSOR"; // Тег, який буде відображатися у логах


#include "driver/gpio.h"
#include "freertos/event_groups.h"

#define BUTTON_GPIO           GPIO_NUM_4  // Або будь-який інший вільний PIN
#define EVENT_BUTTON_PRESSED  (1 << 0)

static EventGroupHandle_t displayEventGroup;

static volatile uint64_t last_interrupt_time = 0;

static TaskHandle_t displayTaskHandle = NULL;

static volatile int sensor_period_ms = 100;

static void IRAM_ATTR button_isr_handler(void* arg) {

    if (gpio_get_level(BUTTON_GPIO) != 0) {
        return;
    }

    uint64_t current_time = esp_timer_get_time(); // Поточний час у мкс

    // Пропускаємо виклик, якщо з моменту 
    // останнього минуло менше 200 мс (200,000 мкс)
    if (current_time - last_interrupt_time > 300000) {
        last_interrupt_time = current_time;

        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        esp_rom_printf("Button Pressed (ISR)!\n");

        // Відправляємо сигнал у Event Group
        // xEventGroupSetBitsFromISR(displayEventGroup, EVENT_BUTTON_PRESSED,
        // &xHigherPriorityTaskWoken);

        // Сповіщаємо конкретну задачу напряму з ISR
        vTaskNotifyGiveFromISR(displayTaskHandle, &xHigherPriorityTaskWoken);
        
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

void init_button(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE // Реакція на натискання (High -> Low)
    };
    gpio_config(&io_conf);

    // Встановлюємо та підключаємо ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
}

#define K_SDA 8
#define K_SCL 9
#define K_MPU_ADDR 0x68
#define K_OLED_ADDR 0x3C

typedef struct {
    float x, y, z;
} GyroSample;

static i2c_master_dev_handle_t mpu;
static i2c_master_dev_handle_t oled;
static QueueHandle_t gyroQueue; // черга гіроскопа

static void i2cInit(void) {
    i2c_master_bus_config_t bus = {
        .i2c_port = I2C_NUM_0, // перший по порядку I2C є ще один
        .sda_io_num = (gpio_num_t)K_SDA,
        .scl_io_num = (gpio_num_t)K_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT, // генератор частоти
        .glitch_ignore_cnt = 7, // ігноримо сигнал який менше 7 тактів
    };
    i2c_master_bus_handle_t busH; // це шина що створена конфігурацією bus
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus, &busH));

    i2c_device_config_t dev = {
        .scl_speed_hz = 400000, // швидкість тактування
        .device_address = K_MPU_ADDR, // адрес приладу  0x68
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(busH, &dev, &mpu)); // привязка приладу до шини
    dev.device_address = K_OLED_ADDR; // адрес моніторчика 0x3C
    ESP_ERROR_CHECK(i2c_master_bus_add_device(busH, &dev, &oled)); // привязка моніторчика до шини
}

static void regWrite(i2c_master_dev_handle_t d, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    ESP_ERROR_CHECK(i2c_master_transmit(d, buf, 2, -1));
}

static void mpuInit(void) {
    regWrite(mpu, 0x6B, 0x00);
}

static GyroSample mpuReadGyro(void) {
    uint8_t reg = 0x43;
    uint8_t d[6];
    ESP_ERROR_CHECK(i2c_master_transmit_receive(mpu, &reg, 1, d, 6, -1));
    int16_t rx = (d[0] << 8) | d[1];
    int16_t ry = (d[2] << 8) | d[3];
    int16_t rz = (d[4] << 8) | d[5];
    
    GyroSample sample = { rx / 131.0f, ry / 131.0f, rz / 131.0f };
    return sample;
}

static uint8_t fb[1024];

static void oledCmd(const uint8_t *cmds, size_t len) {
    uint8_t buf[32] = { 0x00 };
    memcpy(buf + 1, cmds, len);
    ESP_ERROR_CHECK(i2c_master_transmit(oled, buf, len + 1, -1));
}

static void oledInit(void) {
    static const uint8_t init_cmds[] = {
        0xAE,             // Display OFF
        0xD5, 0x80,       // Set Display Clock
        0xA8, 0x3F,       // Set Multiplex Ratio
        0xD3, 0x00, 0x40, // Set Display Offset & Start Line
        0x8D, 0x14,       // Enable Charge Pump
        0x20, 0x00,       // Set Memory Addressing Mode (Horizontal)
        0xA1, 0xC8,       // Flip Screen (Segment Remap & COM Scan Direction)
        0xDA, 0x12,       // Set COM Pins
        0x81, 0xCF,       // Set Contrast
        0xD9, 0xF1,       // Set Pre-charge Period
        0xDB, 0x40,       // Set VCOMH Deselect Level
        0xA4, 0xA6, 0xAF  // Output RAM, Normal Display, Display ON
    };

    oledCmd(init_cmds, sizeof(init_cmds));
}
static void oledFlush(void) {
    uint8_t c1[] = {0x21, 0, 127}; oledCmd(c1, 3);
    uint8_t c2[] = {0x22, 0, 7}; oledCmd(c2, 3);
    static uint8_t out[1025];
    out[0] = 0x40;
    memcpy(out + 1, fb, 1024);
    ESP_ERROR_CHECK(i2c_master_transmit(oled, out, 1025, -1));
}

static const uint8_t* glyph(char c) {
    static const uint8_t d[][5] = {
        {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
        {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
        {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
        {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
        {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    };
    static const uint8_t minus[5] = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t dot  [5] = {0x00,0x60,0x60,0x00,0x00};
    static const uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t sp   [5] = {0,0,0,0,0};
    static const uint8_t g_   [5] = {0x0C,0x52,0x52,0x52,0x3E};
    static const uint8_t x_   [5] = {0x44,0x28,0x10,0x28,0x44};
    static const uint8_t y_   [5] = {0x0C,0x50,0x50,0x50,0x3C};
    static const uint8_t z_   [5] = {0x44,0x64,0x54,0x4C,0x44};

    if (c >= '0' && c <= '9') return d[c - '0'];
    switch (c) {
        case '-': return minus; case '.': return dot;
        case ':': return colon; case 'g': return g_;
        case 'x': return x_;     case 'y': return y_;
        case 'z': return z_;     default:  return sp;
    }
}

static void oledText(int page, const char* s) {
    uint8_t* p = fb + page * 128;
    memset(p, 0, 128);
    for (int col = 0; *s && col + 6 <= 128; ++s, col += 6) {
        memcpy(p + col, glyph(*s), 5);  
    }
}

static void sensorTask(void* pvParameters) {
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(sensor_period_ms));
        GyroSample s = mpuReadGyro();

        // Число 0 — це час очікування (таймаут) у тактах системного таймера (Ticks), 
        // який задача готова зачекати, якщо черга виявиться повністю заповненою.
        // xQueueSend(gyroQueue, &s, 0);

        // Намагаємося відправити дані в чергу з таймаутом 0
        if (xQueueSend(gyroQueue, &s, 0) == pdPASS) {
            // Використовуємо printf з \n та fflush
            printf("UART TX -> Gyro X: %.1f, Y: %.1f, Z: %.1f [Period: %dms]\n", s.x, s.y, s.z, sensor_period_ms);
            fflush(stdout); 
        } 
        
         {
           // printf("Queue full! Sample dropped.\n");
           // fflush(stdout);
        }
    }
}

// Вмикаємо екран (Display ON)
static void oledPowerOn(void) {
    uint8_t cmd = 0xAF;
    oledCmd(&cmd, 1);
}

// Вимикаємо екран (Display OFF)
static void oledPowerOff(void) {
    uint8_t cmd = 0xAE;
    oledCmd(&cmd, 1);
}

// Винесемо логіку малювання в окрему допоміжну функцію
static void updateDisplayData(void) {
    GyroSample s;
    bool has_data = false;

    // Вичитаємо найсвіжіші дані з черги
    while (xQueueReceive(gyroQueue, &s, 0) == pdPASS) {
        has_data = true;
    }

    // Якщо в черзі нічого не було, даємо 100 мс на отримання першого ж виміру
    if (!has_data) {
        has_data = (xQueueReceive(gyroQueue, &s, pdMS_TO_TICKS(100)) == pdPASS);
    }

    if (has_data) {
        char line[24];
        snprintf(line, sizeof(line), "gx: %8.1f", s.x);
        oledText(1, line);
        snprintf(line, sizeof(line), "gy: %8.1f", s.y);
        oledText(3, line);
        snprintf(line, sizeof(line), "gz: %8.1f", s.z);
        oledText(5, line);

        oledFlush();
        oledPowerOn();

        vTaskDelay(pdMS_TO_TICKS(500)); // Показуємо 0.5 с
        oledPowerOff();
    }
}

static void displayTask(void* pvParameters) {
    GyroSample s;
    char line[24];

    oledPowerOff();

    for (;;) {
        // Задача засинає і чекає, поки в черзі з'явиться новий вимір від гіроскопа. 
        // Параметр portMAX_DELAY означає, що вона чекатиме нескінченно, не витрачаючи процесорний час, 
        // поки сенсор не надішле нові дані.
        // xQueueReceive(gyroQueue, &s, portMAX_DELAY);

        // Чекаємо подію через xEventGroupWaitBits (це один спосіб)
        // EventBits_t bits = xEventGroupWaitBits(
        //     displayEventGroup,
        //     EVENT_BUTTON_PRESSED,
        //     pdTRUE,
        //     pdFALSE,
        //     pdMS_TO_TICKS(5000)
        // );

        // // Перевіряємо, ЧОМУ прокинулася задача
        // if (bits & EVENT_BUTTON_PRESSED) {
        //     printf("Push button!\n");
        //     fflush(stdout);
        // } else {
        //     printf("Timeout 5s elapsed\n");
        //     fflush(stdout);
        // }


        // інший спосіб по лічильнику переривань в самій функції викликається
        // ulTaskNotifyTake який шукає контекст суто того методу з якого його викликали
        // і по displayTaskHandle який фігурує в методі переривання button_isr_handler кнопки
        uint32_t ulNotifiedValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));

        if (ulNotifiedValue > 0) {
            printf("Push button!\n");
        } else {
            printf("Timeout 5s elapsed\n");
        }


        updateDisplayData();
    }
}

// Читання команд по UART від комп'ютера
static void uartCommandTask(void* pvParameters) {
    char rx_buffer[64];
    
    // Налаштування UART0 для читання з консолі (stdin)
    // У ESP-IDF стандартний ввід вже прив'язаний до UART0, тому можемо читати через getchar або fgets
    while (1) {
        // Читаємо рядок з термінала комп'ютера (блокуючий виклик)
        if (fgets(rx_buffer, sizeof(rx_buffer), stdin) != NULL) {
            // Прибираємо символ перенесення рядка в кінці
            rx_buffer[strcspn(rx_buffer, "\r\n")] = 0;

            // Перевіряємо команду зміни періоду, наприклад: "PERIOD 500" або просто число "250"
            if (strncmp(rx_buffer, "PERIOD ", 7) == 0) {
                int new_period = atoi(rx_buffer + 7);
                if (new_period >= 20 && new_period <= 5000) {
                    sensor_period_ms = new_period;
                    // Підтвердження рядком у відповідь на комп'ютер за вимогами задачі
                    printf("ACK: Period changed to %d ms\n", sensor_period_ms);
                } else {
                    printf("NACK: Invalid period value (use 20-5000)\n");
                }
                fflush(stdout);
            } else if (strlen(rx_buffer) > 0) {
                printf("ACK: Unknown command -> %s\n", rx_buffer);
                fflush(stdout);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void) {
    i2cInit();
    mpuInit();
    oledInit();

    // 1. Створюємо Event Group
    displayEventGroup = xEventGroupCreate();

    // 2. Ініціалізуємо кнопку
    init_button();


    // Для сенсорів завжди роблять так. Створюють буфер з одиним значенням
    // xQueueCreate(1, sizeof(GyroSample))
    // Завжди перезаписує єдиний елемент у черзі найновішим значенням
    // xQueueOverwrite(gyroQueue, &sample);

    // створюємо чергу з 8 елементів розміром sizeof(GyroSample)
    // ця черга не буде переписуватися якщо заповниться буфер до 8 елементів
    gyroQueue = xQueueCreate(8, sizeof(GyroSample)); 
    xTaskCreate(sensorTask, "sensor", 4096, NULL, 5, NULL);

    // Ось що означає кожен із 6 параметрів по порядку:

    // 1) sensorTask (Вказівник на функцію):
    // Ім'я C-функції (void sensorTask(void *pvParameters)), яку FreeRTOS має запустити як окрему задачу. 
    // У ній зазвичай знаходиться нескінченний цикл while(1).
    // 2) "sensor" (Текстове ім'я):
    // Назва задачи у вигляді рядка. Потрібна виключно для зручності відлагодження (наприклад, 
    // для виводу у логи ESP-IDF або у системному моніторі задач).
    // 3) 4096 (Розмір стека в байтах):
    // Розмір пам'яті (Stack), яку операційна система виділяє під цю задачу (4 КБ). У цій пам'яті 
    // зберігатимуться локальні змінні задачи та адреси викликів внутрішніх функцій.
    // (Для задач із роботою по I2C/SPI значення 2048–4096 є стандартним мінімумом).
    // 4) NULL (Параметри для функції):
    // Вказівник на дані, які можна передати у функцію sensorTask при її запуску. Якщо передавати нічого
    //  не потрібно — ставлять NULL.
    // 5 (Пріоритет задачи):
    // Рівень важливості задачи (число від 0 до configMAX_PRIORITIES - 1). Чим вище число, тим вищий
    //  пріоритет. Задача з пріоритетом 5 виконуватиметься раніше за задачу з пріоритетом 2, перериваючи її,
    // коли сенсору потрібно терміново вичитати дані.
    // NULL (Хендл задачи / Task Handle):
    // Вказівник на змінну типу TaskHandle_t. Використовується, якщо вам потрібно зберегти «посилання» 
    // на створену задачу, щоб пізніше з іншого місця програми зупинити її, 
    // видалити (vTaskDelete) або змінити її пріоритет. Якщо це не потрібно — передають NULL.

    xTaskCreate(displayTask, "display", 4096, NULL, 4, &displayTaskHandle);

    // обробки вхідних команд UART
    xTaskCreate(uartCommandTask, "uart_cmd", 4096, NULL, 3, NULL);
    
    printf("UART initialized. System started successfully. Send 'PERIOD <ms>' to change rate.\n");
    fflush(stdout);
}