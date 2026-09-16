#ifndef SERVER_H
#define SERVER_H

#include "esp_http_server.h"
#include "esp_log.h"
#include <stdint.h>
#include <stddef.h>
#include "uart.h"

extern volatile uint16_t sensor_period_ms;
extern volatile uint16_t display_period_ms;
extern volatile bool isConfigLoaded;


// ############### Функції генерації HTML-сторінок ####################

static esp_err_t sendLoadingHtml(httpd_req_t *req) {
    char resp_str[1024];

    snprintf(resp_str, sizeof(resp_str), R"rawliteral(
    <!DOCTYPE html>
        <html>
            <head>
                <meta charset='UTF-8'>
                <meta http-equiv='refresh' content='2'>
            </head>
            <body>
                <h2>System is initializing...</h2>
                <p>Please wait. Retrieving parameters from the device...</p>
            </body>
        </html>
    )rawliteral");

    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


static esp_err_t sendHtml(httpd_req_t *req) {
    char resp_str[4096]; // Буфер під всю HTML сторінку

    // Підготуємо статуси для radio кнопок
    const char *auto_chk = (strcmp(modeSelection, "auto") == 0) ? "checked" : "";
    const char *manual_chk = (strcmp(modeSelection, "manual") == 0) ? "checked" : "";
    const char *off_chk = (strcmp(modeSelection, "off") == 0) ? "checked" : "";

    snprintf(resp_str, sizeof(resp_str), R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset='utf-8'>
      <title>ESP32 Config</title>
    </head>
    <body>
      <h1>Налаштування пристрою</h1>
      <form action='/save' method='POST'>
        Команда: <input type='text' name='cmd' value='%s'><br>
        
        <p style='color: #4CAF50'>Щоб відправити параметри на STM32 введіть в поле cmd строку: CONFIG_TO_ESP32</p>
        <p style='color: #4CAF50'>Щоб змінити період показу даних введіть: DISPLAY_PERIOD</p>
        <p style='color: #4CAF50'>Щоб змінити період захвату даних з сенсора введіть: SENSOR_PERIOD</p>

        <br>
        <p>ПАРАМЕТРИ</p>
        Текстове ім'я: <input type='text' name='name' value='%s'><br><br>
        Числове значення: <input type='number' name='val' value='%d'><br><br>
       
        Режим роботи:<br>
        <input type='radio' id='auto' name='mode' value='auto' %s /> Автоматичний<br>
        <input type='radio' id='manual' name='mode' value='manual' %s /> Ручний<br>
        <input type='radio' id='off' name='mode' value='off' %s /> Вимкнено<br><br>
        <input type='submit' value='Зберегти'>
      </form>

    <div style="margin-top: 20px;">
        <h3>Керування світлодіодом</h3>
        <form action='/save' method='POST' style='display:inline;'>
            <input type='hidden' name='led' value='ON'>
            <button type='submit' style='padding: 10px 20px; background-color: #4CAF50; color: white;'>Увімкнути LED</button>
        </form>

        <form action='/save' method='POST' style='display:inline; margin-left: 10px;'>
            <input type='hidden' name='led' value='OFF'>
            <button type='submit' style='padding: 10px 20px; background-color: #f44336; color: white;'>Вимкнути LED</button>
        </form>
    </div>

      <h3>Поточні дані в пам'яті:</h3>
      Ім'я: %s<br>
      Значення: %d<br>
      Режим: %s<br>
      Led: %s<br>
    </body>
    </html>
    )rawliteral", 
    deviceCMD,
    deviceName, pwmValue, 
    auto_chk, manual_chk, off_chk, 
    deviceName, pwmValue, modeSelection, ledStatus);

    // Відправляємо сформовану сторінку браузеру
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t valid_period_command(int new_period, int min_val, int max_val, const char *name) {
    if (new_period >= min_val && new_period <= max_val) {
        return ESP_OK;
    }

    printf("NACK: Invalid %s period (use %d-%d)\n", name, min_val, max_val);
    fflush(stdout);
    return ESP_ERR_INVALID_ARG; 
}

static esp_err_t handleSave(httpd_req_t *req) {
    
    // Вмикаємо червоний світлодіод на початку виконання handleSave 
    // він буде сигналізувати про збереження даних
    gpio_set_level(LED_RED_GPIO, 1);

    // Отримуємо довжину тіла POST-запиту
    char content[256];
   
    if (getContent(req, content, sizeof(content)) != ESP_OK) {
        return ESP_FAIL;
    }

    // Якщо все добре, виводимо отриманий вміст
    printf("Отримано дані: %s\n", content);
   

    // Зчитуємо параметр 'cmd'
    if (getPostParam(content, "cmd", deviceCMD, sizeof(deviceCMD)) == ESP_OK) {
        deviceCMD[strcspn(deviceCMD, "\r\n")] = 0;

        if (strncmp(deviceCMD, "SENSOR_PERIOD ", 14) == 0) {
            uint16_t new_period = (uint16_t)strtoul(deviceCMD + 14, NULL, 10);
            if(valid_period_command(new_period, 20, 500, "Sensor") == ESP_OK) {
                sensor_period_ms = new_period;
                printf("Успішно змінено SENSOR_PERIOD! VAL: %u\n", sensor_period_ms);
            }           
        } 
        else if (strncmp(deviceCMD, "DISPLAY_PERIOD ", 15) == 0) {
            uint16_t new_period = (uint16_t)strtoul(deviceCMD + 15, NULL, 10);
            if(valid_period_command(new_period, 100, 30000, "Display") == ESP_OK) {
                display_period_ms = new_period;
                printf("Успішно змінено DISPLAY_PERIOD! VAL: %u\n", display_period_ms);
            }
        } else if (strncmp(deviceCMD, "CONFIG_TO_ESP32", 15) == 0) {

            // Зчитуємо параметр 'name'
            if (getPostParam(content, "name", deviceName, sizeof(deviceName)) != ESP_OK) {
                printf("Нема параметру: %s\n", "name");
            } 

            // Зчитуємо параметр 'mode'
            if (getPostParam(content, "mode", modeSelection, sizeof(modeSelection)) != ESP_OK) {
                printf("Нема параметру: %s\n", "mode");
            } 

            // Зчитуємо параметр 'val' 
            if (getPostParamInt(content, "val", &pwmValue) != ESP_OK) {
                printf("Нема параметру: %s\n", "val");
            } 

            sendData((const char *)deviceName, pwmValue, (const char *)modeSelection);

            printf("Успішно відправили на збереження дані  CONFIG_TO_ESP32\r\n");

        } 
        else {
            printf("ACK: Unknown command -> %s\n", deviceCMD);
            fflush(stdout);
        }
    }

    // Керування світлодіодом (параметр 'led' може бути "ON" або "OFF")
    if (getPostParam(content, "led", ledStatus, sizeof(ledStatus)) == ESP_OK) {
        if (strcmp(ledStatus, "ON") == 0) {
            gpio_set_level(LED_GPIO, 1); // Увімкнути світлодіод
            printf("LED is ON\n");
        } 
        else if (strcmp(ledStatus, "OFF") == 0) {
            gpio_set_level(LED_GPIO, 0); // Вимкнути світлодіод
            printf("LED is OFF\n");
        }
    }

    // Виводимо в консоль для перевірки (замість Serial.println)
    printf("Отримано нові налаштування:\n");
    printf("Ім'я: %s\n", deviceName);
    printf("Значення: %d\n", pwmValue);
    printf("Режим: %s\n", modeSelection);
    printf("Команда: %s\n", deviceCMD);

    vTaskDelay(pdMS_TO_TICKS(500)); // Затримка на 500 мілісекунд (0.5 секунд)

    // Успішне завершення handleSave: вимикаємо червоний світлодіод
    gpio_set_level(LED_RED_GPIO, 0);

    // Перенаправляємо користувача назад на головну сторінку (HTTP статус 303 See Other)
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0); // Відправляємо порожню відповідь для редиректу
    return ESP_OK;
}

static esp_err_t favicon_get_handler(httpd_req_t *req) {
    // Відправляємо статус 204 (No Content), щоб браузер знав, що іконки немає, і більше не спамив запитами
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// 1. Створюємо єдиний обробник для шляху "/"
static esp_err_t root_handler(httpd_req_t *req) {
    if (!isConfigLoaded) {
        // Повертаємо сторінку завантаження
        return sendLoadingHtml(req); 
    } else {
        // Повертаємо робочу сторінку
        return sendHtml(req); 
    }
}

// Функція запуску та роутингу сервера прямо в заголовочному файлі
static inline void route_webserver(httpd_handle_t server_handle) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Запускаємо сервер
    if (httpd_start(&server_handle, &config) == ESP_OK) {
        
        // Реєстрація головного маршруту (GET "/")
        httpd_uri_t root_uri = {
            .uri      = "/",
            .method   = HTTP_GET,
            .handler  = root_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle, &root_uri);

        // Реєстрація маршруту збереження (POST "/save")
        httpd_uri_t save_uri = {
            .uri      = "/save",
            .method   = HTTP_POST,
            .handler  = handleSave,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle, &save_uri);

        // Реєстрація маршруту для favicon.ico
        httpd_uri_t favicon_uri = {
            .uri      = "/favicon.ico",
            .method   = HTTP_GET,
            .handler  = favicon_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle, &favicon_uri);

        printf("HTTP server started and routed successfully\n");
    }
}

#endif // SERVER_H