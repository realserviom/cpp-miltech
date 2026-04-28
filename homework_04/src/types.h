#ifndef TYPES_H
#define TYPES_H
#include <cmath>
#include <iostream>


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
 

typedef struct {
    // iмпульсiв на один оберт колеса  
    int ticks_per_revolution;

    // радiус колеса у метрах (дiаметр 60 см)
    float wheel_radius_m;

    // вiдстань мiж лiвим i правим бортом, у метрах
    float wheelbase_m;
} NrkConfig;


struct Coord {
    double x;
    double y;

    // Додавання координат
	Coord operator+(const Coord& other) const {
    	Coord result;
        result.x = x + other.x;
        result.y = y + other.y;
        return result;
	}
 
	// Віднімання координат
	Coord operator-(const Coord& other) const {
    	Coord result;
        result.x = x - other.x;
        result.y = y - other.y;
        return result;
	}
 
	// Множення на скаляр
	Coord operator*(float s) const {
    	Coord result;
        result.x = x * s;
        result.y = y * s;
        return result;
	}

    // Ділення на скаляр
    Coord operator/(float s) const {
        if (std::abs(s) < 1e-6f) return {0, 0};
        return { x / s, y / s };
    }

    // Оператор порівняння
    bool operator==(const Coord& other) const {
        // Визначаємо точність (5 знаків після коми)
        const double eps = 1e-6f; 
            
        // Перевіряємо, чи різниця по обох осях в межах норми
        return (std::abs(x - other.x) < eps) && (std::abs(y - other.y) < eps);
    }

};

#endif