#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

// Налаштування Wi-Fi для Wokwi
#define EXAMPLE_ESP_WIFI_SSID      "Wokwi-GUEST"
#define EXAMPLE_ESP_WIFI_PASS      ""

// Змінні для збереження наших параметрів
static char deviceName[32] = "ESP32_Device"; // Текстовий параметр
static int pwmValue = 128;                  // Числовий параметр (або твій sensor_period_ms)
static char modeSelection[16] = "auto";     // Вибір radio (замість String)

// Хендл для веб-сервера ESP-IDF
static httpd_handle_t server = NULL;

#define BUTTON_GPIO           GPIO_NUM_4  // Або будь-який інший вільний PIN
#define EVENT_BUTTON_PRESSED  (1 << 0)
#define UART_NUM UART_NUM_1 // Використовуємо UART1

#define SENSOR_PERIOD 1
#define DISPLAY_PERIOD 2

#define UART_NUM_CLIENT UART_NUM_2
#define TX_PIN_2 20
#define RX_PIN_2 21

#define TX_PIN_1 17
#define RX_PIN_1 18

// це інший спосіб поки закоментований
//static EventGroupHandle_t displayEventGroup;

static volatile uint64_t last_interrupt_time = 0;

static TaskHandle_t displayTaskHandle = NULL;

static volatile int sensor_period_ms = 100;
static volatile int display_period_ms = 5000;


#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

static void wifi_init_sta(void) {
    // Ініціалізація NVS (обов'язково для Wi-Fi в ESP-IDF)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NEW_VERSION_FOUND || ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Wokwi-GUEST",
            .password = "",
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    printf("Connecting to WiFi...\n");
    // Утиліта блокується, поки не підключиться (або можна чекати через події)
    esp_wifi_connect();
}


void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Налаштовуємо параметри (конфігурація, буфери)
    uart_param_config(UART_NUM, &uart_config);

    // Прив'язуємо піни: 17 - TX, 18 - RX (-1 означає без керування потоком)
    uart_set_pin(UART_NUM, TX_PIN_1, RX_PIN_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Встановлюємо драйвер (номер, буфер на приймання, буфер на передачу, черга)
    uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0);
}

// Функція генерації HTML-сторінки
static esp_err_t sendHtml(httpd_req_t *req) {
    char resp_str[2048]; // Буфер під всю HTML сторінку

    // Підготуємо статуси для radio кнопок
    const char *auto_chk = (strcmp(modeSelection, "auto") == 0) ? "checked" : "";
    const char *manual_chk = (strcmp(modeSelection, "manual") == 0) ? "checked" : "";
    const char *off_chk = (strcmp(modeSelection, "off") == 0) ? "checked" : "";

    // Формуємо HTML за один раз через snprintf (замість .replace)
    snprintf(resp_str, sizeof(resp_str), R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset='utf-8'>
      <title>ESP32 Config</title>
    </head>
    <body>
      <h1>Налаштування пристрою</h1>
      <form action='/save' method='GET'>
        Текстове ім'я: <input type='text' name='name' value='%s'><br><br>
        Числове значення: <input type='number' name='val' value='%d'><br><br>
        
        Режим роботи:<br>
        <input type='radio' id='auto' name='mode' value='auto' %s> Автоматичний<br>
        <input type='radio' id='manual' name='mode' value='manual' %s> Ручний<br>
        <input type='radio' id='off' name='mode' value='off' %s> Вимкнено<br><br>
        
        <input type='submit' value='Зберегти'>
      </form>

      <h3>Поточні дані в пам'яті:</h3>
      Ім'я: %s<br>
      Значення: %d<br>
      Режим: %s<br>
    </body>
    </html>
    )rawliteral", 
    deviceName, pwmValue, 
    auto_chk, manual_chk, off_chk, 
    deviceName, pwmValue, modeSelection);

    // Відправляємо сформовану сторінку браузеру
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t handleSave(httpd_req_t *req) {
    char query[256];
    
    // Перевіряємо, чи є параметри в URL-запиті
    if (httpd_req_get_url_query_len(req) < sizeof(query)) {
        if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
            char param_buf[64];

            // 1. Зчитуємо параметр 'name'
            if (httpd_query_key_value(query, "name", param_buf, sizeof(param_buf)) == ESP_OK) {
                // Можна додати базову заміну знаків %20 (пробіли), якщо браузер їх кодує
                // snprintf(deviceName, sizeof(deviceName), "%s", param_buf);
                strlcpy(deviceName, param_buf, sizeof(deviceName));
            }

            // 2. Зчитуємо параметр 'val'
            if (httpd_query_key_value(query, "val", param_buf, sizeof(param_buf)) == ESP_OK) {
                pwmValue = atoi(param_buf);
            }

            // 3. Зчитуємо параметр 'mode'
            if (httpd_query_key_value(query, "mode", param_buf, sizeof(param_buf)) == ESP_OK) {
                //snprintf(modeSelection, sizeof(modeSelection), "%s", param_buf);
                strlcpy(modeSelection, param_buf, sizeof(modeSelection));
            }
        }
    }

    // Виводимо в консоль для перевірки (замість Serial.println)
    printf("Отримано нові налаштування:\n");
    printf("Ім'я: %s\n", deviceName);
    printf("Значення: %d\n", pwmValue);
    printf("Режим: %s\n", modeSelection);

    // Перенаправляємо користувача назад на головну сторінку (HTTP статус 303 See Other)
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0); // Відправляємо порожню відповідь для редиректу
    return ESP_OK;
}

void init_text_uart(void) {
    uart_config_t uart_config2 = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_NUM_CLIENT, &uart_config2);
    uart_set_pin(UART_NUM_CLIENT, TX_PIN_2, RX_PIN_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_CLIENT, 1024, 0, 0, NULL, 0);
}

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
        
        //esp_rom_printf("Button Pressed (ISR)!\n");

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

void initServer(void) {
    // Підключаємося до Wi-Fi замість WiFi.begin
    wifi_init_sta();
    
    vTaskDelay(pdMS_TO_TICKS(2000)); // Невелика пауза на отримання IP-адреси в симуляторі
    printf("Connected! HTTP server starting...\n");

    // Конфігурація нативного веб-сервера ESP-IDF
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Реєстрація головного маршруту (замість server.on("/", sendHtml))
        httpd_uri_t root_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = sendHtml, // функція, що формує твій HTML
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root_uri);

        // Реєстрація маршруту збереження (замість server.on("/save", handleSave))
        httpd_uri_t save_uri = {
            .uri       = "/save",
            .method    = HTTP_GET,
            .handler   = handleSave, // функція обробки даних з форми
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &save_uri);

        printf("HTTP server started\n");
    }
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
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(100));
        GyroSample s = mpuReadGyro();

        // Число 0 — це час очікування (таймаут) у тактах системного таймера (Ticks), 
        // який задача готова зачекати, якщо черга виявиться повністю заповненою.
        // xQueueSend(gyroQueue, &s, 0);

        // Намагаємося відправити дані в чергу з таймаутом 0
        if (xQueueSend(gyroQueue, &s, 0) == pdPASS) {
            // Використовуємо printf з \n та fflush
            //printf("Sent to queue -> X: %.1f, Y: %.1f, Z: %.1f\n", s.x, s.y, s.z);
            //fflush(stdout); 
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
static void updateDisplayData( GyroSample *s, bool has_data ) {

    if (!has_data) {
      return;
    }

    char line[24];
    snprintf(line, sizeof(line), "gx: %8.1f", s->x);
    oledText(1, line);
    snprintf(line, sizeof(line), "gy: %8.1f", s->y);
    oledText(3, line);
    snprintf(line, sizeof(line), "gz: %8.1f", s->z);
    oledText(5, line);

    oledFlush();
    oledPowerOn();

    vTaskDelay(pdMS_TO_TICKS(500)); // Показуємо 0.5 с
    oledPowerOff();
    
}

static void displayTask(void* pvParameters) {
    // GyroSample s;
    //char line[24];

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
        uint32_t ulNotifiedValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(display_period_ms));

        if (ulNotifiedValue > 0) {
           // printf("Push button!\n");
        } else {
           // printf("Timeout 5s elapsed\n");
        }

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

        updateDisplayData(&s, has_data);

        char result_str[128];
        sprintf(result_str, "Result: gx: %8.1f, gy: %8.1f, gz: %8.1f, display time: %d, sensor time: %d\n", s.x, s.y, s.z, display_period_ms, sensor_period_ms);
        // Відправляємо текст через UART_NUM_CLIENT
        uart_write_bytes(UART_NUM_CLIENT, result_str, strlen(result_str));
    }
}

// Функція розрахунку CRC16 (наприклад, Modbus або CCITT)
uint16_t calculateCRC(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc;
}


void sendPacket(uint8_t cmd, uint16_t val) {
    uint8_t packet[6]; // 1 (заголовок) + 1 (команда) + 2 (значення) + 2 (CRC) = 6 байт
    packet[0] = 0xAA; // Заголовок
    packet[1] = cmd;  // Команда
    packet[2] = (uint8_t)(val & 0xFF);       // Молодший байт значення
    packet[3] = (uint8_t)(val >> 8);         // Старший байт значення

    // Рахуємо CRC для перших 4 байт (заголовок, команда, значення [2 байти])
    uint16_t crc = calculateCRC(packet, 4); 
    
    packet[4] = (uint8_t)(crc & 0xFF);       // Молодший байт CRC
    packet[5] = (uint8_t)(crc >> 8);         // Старший байт CRC

    // Відправляємо масив байтів у UART (тепер довжина 6 байт)
    uart_write_bytes(UART_NUM, (const char *)packet, 6);
    
    printf("Відправлено пакет -> CMD: 0x%02X, VAL: %d, CRC: 0x%04X\n", cmd, val, crc);
}

static void handle_period_command(uint8_t cmd, const char *buffer, int prefix_len, volatile int *target_var, int min_val, int max_val, const char *name) {
    int new_period = atoi(buffer + prefix_len);
    if (new_period >= min_val && new_period <= max_val) {
        sendPacket((uint8_t)cmd, (uint16_t)new_period);
    } else {
        printf("NACK: Invalid %s period (use %d-%d)\n", name, min_val, max_val);
    }
    fflush(stdout);
}

// Головна задача, яка слухає ввід із консолі (Serial Monitor)
void uartCommandTask(void *pvParameters) {
    printf("Введіть команду (напр: SENSOR_PERIOD 500 або DISPLAY_PERIOD 5000) і натисніть Enter:\n");

    char buffer[64];
    int index = 0;

    while (1) {
        uint8_t ch;
        // Читаємо по одному байту з UART1 (або іншого порту) з таймаутом 50 мс
        int len = uart_read_bytes(UART_NUM, &ch, 1, pdMS_TO_TICKS(50));

        if (len <= 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue; // Нічого не прийшло, йдемо далі
        }

        // if (ch == EOF) {
        //     vTaskDelay(pdMS_TO_TICKS(50));
        //     continue;
        // }
        
        if (ch == '\n' || ch == '\r') {
            buffer[index] = '\0'; // Завершуємо рядок
            
            if (index > 0) {
                buffer[strcspn(buffer, "\r\n")] = 0;

                if (strncmp(buffer, "SENSOR_PERIOD ", 14) == 0) {
                    handle_period_command(SENSOR_PERIOD, buffer, 14, &sensor_period_ms, 20, 500, "Sensor");
                } 
                else if (strncmp(buffer, "DISPLAY_PERIOD ",
                     15) == 0) {
                    handle_period_command(DISPLAY_PERIOD, buffer, 15, &display_period_ms, 100, 30000, "Display");
                } 
                else {
                    printf("ACK: Unknown command -> %s\n", buffer);
                    fflush(stdout);
                }
            }
            index = 0;
        } 
        else if (index < sizeof(buffer) - 1) {
            buffer[index++] = (char)ch;
        }
    }
}

static void uartReceiveTask(void* pvParameters) {
    uint8_t rx_buffer[6];
    size_t total_read = 0;

    while (1) {
        // Читаємо байти з UART1 (пін 18)
        // Чекаємо поки назбирається весь пакет із 6 байтів
        int length = uart_read_bytes(UART_NUM_1, &rx_buffer[total_read], 6 - total_read, pdMS_TO_TICKS(100));
        
        if (length > 0) {
            total_read += length;
            
            // Якщо є перший байт, але він не 0xAA (синхронізація по заголовоку)
            if (total_read > 0 && rx_buffer[0] != 0xAA) {
                // Зсуваємо буфер у пошуках правильного заголовка
                rx_buffer[0] = rx_buffer[1]; // спрощений приклад зсуву
                total_read = 1;
                continue;
            }

            // Як тільки назбирали рівно 6 байтів пакету
            if (total_read >= 6) {
                //uint8_t header = rx_buffer[0];
                uint8_t cmd    = rx_buffer[1];
                uint16_t  val =  rx_buffer[2] | ((uint16_t)rx_buffer[3] << 8);
                
                // Збираємо назад CRC із двох байтів (молодший + старший)
                uint16_t received_crc = rx_buffer[4] | ((uint16_t)rx_buffer[5] << 8);
                
                // Рахуємо CRC для перших трьох байтів отриманого пакету
                uint16_t calculated_crc = calculateCRC(rx_buffer, 4);

                // Перевіряємо цілісність
                if (calculated_crc == received_crc) {
                    if(cmd == SENSOR_PERIOD) {
                        sensor_period_ms = val;
                        char cmd_str[64];
                        sprintf(cmd_str, "Успішно змінено SENSOR_PERIOD! VAL: %d\n", val);

                        // Відправляємо текст через UART2
                        uart_write_bytes(UART_NUM_CLIENT, cmd_str, strlen(cmd_str));
                      
                    }

                    if(cmd == DISPLAY_PERIOD) {
                        display_period_ms = val;
                        char cmd_str[64];
                        sprintf(cmd_str, "Успішно змінено DISPLAY_PERIOD! VAL: %d\n", val);
                        // Відправляємо текст через UART2
                        uart_write_bytes(UART_NUM_CLIENT, cmd_str, strlen(cmd_str));
                    }
                    
                } else {
                    printf("Помилка CRC! Отримано: 0x%04X, розраховано: 0x%04X\n", received_crc, calculated_crc);
                }

                // Скидаємо лічильник для читання наступного пакету
                total_read = 0;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void uartReceiveAnswer(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(2000)); // Чекаємо ініціалізації системи
   
    while (1) {

        // Слухаємо відповідь (ACK / NACK) у буфер
        char rx_buf[128] = {0};
        int len = uart_read_bytes(UART_NUM_CLIENT, rx_buf, sizeof(rx_buf) - 1, pdMS_TO_TICKS(1000));

        if (len > 0) {
            rx_buf[len] = '\0'; // Завершуємо рядок
            // Очищаємо від зайвих символів переходу рядка
            rx_buf[strcspn(rx_buf, "\r\n")] = 0;
            
            printf("[UART2 Client] Отримано відповідь: [%s]\n", rx_buf);
        } 

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void app_main(void) {
    i2cInit();
    mpuInit();
    oledInit();
    init_uart();
    init_text_uart();
    

    // Створюємо Event Group
    //displayEventGroup = xEventGroupCreate();

    // Ініціалізуємо кнопку
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
    xTaskCreate(uartCommandTask, "uart_cmd", 4096, NULL, 3, NULL);
    xTaskCreate(uartReceiveTask, "uart_rec", 4096, NULL, 6, NULL);
    xTaskCreate(uartReceiveAnswer, "answer_client", 4096, NULL, 2, NULL);   

    initServer();
}



