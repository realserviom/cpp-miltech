#pragma once

#include <cmath>
#include <cstdint>

// Вхідні дані
struct Target {
  double distance;   // метри
  double azimuth;    // радіани
  double elevation;  // радіани
  bool locked;
};

enum class ActuatorStatus { READY, RELOADING, ERROR };

// Вихідні дані
struct GimbalCommand {
  double target_azimuth;
  double target_elevation;
};

struct ServoCommand {
  bool trigger_fire;
};

enum class TurretStatus { IDLE, TRACKING, TRIGGER_RELOADING, TRIGGER_READY, ERROR };

// Клас логіки
class TurretController {
public:
  struct Output {
    GimbalCommand gimbal;
    ServoCommand servo;
    TurretStatus status;
  };

  static Output calculate(const Target& target, ActuatorStatus actuator)
  {
    Output out;

    // Логіка статусу
    if (actuator == ActuatorStatus::ERROR) {
      out.status = TurretStatus::ERROR;
    }
    else if (actuator == ActuatorStatus::RELOADING) {
      out.status = TurretStatus::TRIGGER_RELOADING;
    }
    else if (target.locked && target.distance < 50.0) {
      out.status = TurretStatus::TRIGGER_READY;
    }
    else if (target.locked) {
      out.status = TurretStatus::TRACKING;
    }
    else {
      out.status = TurretStatus::IDLE;
    }

    // Логіка керування гімбалом
    out.gimbal.target_azimuth = target.azimuth;
    out.gimbal.target_elevation = target.elevation;

    // Логіка пострілу
    out.servo.trigger_fire = (out.status == TurretStatus::TRIGGER_READY);

    return out;
  }
};