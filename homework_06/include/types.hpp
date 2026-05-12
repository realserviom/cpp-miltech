
#ifndef TYPES_HPP
#define TYPES_HPP

typedef char def_name_ammo[50];
typedef char def_type_ammo[30];

typedef struct {
  def_name_ammo name;  // Назва
  float m;             // m (кг)
  float d;             // d (drag)
  float l;             // l (lift)
  def_type_ammo type;  // Тип (Вільне падіння / Планеруючий)
} Ammunition;

inline const Ammunition ammoTable[] = {{"VOG-17", 0.35, 0.07, 0.0, "Вільний"},
                                       {"M67", 0.6, 0.10, 0.0, "Вільний"},
                                       {"RKG-3", 1.2, 0.10, 0.0, "Вільний"},
                                       {"GLIDING-VOG", 0.45, 0.10, 1.0, "Планеруючий"},
                                       {"GLIDING-RKG", 1.4, 0.10, 1.0, "Планеруючий"}};

// Структрура для повернення зчитаних даних
struct DroneInput {
  float xd, yd, zd;
  float targetX, targetY;
  float V0;
  float accelerationPath;
  def_name_ammo name_ammo;
};

#endif