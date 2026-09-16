// drone_link.h — бінарний UART-протокол ДЗ11 (заняття 4.4)
// Спільний для симулятора фізики, чекера і програми студента.
//
// Кадр на дроті:
//   [0]  MAGIC0 = 0xA5
//   [1]  MAGIC1 = 0x5A
//   [2]  TYPE   (1 байт)        — тип пакета (PacketType)
//   [3]  LEN    (1 байт)        — довжина payload у байтах
//   [4..4+LEN-1]  payload       — корисні дані (little-endian)
//   [..] CRC16  (2 байти, LE)   — CRC-16/CCITT-FALSE по TYPE+LEN+payload
//
// Усі multi-byte поля — little-endian (як на ARM/x86 за замовчуванням).
// Кадр самосинхронізується: приймач шукає MAGIC0,MAGIC1, потім читає LEN і CRC.


#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdint.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"


#define MAGIC0 0xA5
#define MAGIC1 0x5A
#define FLASH_USER_START_ADDR ADDR_FLASH_SECTOR_7


// Створюємо назви для switch
typedef enum {
    UART_PROFILE_SIMPLE,
    UART_PROFILE_ADVANCED
} uart_profile_name_t;

// Виносимо характеристики як константи
static const uart_config_t UART_CONFIG_SIMPLE = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};

static const uart_config_t UART_CONFIG_ADVANCED = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};



void init_uart(uart_profile_name_t name, uart_port_t uart_num, gpio_num_t tx_pin, gpio_num_t rx_pin) {
    const uart_config_t* selected_config = NULL;

    switch (name) {
        case UART_PROFILE_SIMPLE:
            selected_config = &UART_CONFIG_SIMPLE;
            break;
        case UART_PROFILE_ADVANCED:
            selected_config = &UART_CONFIG_ADVANCED;
            break;
        default:
            ESP_LOGE("UART", "Unknown profile name!");
            return;
    }

    uart_param_config(uart_num, selected_config);
    uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(uart_num, 1024, 0, 0, NULL, 0);
}

void init_simple_uart(uart_port_t uart_num, gpio_num_t tx_pin, gpio_num_t rx_pin) {
    init_uart(UART_PROFILE_SIMPLE, uart_num, tx_pin, rx_pin);
}

// ---- CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) ----
static inline uint16_t crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; ++b)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
    return crc;
}

enum PacketType : uint8_t {
    // відповідь строкою
    PKT_ANSWER    = 0x01,
    // тип пакету від і до STM32 з даним
    PKT_DATA = 0x02,
    // тип пакету до STM32 для отримання конфігурації
    PKT_GET_DATA = 0x03 
};


#pragma pack(push, 1)

// PKT_ANSWER — відповідь (зворотний канал на залізі)
struct Answer {
    char msg[100];
};

// PKT_DATA — конфігурація що передається на STM
struct Data {
    char name[20]; 
    uint8_t val;
    char mode[20]; 
};

#pragma pack(pop)


// Запакувати кадр у буфер out (повертає кількість записаних байтів).
// out має бути >= 6 + payloadLen.
static inline size_t encode(uint8_t type, const void* payload, uint8_t payloadLen,
                     uint8_t* out) {
    out[0] = MAGIC0;
    out[1] = MAGIC1;
    out[2] = type;
    out[3] = payloadLen;
    if (payloadLen && payload) memcpy(out + 4, payload, payloadLen);
    uint16_t c = crc16(out + 2, (size_t)payloadLen + 2);  // по TYPE+LEN+payload
    out[4 + payloadLen]     = (uint8_t)(c & 0xFF);
    out[4 + payloadLen + 1] = (uint8_t)(c >> 8);
    return (size_t)payloadLen + 6;
}

void sendConfigRequest()
{
    printf("sendConfigRequest\n\r");
    uint8_t out[128]; 
    // Передаємо NULL та 0, оскільки корисного навантаження (payload) немає
    size_t m = encode(PKT_GET_DATA, NULL, 0, out);
    uart_write_bytes(UART_NUM, (const char *)out, m);
}

void sendAnswer(const char *msg)
{
    struct Answer a;
    snprintf(a.msg, sizeof(a.msg), "%s", msg);
    
    uint8_t out[128]; 
    
    size_t m = encode(PKT_ANSWER, (const uint8_t *)&a, sizeof(a), out);
    uart_write_bytes(UART_NUM, (const char *)out, m);
}

void sendData(const char *name_str, uint8_t val_num, const char *mode_str)
{
    struct Data data;
    
    snprintf(data.name, sizeof(data.name), "%s", name_str);
    data.val = val_num;
    snprintf(data.mode, sizeof(data.mode), "%s", mode_str);

    uint8_t out[64];
    size_t m = encode(PKT_DATA, (const uint8_t *)&data, sizeof(data), out);
    uart_write_bytes(UART_NUM, (const char *)out, m);
}

// Стан парсера
typedef enum { 
    S_M0, 
    S_M1, 
    S_TYPE, 
    S_LEN, 
    S_PAYLOAD, 
    S_CRC0, 
    S_CRC1 
} parser_state_t;

// Структура даних парсера
typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t idx;
    uint8_t buf[260];
    uint16_t crc_rx;
    parser_state_t st;
} uart_parser_t;


// Ініціалізація парсера
static inline void parser_init(uart_parser_t *p) {
    p->st = S_M0;
    p->idx = 0;
    p->len = 0;
    p->type = 0;
}

// Функція обробки байта (аналог feed)
static bool parser_feed(uart_parser_t *p, uint8_t byte, uint8_t *outType, uint8_t *outPayload, uint8_t *outLen) {
    switch (p->st) {
        case S_M0:
            if (byte == MAGIC0)
                p->st = S_M1;
            break;
        case S_M1:
            p->st = (byte == MAGIC1) ? S_TYPE : S_M0;
            break;
        case S_TYPE:
            p->type = byte;
            p->st = S_LEN;
            break;
        case S_LEN:
            p->len = byte;
            p->idx = 0;
            p->st = p->len ? S_PAYLOAD : S_CRC0;
            break;
        case S_PAYLOAD:
            p->buf[p->idx++] = byte;
            if (p->idx >= p->len)
                p->st = S_CRC0;
            break;
        case S_CRC0:
            p->crc_rx = byte;
            p->st = S_CRC1;
            break;
        case S_CRC1: {
            p->crc_rx |= (uint16_t)byte << 8;
            p->st = S_M0;
            
            uint8_t tmp[262];
            tmp[0] = p->type;
            tmp[1] = p->len;
            memcpy(tmp + 2, p->buf, p->len);
            
            if (crc16(tmp, (size_t)p->len + 2) == p->crc_rx) {
                *outType = p->type;
                *outLen = p->len;
                memcpy(outPayload, p->buf, p->len);
                return true;
            }
            break;
        }
    }
    return false;
}


#endif // UART_LINK_H
