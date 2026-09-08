#ifndef OLED_H
#define OLED_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_log.h"


// Статичний буфер екрана для цього заголовочного файлу
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

#endif // OLED_H
