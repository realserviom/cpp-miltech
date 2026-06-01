#pragma once
#ifndef TYPES_H
#define TYPES_H
#include <stdlib.h>
#include <string>

// імя боєприпасу

using def_ammoName = std::string;

struct AmmoParams {
  float mass;  // маса (кг)
  float drag;  // коефіцієнт опору
  float lift;  // коефіцієнт підйому
};

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

struct DroneConfig {
    Coord startPos;         // початкова позиція (x, y)
    float altitude;         // висота
    float initialDir;       // початковий напрямок (рад)
    float attackSpeed;      // швидкість атаки (м/с)
    float accelPath;        // шлях розгону (м)
    def_ammoName ammoName;     // обрані боєприпаси
    float arrayTimeStep;    // крок часу масиву цілей
    float simTimeStep;      // крок симуляції
    float hitRadius;        // радіус влучення
    float angularSpeed;     // кутова швидкість (рад/с)
    float turnThreshold;    // поріг повороту (рад)


    // величина оберту дрона в радіанах за одну ітерацію
    float radInIteration;

    // час зупинки чи прискорення дрона згідно формули t = 2*S/v
    float timeAcceleration;

    // прискорення дрона згідно формули a = v/t
    float acceleration;

    // похибка в часі для зони ураження
    float timeHitRadius;

    // Викликаємо після того, як заповнили структуру з JSON
    void updateCalculatedParams() {
        radInIteration = angularSpeed * simTimeStep;
        timeAcceleration = (2.0f * accelPath) / attackSpeed;
        acceleration = attackSpeed / timeAcceleration;
        timeHitRadius = hitRadius / attackSpeed;
    }
};

struct SimStep {
    Coord pos;          // позиція дрона
    float direction;    // напрямок (рад)
    int   state;        // стан дрона (0-4)
    int   targetIdx;    // індекс поточної цілі
    Coord dropPoint;    	// точка скиду (куди летить дрон)
	Coord aimPoint;     	// куди впаде бомба (якщо скинути зараз)
	Coord predictedTarget;  // прогнозована позиція цілі
};

enum class ProviderType { 
    JSON 
};

enum class LoaderType { 
    FILE 
};

enum class SolverType {
    ANALYTICAL
};


#endif