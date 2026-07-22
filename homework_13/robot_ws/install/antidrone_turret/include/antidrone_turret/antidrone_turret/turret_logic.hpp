#pragma once

#include <cstdint>

namespace turret_logic {

namespace TargetState {
constexpr uint8_t NONE = 0;
constexpr uint8_t LOW_CONFIDENCE = 1;
constexpr uint8_t LOCKED = 2;
}  // namespace TargetState

namespace ActionState {
constexpr uint8_t IDLE = 0;
constexpr uint8_t TRACK = 1;
}  // namespace ActionState

namespace TriggerState {
constexpr uint8_t SKIP = 0;
constexpr uint8_t REQUESTED = 1;
constexpr uint8_t RELOADING = 2;
}  // namespace TriggerState

namespace GimbalDir {
constexpr int8_t DOWN = -1;
constexpr int8_t CENTER = 0;
constexpr int8_t UP = 1;
}  // namespace GimbalDir

namespace ServoDir {
constexpr int8_t LEFT = -1;
constexpr int8_t CENTER = 0;
constexpr int8_t RIGHT = 1;
}  // namespace ServoDir

namespace ActuatorState {
constexpr uint8_t READY = 0;
constexpr uint8_t RELOADING = 1;
}  // namespace ActuatorState

// --- Вхідні та вихідні структури ---

struct Target {
  bool visible{false};
  float x{0.0f};
  float y{0.0f};
  float distance_m{0.0f};
  float confidence{0.0f};
};

struct Config {
  float confidence_threshold{0.7f};
  float max_distance_m{50.0f};
  float frame_center_x{320.0f};
  float frame_center_y{240.0f};
};

struct GimbalCommand {
  int8_t direction{GimbalDir::CENTER};
  float target_y{0.0f};
  float error_y{0.0f};
};

struct ServoCommand {
  int8_t direction{ServoDir::CENTER};
  float target_x{0.0f};
  float error_x{0.0f};
};

struct TurretStatus {
  uint8_t target_state{TargetState::NONE};
  uint8_t action{ActionState::IDLE};
  uint8_t trigger_state{TriggerState::SKIP};
  float confidence{0.0f};
  float distance_m{0.0f};
};

// --- Клас контролера ---

class TurretController {
public:
  struct Output {
    GimbalCommand gimbal;
    ServoCommand servo;
    TurretStatus status;
    bool publish_commands{false};      // Прапор для ноди: чи публікувати /gimbal/cmd та /servo/cmd
    bool call_trigger_service{false};  // Прапор для ноди: чи викликати сервіс /actuator/trigger
  };

  static Output calculate(const Target& target, uint8_t actuator_state, const Config& config = Config())
  {
    Output out;

    out.status.confidence = target.confidence;
    out.status.distance_m = target.distance_m;

    // Оцінка стану цілі
    if (!target.visible) {
      out.status.target_state = TargetState::NONE;
    }
    else if (target.confidence < config.confidence_threshold) {
      out.status.target_state = TargetState::LOW_CONFIDENCE;
    }
    else {
      out.status.target_state = TargetState::LOCKED;
    }

    // Якщо ціль відсутня або є невпевненість -> ACTION_IDLE
    if (out.status.target_state != TargetState::LOCKED) {
      out.status.action = ActionState::IDLE;
      out.status.trigger_state = TriggerState::SKIP;
      out.publish_commands = false;
      out.call_trigger_service = false;
      return out;
    }

    // Якщо ціль LOCKED тоді action = ACTION_TRACK і розрахунок команд наведення
    out.status.action = ActionState::TRACK;
    out.publish_commands = true;

    // Горизонтальне наведення (Yaw Servo)
    out.servo.target_x = target.x;
    out.servo.error_x = target.x - config.frame_center_x;
    if (out.servo.error_x > 0.0f) {
      out.servo.direction = ServoDir::RIGHT;
    }
    else if (out.servo.error_x < 0.0f) {
      out.servo.direction = ServoDir::LEFT;
    }
    else {
      out.servo.direction = ServoDir::CENTER;
    }

    // Вертикальне наведення (Gimbal)
    out.gimbal.target_y = target.y;
    out.gimbal.error_y = config.frame_center_y - target.y;  // y росте вниз у кадрі
    if (out.gimbal.error_y > 0.0f) {
      out.gimbal.direction = GimbalDir::UP;
    }
    else if (out.gimbal.error_y < 0.0f) {
      out.gimbal.direction = GimbalDir::DOWN;
    }
    else {
      out.gimbal.direction = GimbalDir::CENTER;
    }

    // Логіка рішення про постріл
    if (actuator_state == ActuatorState::RELOADING) {
      out.status.trigger_state = TriggerState::RELOADING;
      out.call_trigger_service = false;
    }
    else if (target.distance_m <= config.max_distance_m) {
      out.status.trigger_state = TriggerState::REQUESTED;
      out.call_trigger_service = true;
    }
    else {
      out.status.trigger_state = TriggerState::SKIP;
      out.call_trigger_service = false;
    }

    return out;
  }
};

}  // namespace turret_logic