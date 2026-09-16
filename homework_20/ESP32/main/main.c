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

// спосіб реалізації подій у freeTos закоментований
//static EventGroupHandle_t displayEventGroup;


// оголошуємо всі глобальні змінні та хендли
static TaskHandle_t displayTaskHandle = NULL;
i2c_master_dev_handle_t oled = NULL;
i2c_master_dev_handle_t mpu = NULL;

static volatile uint64_t last_interrupt_time = 0;
volatile uint16_t sensor_period_ms = 100;
volatile uint16_t display_period_ms = 5000;
volatile bool isConfigLoaded = false;

static esp_err_t sendHtml(httpd_req_t *req);
static esp_err_t sendLoadingHtml(httpd_req_t *req);
static esp_err_t handleSave(httpd_req_t *req);
static esp_err_t favicon_get_handler(httpd_req_t *req);

// Змінні для збереження наших параметрів
static char deviceCMD[64] = "DISPLAY_PERIOD 5000"; // Текстовий параметр команди
static char deviceName[20] = "ESP32_Device"; // Текстовий параметр
static int pwmValue = 128;                  // Числовий параметр (або твій sensor_period_ms)
static char modeSelection[20] = "auto";     // Вибір radio (замість String)
static char ledStatus[4] = "OFF";          // режим світодіода жовтого

// Хендл для веб-сервера ESP-IDF
static httpd_handle_t server = NULL;

// Налаштування Wi-Fi
#define ESP_WIFI_SSID      "ESP32-WIFI"
#define ESP_WIFI_PASS      "12345678"
#define ESP_WIFI_CHANNEL   0

#define BUTTON_GPIO           GPIO_NUM_4  // Або будь-який інший вільний PIN
#define EVENT_BUTTON_PRESSED  (1 << 0)
#define UART_NUM UART_NUM_1 // Використовуємо UART1

#define SENSOR_PERIOD 1
#define DISPLAY_PERIOD 2

#define TX_PIN_1 GPIO_NUM_17
#define RX_PIN_1 GPIO_NUM_18

#define LED_GPIO GPIO_NUM_5  // пін жовтого світлодіода
#define LED_RED_GPIO GPIO_NUM_19 // пін червоного світодіода (працює тільки коли зберігаються дані)

#define K_SDA GPIO_NUM_21
#define K_SCL GPIO_NUM_22
#define K_MPU_ADDR 0x68
#define K_OLED_ADDR 0x3C

typedef struct {
    float x, y, z;
} GyroSample;


static QueueHandle_t gyroQueue; // черга гіроскопа

#include "urldecode.h"
#include "http.h"
#include "uart.h"
#include "server.h"
#include "esp_http_server.h"
#include "i2c_config.h"
#include "oled.h"
#include "gpio.h"
#include "system_init.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "wifi.h"


// Сховища для даних
struct Answer currentAnswer;
struct Data currentData;

// Ініціалізація жовтого світлодіода
void init_led(void) {
    init_gpio_simple_output(LED_GPIO, 0);
}

// Ініціалізація червоного світлодіода
void init_red_led(void) {
    init_gpio_simple_output(LED_RED_GPIO, 0);
}

void init_button(void) {
    init_gpio_button(BUTTON_GPIO);
}

void init_uarts() {
    init_simple_uart(UART_NUM, TX_PIN_1, RX_PIN_1);
    //init_simple_uart(UART_NUM_CLIENT, TX_PIN_2, RX_PIN_2);
}

void initServer(void) {
    // Підключаємося до Wi-Fi
    // Створюємо та заповнюємо структуру конфігурації (включаючи канал, якщо потрібно)
    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = ESP_WIFI_SSID,
    //         .password = ESP_WIFI_PASS,
    //         .channel = 0 // 0 означає автосканування каналу
    //     },
    // };

    // Передаємо адресу структури у функцію
    //wifi_init_sta(&wifi_config);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,  // Назва вашої мережі, яку буде бачити телефон/ПК
            .channel = 1,
            .password = ESP_WIFI_PASS,        // Пароль від мережі (мінімум 8 символів)
            .max_connection = 4,           // Максимальна кількість пристроїв, що підключаються
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    // Передаємо адресу структури у функцію
    wifi_init_softap(&wifi_config);

    vTaskDelay(pdMS_TO_TICKS(2000)); // Невелика пауза на отримання IP-адреси
    printf("Connected! HTTP server starting...\n");
    route_webserver(&server);
}

static void regWrite(i2c_master_dev_handle_t d, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    ESP_ERROR_CHECK(i2c_master_transmit(d, buf, 2, pdMS_TO_TICKS(1000)));
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

static void sensorTask(void* pvParameters) {
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(sensor_period_ms));
        GyroSample s = mpuReadGyro();

        // Число 0 — це час очікування (таймаут) у тактах системного таймера (Ticks), 
        // який задача готова зачекати, якщо черга виявиться повністю заповненою.
        // xQueueSend(gyroQueue, &s, 0);
        // якщо 0 не будемо очікувати
        if (xQueueSend(gyroQueue, &s, 0) == pdPASS) {
            printf("Sent to queue -> X: %.1f, Y: %.1f, Z: %.1f\n", s.x, s.y, s.z);
        } 
    }
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
        // Відправляємо текст через UART_NUM
        sendAnswer(result_str);

    }
}


static void uartReceiveTask(void* pvParameters)
{
    uart_parser_t parser;
    parser_init(&parser);

    // Буфер для пакетного читання (ефективніше, ніж по 1 байту)
    uint8_t rx_buf[128];
    
    uint8_t outType;
    uint8_t outPayload[512];
    uint8_t outLen;

    while (1) {
        // Читаємо дані з UART. Функція блокується на 100 мс (100 / portTICK_PERIOD_MS)
        // замість використання usleep(), що дозволяє FreeRTOS оптимізувати час процесора.
        int rx_len = uart_read_bytes(UART_NUM_1, rx_buf, sizeof(rx_buf), 100 / portTICK_PERIOD_MS);

        for (int i = 0; i < rx_len; i++) {
            if (parser_feed(&parser, rx_buf[i], &outType, outPayload, &outLen)) {
                if (outType == PKT_ANSWER && outLen == sizeof(struct Answer)) {
                    memcpy(&currentAnswer, outPayload, sizeof(struct Answer));
                    printf("Отримали відповідь!  Msg: %s\n", currentAnswer.msg);
                }
                // answer from stm32 with structer Data
                else if (outType == PKT_DATA && outLen == sizeof(struct Data)) {
                    memcpy(&currentData, outPayload, sizeof(struct Data));
                    isConfigLoaded = true;

                    strncpy(deviceName, currentData.name, sizeof(deviceName) - 1);
                    deviceName[sizeof(deviceName) - 1] = '\0';

                    pwmValue = currentData.val;

                    strncpy(modeSelection, currentData.mode, sizeof(modeSelection) - 1);
                    modeSelection[sizeof(modeSelection) - 1] = '\0';

                    printf("GET PKT_DATA!\n");
                    printf("deviceName: %s\n", deviceName);
                    printf("pwmValue: %d\n", pwmValue);
                    printf("modeSelection: %s\n", modeSelection);
                }
                // answer for test (cyclic uart only for ESP32) 
                else if (outType == PKT_GET_DATA) {
                    isConfigLoaded = true;
                    printf("TEST ANSWER PKT_DATA!\n");
                }
                else {
                    printf("NO NAME! outType: %d\n", outType);
                }
            }
        }
        
    }
}

void app_main(void) {

    // Спочатку ініціалізуємо NVS (один раз на старті програми)
    init_nvs();

    i2cInit();
    mpuInit();
    oledInit();
    init_led();
    init_red_led();
    init_button();

    init_uarts();
     
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
    // зберігатимуться локальні змінні задачі та адреси викликів внутрішніх функцій.
    // (Для задач із роботою по I2C/SPI значення 2048–4096 є стандартним мінімумом).
    // 4) NULL (Параметри для функції):
    // Вказівник на дані, які можна передати у функцію sensorTask при її запуску. Якщо передавати нічого
    //  не потрібно — ставлять NULL.
    // 5 (Пріоритет задачи):
    // Рівень важливості задачи (число від 0 до configMAX_PRIORITIES - 1). Чим вище число, тим вищий
    // пріоритет. Задача з пріоритетом 5 виконуватиметься раніше за задачу з пріоритетом 2, перериваючи її,
    // коли сенсору потрібно терміново вичитати дані.
    // NULL (Хендл задачи / Task Handle):
    // Вказівник на змінну типу TaskHandle_t. Використовується, якщо вам потрібно зберегти «посилання» 
    // на створену задачу, щоб пізніше з іншого місця програми зупинити її, 
    // видалити (vTaskDelete) або змінити її пріоритет. Якщо це не потрібно — передають NULL.

    xTaskCreate(displayTask, "display", 4096, NULL, 4, &displayTaskHandle);
  
    initServer();

    vTaskDelay(pdMS_TO_TICKS(2000));

    sendConfigRequest();

    xTaskCreate(uartReceiveTask, "uart_rec", 4096, NULL, 6, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



