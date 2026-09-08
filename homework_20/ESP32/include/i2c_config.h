#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

#include "driver/i2c_master.h"
#include "esp_log.h"

// Піни та адреси (можна винести сюди або залишити в main.c)
#define K_SDA 8
#define K_SCL 9
#define K_MPU_ADDR 0x68
#define K_OLED_ADDR 0x3C

// Оголошуємо змінні через extern, щоб вони були доступні усім, хто підключить цей файл
extern i2c_master_dev_handle_t mpu;
extern i2c_master_dev_handle_t oled;

// Функція ініціалізації I2C, MPU та OLED
static inline void i2cInit(void) {
    i2c_master_bus_config_t bus = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = (gpio_num_t)K_SDA,
        .scl_io_num = (gpio_num_t)K_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    
    i2c_master_bus_handle_t busH;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus, &busH));

    i2c_device_config_t dev = {
        .scl_speed_hz = 400000,
        .device_address = K_MPU_ADDR,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(busH, &dev, &mpu));
    
    dev.device_address = K_OLED_ADDR;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(busH, &dev, &oled));
    
    printf("I2C bus and devices initialized successfully\n");
}

#endif // I2C_CONFIG_H