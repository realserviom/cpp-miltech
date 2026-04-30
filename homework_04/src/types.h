#ifndef TYPES_H
#define TYPES_H
#pragma once
#include <cmath>
#include <iostream>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


struct NrkStep {
    int timestamp_ms;     // час
    int fl_ticks;         // накопичувальнi iмпульси переднього лівого
    int fr_ticks;         // накопичувальнi iмпульси переднього правого
    int bl_ticks;         // накопичувальнi iмпульси заднього лівого 
    int br_ticks;         // накопичувальнi iмпульси заднього правого 

    // для виводу dbg(steps)
    friend std::ostream& operator<<(std::ostream& os, const NrkStep& s) {
        return os << "Step[t: " << s.timestamp_ms 
                  << ", fl: " << s.fl_ticks
                  << ", fr: " << s.fr_ticks
                  << ", bl: " << s.bl_ticks 
                  << ", br: " << s.br_ticks << "]";
    }

};
 

struct NrkConfig {
    // iмпульсiв на один оберт колеса  
    int ticks_per_revolution;

    // радiус колеса у метрах (дiаметр 60 см)
    float wheel_radius_m;

    // вiдстань мiж лiвим i правим бортом, у метрах
    float wheelbase_m;

    // дистанція в метрах за один імпульс на колесо
    double distance_per_tick;

    // Викликаємо після того, як заповнили структуру
    void updateCalculatedParams() {
        distance_per_tick = 2.0 * M_PI * wheel_radius_m / ticks_per_revolution;
    }

};



struct Coord {
    double x;
    double y;

    // Додавання координат
    // поки що не використовуємо 
	Coord operator+(const Coord& other) const {
    	Coord result;
        result.x = x + other.x;
        result.y = y + other.y;
        return result;
	}
 
};

struct position {
    int timestamp_ms; // час в мілісекундах
    Coord pos;        // позиція нрк
    double theta;
};

#endif