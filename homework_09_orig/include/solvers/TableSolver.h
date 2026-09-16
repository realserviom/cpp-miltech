#pragma once
#include <string>
#include "Types.h"
#include "../interfaces/IBallisticSolver.h"
#include "BallisticTable.h"

class TableSolver : public IBallisticSolver {
private:
    // Об'єкт нашої 5D LUT-таблиці
    BallisticTable m_table;

    // Сховище для кешу, щоб уникнути повторних пошуків,
    // коли методи часу та дистанції викликаються послідовно
    struct Cache {
        float z = -1.0f;
        float v = -1.0f;
        float m = -1.0f;
        float d = -1.0f;
        float l = -1.0f;
        BallisticTable::Result result{0.0f, 0.0f};
    } m_cache;

    // Внутрішній хелпер для перевірки та оновлення закешованих значень
    BallisticTable::Result getOrUpdateCache(float z, float v, float m, float d, float l);

public:
    // Конструктор, який приймає шлях до запеченого файлу балістики
    TableSolver(const std::string& path);

    // Реалізація інтерфейсних методів
    double getDistDuringFall(float &t_pol, DroneConfig &myDrone, const AmmoParams *ammo) override;
    float getTimePol(const float &d, const float &l, const float &m, const float &zd, const float &attackSpeed) override;
    
    ~TableSolver() override = default;
};