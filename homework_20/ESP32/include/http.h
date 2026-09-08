#include <esp_http_server.h>
#include <esp_http_server.h>
#include <string.h>

esp_err_t getContent(httpd_req_t *req, char *buffer, size_t buffer_size) {
    int total_len = req->content_len;
    int cur_len = 0;

    // Перевіряємо, чи поміститься тіло запиту в буфер (залишаємо 1 байт для '\0')
    if (total_len >= buffer_size) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
        return ESP_FAIL;
    }

    // Якщо тіло запиту порожнє
    if (total_len == 0) {
        buffer[0] = '\0';
        return ESP_OK;
    }

    // Зчитуємо дані
    cur_len = httpd_req_recv(req, buffer, total_len);
    if (cur_len <= 0) {
        // Якщо стався таймаут
        if (cur_len == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    // Завершуємо рядок нуль-символом
    buffer[cur_len] = '\0';
    
    return ESP_OK;
}

esp_err_t getPostParamInt(const char *content, const char *key, int *dest) {
    char param_buf[32];

    if (httpd_query_key_value(content, key, param_buf, sizeof(param_buf)) == ESP_OK) {
        *dest = atoi(param_buf);
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t getPostParam(const char *content, const char *key, char *dest, size_t dest_size) {
    if (!content || !key || !dest || dest_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Буфер для сирих даних. 
    // Робимо його розмір рівним dest_size, оскільки зашифрований (URL-encoded) 
    // рядок завжди довший або такий самий, як розкодований.
    char raw_buf[dest_size]; 

    // Шукаємо параметр за ключем
    if (httpd_query_key_value(content, key, raw_buf, sizeof(raw_buf)) == ESP_OK) {
        char decoded_buf[dest_size];
        
        // Декодуємо значення
        url_decode(decoded_buf, raw_buf);
        
        // Безпечно копіюємо результат у цільовий буфер
        strlcpy(dest, decoded_buf, dest_size);
        
        return ESP_OK;
    }

    // Якщо ключ не знайдено
    return ESP_ERR_NOT_FOUND;
}


esp_err_t getQueryString(httpd_req_t *req, char *buffer, size_t buffer_size) {
    // Отримуємо довжину GET-параметрів. Додаємо +1 для нуль-символу '\0'
    size_t query_len = httpd_req_get_url_query_len(req) + 1;

    // Якщо параметри є (довжина більше 1, бо 1 — це лише '\0')
    if (query_len > 1) {
        // Перевіряємо, чи влізе запит у наш буфер
        if (query_len > buffer_size) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Query too long");
            return ESP_FAIL;
        }
        
        // Зчитуємо рядок у буфер
        if (httpd_req_get_url_query_str(req, buffer, query_len) == ESP_OK) {
            return ESP_OK;
        }
    }

    // Якщо параметрів в URL немає (наприклад, просто звернулися до "/")
    buffer[0] = '\0';
    return ESP_ERR_NOT_FOUND;
}

esp_err_t getGetParamInt(const char *data, const char *key, int *dest) {
    char param_buf[32];
    if (httpd_query_key_value(data, key, param_buf, sizeof(param_buf)) == ESP_OK) {
        *dest = atoi(param_buf);
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t getGetParam(const char *data, const char *key, char *dest, size_t dest_size) {
    if (!data || !key || !dest || dest_size == 0) return ESP_ERR_INVALID_ARG;

    char raw_buf[dest_size]; 
    if (httpd_query_key_value(data, key, raw_buf, sizeof(raw_buf)) == ESP_OK) {
        char decoded_buf[dest_size];
        url_decode(decoded_buf, raw_buf);
        strlcpy(dest, decoded_buf, dest_size);
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}

// ################## так працюємо з get параметрами ######################
// esp_err_t get_handler(httpd_req_t *req) {
//     char query[256];

//     // Отримуємо рядок параметрів з URL
//     if (getQueryString(req, query, sizeof(query)) == ESP_OK) {
        
//         // Зчитуємо текст (наприклад, ім'я)
//         char deviceName[64];
//         if (getParam(query, "name", deviceName, sizeof(deviceName)) == ESP_OK) {
//             printf("Отримано ім'я: %s\n", deviceName);
//         }

//         // Зчитуємо число (наприклад, ШІМ або режим)
//         int pwmValue = 0;
//         if (getParamInt(query, "val", &pwmValue) == ESP_OK) {
//             printf("Отримано значення: %d\n", pwmValue);
//         }
//     }

//     httpd_resp_sendstr(req, "GET OK");
//     return ESP_OK;
// }


// ############## Перевіряємо, чи є параметри в URL-запиті для GET (старий код) #############################
// char query[256];
// if (httpd_req_get_url_query_len(req) < sizeof(query)) {
//     if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
//         char param_buf[64];

//         // 1. Зчитуємо параметр 'name'
//         if (httpd_query_key_value(query, "name", param_buf, sizeof(param_buf)) == ESP_OK) {
//             // Можна додати базову заміну знаків %20 (пробіли), якщо браузер їх кодує
//             // snprintf(deviceName, sizeof(deviceName), "%s", param_buf);
//             strlcpy(deviceName, param_buf, sizeof(deviceName));
//         }

//         // 2. Зчитуємо параметр 'val'
//         if (httpd_query_key_value(query, "val", param_buf, sizeof(param_buf)) == ESP_OK) {
//             pwmValue = atoi(param_buf);
//         }

//         // 3. Зчитуємо параметр 'mode'
//         if (httpd_query_key_value(query, "mode", param_buf, sizeof(param_buf)) == ESP_OK) {
//             //snprintf(modeSelection, sizeof(modeSelection), "%s", param_buf);
//             strlcpy(modeSelection, param_buf, sizeof(modeSelection));
//         }

//         // 1. Зчитуємо параметр 'cmd'
//         if (httpd_query_key_value(query, "cmd", param_buf, sizeof(param_buf)) == ESP_OK) {
//             // Можна додати базову заміну знаків %20 (пробіли), якщо браузер їх кодує
//             // snprintf(deviceName, sizeof(deviceName), "%s", param_buf);
//             strlcpy(deviceCMD, param_buf, sizeof(deviceCMD));


//             deviceCMD[strcspn(deviceCMD, "\r\n")] = 0;

//             if (strncmp(deviceCMD, "SENSOR_PERIOD ", 14) == 0) {
//                 handle_period_command(SENSOR_PERIOD, deviceCMD, 14, &sensor_period_ms, 20, 500, "Sensor");
//             } 
//             else if (strncmp(deviceCMD, "DISPLAY_PERIOD ",
//                  15) == 0) {
//                 handle_period_command(DISPLAY_PERIOD, deviceCMD, 15, &display_period_ms, 100, 30000, "Display");
//             } 
//             else {
//                 printf("ACK: Unknown command -> %s\n", deviceCMD);
//                 fflush(stdout);
//             }
//         }
//     }
// }