#ifndef UART_LINK_H
#define UART_LINK_H

#include "main.h"
#include "usart.h" // Для доступу до huart1
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MAGIC0 0xA5
#define MAGIC1 0x5A

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
    PKT_ANSWER    = 0x01,  // відповідь строкою
    PKT_FOR_STM   = 0x03   // передача пакета для STM
};

#pragma pack(push, 1)

// PKT_ANSWER
struct Answer {
    char msg[100];
};

// PKT_FOR_STM
struct For_stm {
    char name[20]; 
    uint8_t val;
    char mode[20]; 
};

#pragma pack(pop)

// Запакувати кадр у буфер out
static inline size_t encode(uint8_t type, const void* payload, uint8_t payloadLen, uint8_t* out) {
    out[0] = MAGIC0;
    out[1] = MAGIC1;
    out[2] = type;
    out[3] = payloadLen;
    if (payloadLen && payload) memcpy(out + 4, payload, payloadLen);
    uint16_t c = crc16(out + 2, (size_t)payloadLen + 2);
    out[4 + payloadLen]     = (uint8_t)(c & 0xFF);
    out[4 + payloadLen + 1] = (uint8_t)(c >> 8);
    return (size_t)payloadLen + 6;
}

// Стан парсера
typedef enum { 
    S_M0, S_M1, S_TYPE, S_LEN, S_PAYLOAD, S_CRC0, S_CRC1 
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

// Функція обробки байта
static bool parser_feed(uart_parser_t *p, uint8_t byte, uint8_t *outType, uint8_t *outPayload, uint8_t *outLen) {
    switch (p->st) {
        case S_M0:
            if (byte == MAGIC0) p->st = S_M1;
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
            if (p->idx >= p->len) p->st = S_CRC0;
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


// Оголошуємо що функція живе у freertos.c
extern void uartSend(const uint8_t* data, size_t len);

// Функція формування та відправки відповіді
static inline void sendAnswer(const char *msg) {
    struct Answer a;
    snprintf(a.msg, sizeof(a.msg), "%s", msg);
    
    uint8_t out[128]; 
    size_t m = encode(PKT_ANSWER, (const uint8_t *)&a, sizeof(a), out);
    
    uartSend(out, m);
}

#endif // UART_LINK_H