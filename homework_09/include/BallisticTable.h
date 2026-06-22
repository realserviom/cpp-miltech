#pragma once

#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>

struct BallisticTable {
    
    // ==========================================
    // Вкладені структури даних (Деталі реалізації)
    // ==========================================
    
    // Результат в кожному вузлі сітки
    struct Result {
        float t;      // час польоту
        float hDist;  // горизонтальна дистанція
    };

    // Індекс і коефіцієнт для одного виміру
    struct Interp {
        int lo;       // нижній індекс в осі
        float frac;   // коефіцієнт зміщення [0..1]
    };

    // ==========================================
    // Осі (Вузли сітки) та плоский масив даних
    // ==========================================
    
    // 5 осей — кожна зі своїм набором вузлів (можливий нерівномірний крок)
    std::vector<float> axisZ0;  // висота
    std::vector<float> axisV0;  // швидкість
    std::vector<float> axisM;   // маса
    std::vector<float> axisD;   // опір (drag)
    std::vector<float> axisL;   // підйомна сила (lift)

    // Плоский масив розміром |Z0| * |V0| * |M| * |D| * |L|
    std::vector<Result> data;

    // ==========================================
    // Методи доступу та індексації
    // ==========================================

    // Індекс у плоскому масиві: [iZ0][iV0][iM][iD][iL]
    size_t index(int iz, int iv, int im, int id, int il) const {
        return ((((size_t)iz * axisV0.size() + iv)
                              * axisM.size()  + im)
                              * axisD.size()  + id)
                              * axisL.size()  + il;
    }

    // Безпечний доступ до конкретного вузла сітки
    const Result& at(int iz, int iv, int im, int id, int il) const {
        return data[index(iz, iv, im, id, il)];
    }

    // ==========================================
    // Статичні математичні хелпери
    // ==========================================

    // Лінійна інтерполяція для Result (обидва поля паралельно)
    static Result lerp(const Result& a, const Result& b, float t) {
        return {
            a.t      + (b.t      - a.t)      * t,
            a.hDist + (b.hDist - a.hDist) * t
        };
    }

    // Пошук індексу та коефіцієнта зміщення для однієї осі
    static Interp findInterp(float val, const std::vector<float>& axis) {
        // Захист: якщо осі порожні (таблиця не завантажена)
        if (axis.empty()) return {0, 0.0f};

        // Захист за нижньою межею (Clamp)
        if (val <= axis.front()) return {0, 0.0f};
        
        // Захист за верхньою межею (Clamp)
        if (val >= axis.back()) {
            return {static_cast<int>(axis.size()) - 2, 1.0f};
        }

        // Бінарний пошук першого елемента, який більший або дорівнює val
        auto it = std::lower_bound(axis.begin(), axis.end(), val);
        int i = static_cast<int>(it - axis.begin()) - 1;
        if (i < 0) i = 0;

        // Обчислення коефіцієнта пропорції між вузлами i та i+1
        float frac = (val - axis[i]) / (axis[i + 1] - axis[i]);
        return {i, frac};
    }

    // ==========================================
    // Головні методи: Завантаження та Пошук
    // ==========================================

    // Багатовимірна лінійна інтерполяція по 5 параметрах (32 -> 16 -> 8 -> 4 -> 2 -> 1)
    Result lookup(float Z0, float V0, float m, float d, float l) const {
        // Шукаємо інтервали та ваги для кожної осі окремо
        Interp iz = findInterp(Z0, axisZ0);
        Interp iv = findInterp(V0, axisV0);
        Interp im = findInterp(m,  axisM);
        Interp id = findInterp(d,  axisD);
        Interp il = findInterp(l,  axisL);

        // 1. Вісь l: згортаємо 32 вершини гіперкуба в 16 точок
        Result v[16];
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                for (int c = 0; c < 2; c++) {
                    for (int e = 0; e < 2; e++) {
                        const auto& lo = at(iz.lo + a, iv.lo + b, im.lo + c, id.lo + e, il.lo);
                        const auto& hi = at(iz.lo + a, iv.lo + b, im.lo + c, id.lo + e, il.lo + 1);
                        v[a * 8 + b * 4 + c * 2 + e] = lerp(lo, hi, il.frac);
                    }
                }
            }
        }

        // 2. Вісь d: згортаємо 16 точок у 8
        Result w[8];
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                for (int c = 0; c < 2; c++) {
                    w[a * 4 + b * 2 + c] = lerp(v[a * 8 + b * 4 + c * 2], 
                                                v[a * 8 + b * 4 + c * 2 + 1], 
                                                id.frac);
                }
            }
        }

        // 3. Вісь m: згортаємо 8 точок у 4
        Result u[4];
        for (int a = 0; a < 2; a++) {
            for (int b = 0; b < 2; b++) {
                u[a * 2 + b] = lerp(w[a * 4 + b * 2], 
                                    w[a * 4 + b * 2 + 1], 
                                    im.frac);
            }
        }

        // 4. Вісь V0: згортаємо 4 точки у 2
        Result s[2];
        for (int a = 0; a < 2; a++) {
            s[a] = lerp(u[a * 2], u[a * 2 + 1], iv.frac);
        }

        // 5. Вісь Z0: фінальний lerp (з 2 точок в 1)
        return lerp(s[0], s[1], iz.frac);
    }

    // Завантаження таблиці з текстового файлу
    bool load(const char* path) {
        std::ifstream f(path);
        if (!f.is_open()) {
          throw std::runtime_error("[BalliscticTable] Critical Error: Failed to load table from " + std::string(path));
        }

        int nZ, nV, nM, nD, nL;
        if (!(f >> nZ >> nV >> nM >> nD >> nL)) return false;

        axisZ0.resize(nZ); for (auto& v : axisZ0) f >> v;
        axisV0.resize(nV); for (auto& v : axisV0) f >> v;
        axisM.resize(nM);  for (auto& v : axisM)  f >> v;
        axisD.resize(nD);  for (auto& v : axisD)  f >> v;
        axisL.resize(nL);  for (auto& v : axisL)  f >> v;

        size_t total = (size_t)nZ * nV * nM * nD * nL;
        data.resize(total);

        // Порядок у файлі: Z0 → V0 → m → d → l (від зовнішнього до внутрішнього циклу)
        for (size_t i = 0; i < total; i++) {
            if (!(f >> data[i].t >> data[i].hDist)) {
                return false;
            }
        }

        return f.good();
    }
};