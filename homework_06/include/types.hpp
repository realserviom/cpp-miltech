#include <stdlib.h>
#include <array>

#ifndef TYPES_HPP
#define TYPES_HPP

const double kEpsilon = 1e-9;  // або інша точність, яка вам потрібна
const double kGravit = 9.81;   // Стандартне значення в м/с²
const int kCountNameAmmo = 50;
const int kCountTypeAmmo = 30;

using def_name_ammo = std::array<char, kCountNameAmmo>;
using def_type_ammo = std::array<char, kCountTypeAmmo>;

struct Ammunition {
  def_name_ammo name_;  // Назва
  float m_;             // m_ (кг)
  float d_;             // d_ (drag)
  float l_;             // l_ (lift)
  def_type_ammo type_;  // Тип (Вільне падіння / Планеруючий)
};

constexpr size_t kAmmoTableSize = 6;

inline const std::array<Ammunition, kAmmoTableSize> kAmmoTable = {{{{"VOG-17"}, 0.35F, 0.07F, 0.0F, {"Вільний"}},
                                                                   {{"M67"}, 0.6F, 0.10F, 0.0F, {"Вільний"}},
                                                                   {{"RKG-3"}, 1.2F, 0.10F, 0.0F, {"Вільний"}},
                                                                   {{"GLIDING-VOG"}, 0.45F, 0.10F, 1.0F, {"Планеруючий"}},
                                                                   {{"GLIDING-RKG"}, 1.4F, 0.10F, 1.0F, {"Планеруючий"}},
                                                                   {{"TEST-AMMO"}, 1.4F, 0.10F, 1.0F, {"Планеруючий"}}}};

// Структрура для повернення зчитаних даних
struct DroneInput {
  float xd_;
  float yd_;
  float zd_;
  float target_x_;
  float target_y_;
  float v0_;
  float acceleration_path_;
  def_name_ammo name_ammo_;
};

// Структура для повернення результатів
struct DropPoints {
  double fire_x_{};
  double fire_y_{};
  double xd_i_{};
  double yd_i_{};
  bool is_middle_point_ = false;
};

#endif