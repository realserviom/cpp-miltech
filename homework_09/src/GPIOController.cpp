#include "GPIOController.h"
#include <iostream>
#include <fstream>

#ifdef REAL_GPIO
#include <gpiod.h>
#include <unistd.h>
#else
#include <unistd.h> // для usleep в mock-режимі
#endif

GPIOController::GPIOController() {
#ifdef REAL_GPIO
    chip = gpiod_chip_open_by_name("gpiochip4");
    if (!chip) {
        std::perror("[GPIO] Помилка відкриття gpiod_chip");
        return;
    }
    start_line = gpiod_chip_get_line(chip, 17);
    drop_line = gpiod_chip_get_line(chip, 27);

    gpiod_line_request_output(start_line, "drone_start", 0);
    gpiod_line_request_output(drop_line, "drone_drop", 0);
    is_ready = true;
#else
    std::cout << "[MOCK GPIO] Режим імітації. Віртуальний чип створено." << std::endl;
    is_ready = true;
#endif
}

GPIOController::~GPIOController() {
#ifdef REAL_GPIO
    if (chip) {
        gpiod_line_release(start_line);
        gpiod_line_release(drop_line);
        gpiod_chip_close(chip);
    }
#endif
}

void GPIOController::set_start(int value) {
#ifdef REAL_GPIO
    if (is_ready)
        gpiod_line_set_value(start_line, value);
#else
    std::cout << "[MOCK GPIO] Лiнiя START -> " << value << " (" << (value ? "ГОТОВИЙ" : "ВИМК") << ")" << std::endl;
    std::ofstream file("/tmp/my_gpio_bank/sim_gpio24/value");

    if (file.is_open()) {
        file << value;
        file.close();
        std::cout << "[MOCK GPIO] Записано START = " << value << std::endl;
    } else {
        std::cerr << "[MOCK GPIO] Помилка файлу START" << std::endl;
    }
#endif
}

void GPIOController::pulse_drop() {
    std::cout << "[GPIO] Виклик команди DROP!" << std::endl;
#ifdef REAL_GPIO
    if (is_ready)
        gpiod_line_set_value(drop_line, 1);
    usleep(80000); // 80 мс
    if (is_ready)
        gpiod_line_set_value(drop_line, 0);
#else
    std::ofstream file("/tmp/my_gpio_bank/sim_gpio23/value");
    if (file.is_open()) {
        file << 1;
        file.close();
        std::cout << "[MOCK GPIO] DROP -> 1" << std::endl;

        usleep(80000);

        file.open("/tmp/my_gpio_bank/sim_gpio23/value");
        if (file.is_open()) {
            file << 0;
            file.close();
            std::cout << "[MOCK GPIO] DROP -> 0" << std::endl;
        }
    } else {
        std::cerr << "[MOCK GPIO] Помилка файлу DROP" << std::endl;
    }
#endif
}