#pragma once
#include "types.h"
#include <cmath>

class Drone {
public:
    Coord pos;               // поточна позиція (x, y)
    float speed;             // поточна швидкість
    float angularState;      // поточне положення дрона відносно осі x
    DroneState state;        // поточний стан
    int target;              // поточна ціль
    Coord dropPoint;         // точка скиду
    Coord aimPoint;          // куди впаде бомба (якщо скинути зараз)
    Coord predictedTarget;   // прогнозована позиція цілі

    DroneConfig config; 

    explicit Drone(const DroneConfig& config) 
        : config(config) 
    {
        pos.x = config.startPos.x;
        pos.y = config.startPos.y;
        speed = 0.0f;
        angularState = config.initialDir;
        state = STOPPED;
        target = 0;
        dropPoint = {0, 0};
        aimPoint = {0, 0};
        predictedTarget = {0, 0};
    }

    // Оновлений метод обертання
    bool updateRotation(float targetAngle, float turnThreshold = 0.0f) {
        // якщо швидкість повороту в радіанах велика 
        // тобто за одну ітерацію більша ніж різниця положень тоді проставляємо зразу вірний кут
        if(std::abs(targetAngle - angularState) < config.radInIteration) {
            angularState = targetAngle;
            return false;
        }
        
        if((targetAngle - angularState) > turnThreshold) {
            // крутимо проти годинникової стрілки
            angularState += config.radInIteration;
            return true;
            
        } else if((angularState - targetAngle) > turnThreshold) {
            // крутимо за годинниковою стрілою
            angularState -= config.radInIteration;
            return true;
        }
        
        return false;
    }

    void updatePosition() {
        // Використовуємо внутрішні змінні класу (з префіксом m_)
        Coord direction = { (float)cos(angularState), (float)sin(angularState) };
        
        Coord velocity = direction * speed;
        Coord acceleration = direction * config.acceleration;
        
        float dt = config.simTimeStep;
        float stepSq = (dt * dt) / 2.0f;

        if (state == MOVING) {
            pos = pos + (velocity * dt);
        } 
        else if (state == ACCELERATING) {
            pos = pos + (velocity * dt) + (acceleration * stepSq);
        }
        else if (state == DECELERATING) {
            pos = pos + (velocity * dt) - (acceleration * stepSq);
        }
    }

    bool needRotation(float targetAngle, float turnThreshold = 0.0f) const {
        return std::abs(targetAngle - angularState) > turnThreshold;
    }

    float calculateArrivalTime(float targetAngle, float distance, float distFall) const {
        float timeTurned = (targetAngle - angularState) > config.turnThreshold ? 
        (targetAngle - angularState) / config.angularSpeed : 0;

        return timeTurned + config.timeAcceleration + ((distance - config.accelPath - distFall) / speed);
    }

    float calculateSmallArrivalTime(float distance) const {

        // рівномірний рух
        if(speed == config.attackSpeed) {
            return distance / config.attackSpeed;
        }
        
        // рівноприскорений рух
        // (a*t^2)/2 + Vo*t - S = 0;
        // Розвяжемо квадратне рівняння 
        // Дискримінант D = b^2 - 4ac
        float D = speed * speed + 2.0f *  config.acceleration * distance;

        if (D < 0) return distance / config.attackSpeed; // На всякий випадок, якщо фізично неможливо

        return (-speed + std::sqrt(D)) /  config.acceleration;
    }

    void move(int newTarget, float targetAngle, bool& keyChangeTarget) {
         // State == STOPPED тільки коли стартує
        if (state == STOPPED) {
            // updateRotation повертає true, якщо поворот ще триває
            state = updateRotation(targetAngle, config.turnThreshold) ? TURNING : ACCELERATING;
        } 
        else if (state == DECELERATING) {
            // Зупиняємо дрон до повної зупинки, щоб потім його повернути
            updatePosition();
            speed -= (config.acceleration * config.simTimeStep);
            
            if (speed <= 0) {
                speed = 0;
                state = TURNING;
            }
        } 
        else if (newTarget != target) {
            // Змінюємо ціль
            keyChangeTarget = true;
            target = newTarget; // Оновлюємо внутрішню ціль дрона, щоб не заходити сюди щоразу
            
            // Перевіряємо чи кут напрямку в межах нової цілі
            if (needRotation(targetAngle, config.turnThreshold)) {
                if (state == TURNING) {
                    // Якщо false — ми закінчили поворот і починаємо рух
                    if (!updateRotation(targetAngle, config.turnThreshold)) {
                        state = ACCELERATING;
                    }
                } else {
                    updatePosition();
                    state = DECELERATING;
                    if (speed <= 0) {
                        speed = 0;
                        state = TURNING;
                    }
                }   
            } else {
                state = (speed <= config.attackSpeed) ? ACCELERATING : MOVING;
                updateRotation(targetAngle); // Обертання без порогу
                updatePosition();
                
                // Збільшуємо швидкість
                speed += (config.acceleration * config.simTimeStep);
                if (speed >= config.attackSpeed) {
                    speed = config.attackSpeed; 
                    state = MOVING;
                }
            }
        } 
        else {
            // Звичайний рух до поточної цілі
            if (state == ACCELERATING) {
                // Обертаємо паралельно руху
                updateRotation(targetAngle);
                updatePosition();
                
                speed += (config.acceleration * config.simTimeStep);
                if (speed >= config.attackSpeed) {
                    speed = config.attackSpeed; 
                    state = MOVING;
                }
            } 
            else if (state == TURNING) {
                if (!updateRotation(targetAngle, config.turnThreshold)) {
                    state = ACCELERATING;
                }
            } 
            else if (state == MOVING) {
                // Рівномірний рух з мінімальним обертанням
                updateRotation(targetAngle);
                updatePosition();
            }       
        }
    }
    
};