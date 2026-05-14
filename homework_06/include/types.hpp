#include <stdlib.h>
#include <array>

#ifndef TYPES_HPP
#define TYPES_HPP

const double EPSILON = 1e-9;  // або інша точність, яка вам потрібна
const double GRAVIT = 9.81;   // Стандартне значення в м/с²
const int COUNT_NAME_AMMO = 50;
const int COUNT_TYPE_AMMO = 30;

using def_name_ammo = std::array<char, COUNT_NAME_AMMO>;
using def_type_ammo = std::array<char, COUNT_TYPE_AMMO>;

struct Ammunition {
  def_name_ammo name;  // Назва
  float m;             // m (кг)
  float d;             // d (drag)
  float l;             // l (lift)
  def_type_ammo type;  // Тип (Вільне падіння / Планеруючий)
};

constexpr size_t AMMO_TABLE_SIZE = 6;

inline const std::array<Ammunition, AMMO_TABLE_SIZE> ammoTable = {{{{"VOG-17"}, 0.35f, 0.07f, 0.0f, {"Вільний"}},
                                                                   {{"M67"}, 0.6f, 0.10f, 0.0f, {"Вільний"}},
                                                                   {{"RKG-3"}, 1.2f, 0.10f, 0.0f, {"Вільний"}},
                                                                   {{"GLIDING-VOG"}, 0.45f, 0.10f, 1.0f, {"Планеруючий"}},
                                                                   {{"GLIDING-RKG"}, 1.4f, 0.10f, 1.0f, {"Планеруючий"}},
                                                                   {{"TEST-AMMO"}, 1.4f, 0.10f, 1.0f, {"Планеруючий"}}}};

// Структрура для повернення зчитаних даних
struct DroneInput {
  float xd, yd, zd;
  float targetX, targetY;
  float V0;
  float accelerationPath;
  def_name_ammo name_ammo;
};

#endif