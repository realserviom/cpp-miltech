#include "GPIOController.h"
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef REAL_GPIO
#include <gpiod.h>
#include <unistd.h>
#else
#include <unistd.h> // для usleep в mock-режимі
#endif

namespace fs = std::filesystem;

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
    std::string dirPath = "/tmp/my_gpio_bank/sim_gpio24";
    std::string filePath = dirPath + "/value";

    try {
      fs::create_directories(dirPath);
      std::ofstream file(filePath);
      if (file.is_open()) {
        file << "1";
        file.close();
        std::cout << "Дані у файл про старт успішно записано!" << std::endl;
      }
      else {
        std::cerr << "Не вдалося відкрити файл для запису!" << std::endl;
      }
    }
    catch (const fs::filesystem_error& e) {
      std::cerr << "Помилка файлової системи: " << e.what() << std::endl;
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

    std::cout << "[MOCK GPIO] Лінія DROP -> ГОТОВИЙ" << std::endl;
    std::string dirPath = "/tmp/my_gpio_bank/sim_gpio23";
    std::string filePath = dirPath + "/value";

    try {
      fs::create_directories(dirPath);
      std::ofstream file(filePath);
      if (file.is_open()) {
        file << "1";
        file.flush();
        usleep(80000);
        // Переміщуємо курсор запису на початок файлу (на нульову позицію)
        file.seekp(0);
        file << "0";  // Запише "0" поверх "1"
        file.close();
      }
      else {
        std::cerr << "Не вдалося відкрити файл для запису!" << std::endl;
      }
    }
    catch (const fs::filesystem_error& e) {
      std::cerr << "Помилка файлової системи: " << e.what() << std::endl;
    }
#endif
}