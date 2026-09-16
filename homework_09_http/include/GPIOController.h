#pragma once

#ifdef REAL_GPIO
// Наперед оголошуємо структури, щоб не тягнути gpiod.h у загальні заголовки
struct gpiod_chip;
struct gpiod_line;
#endif

class GPIOController {
private:
    bool is_ready = false;
#ifdef REAL_GPIO
    struct gpiod_chip* chip = nullptr;
    struct gpiod_line* start_line = nullptr;
    struct gpiod_line* drop_line = nullptr;
#endif

public:
    GPIOController();
    ~GPIOController();

    void set_start(int value);
    void pulse_drop();
};