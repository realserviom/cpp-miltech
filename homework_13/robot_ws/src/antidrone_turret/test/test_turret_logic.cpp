#include <gtest/gtest.h>
#include "antidrone_turret/turret_logic.hpp"

// struct Output {
//   GimbalCommand gimbal;
//   ServoCommand servo;
//   TurretStatus status;
//   bool publish_commands{false};      // Прапор для ноди: чи публікувати /gimbal/cmd та /servo/cmd
//   bool call_trigger_service{false};  // Прапор для ноди: чи викликати сервіс /actuator/trigger
// };

// struct TurretStatus {
//   uint8_t target_state{TargetState::NONE};
//   uint8_t action{ActionState::IDLE};
//   uint8_t trigger_state{TriggerState::SKIP};
//   float confidence{0.0f};
//   float distance_m{0.0f};
// };

using namespace turret_logic;

TEST(TurretLogicTest, ReloadingStatusWhenActuatorBusy)
{
  Target t{true, 350.0, 260.0, 15.0, 0.9};
  auto result = TurretController::calculate(t, ActuatorState::RELOADING);

  EXPECT_EQ(result.gimbal.direction, static_cast<int>(GimbalDir::DOWN));
  EXPECT_EQ(result.servo.direction, static_cast<int>(ServoDir::RIGHT));
  EXPECT_EQ(result.status.action, ActionState::TRACK);
  EXPECT_EQ(result.status.trigger_state, TriggerState::RELOADING);
  EXPECT_EQ(result.status.target_state, TargetState::LOCKED);
  EXPECT_TRUE(result.publish_commands);
  EXPECT_FALSE(result.call_trigger_service);
}

TEST(TurretLogicTest, ReadyToFireWhenInRange)
{
  Target t{true, 350.0, 260.0, 31.0, 0.6};
  auto result = TurretController::calculate(t, ActuatorState::RELOADING);

  EXPECT_EQ(result.gimbal.direction, static_cast<int>(GimbalDir::CENTER));
  EXPECT_EQ(result.servo.direction, static_cast<int>(ServoDir::CENTER));
  EXPECT_EQ(result.status.action, ActionState::IDLE);
  EXPECT_EQ(result.status.trigger_state, TriggerState::SKIP);
  EXPECT_EQ(result.status.target_state, TargetState::LOW_CONFIDENCE);
  EXPECT_FALSE(result.publish_commands);
  EXPECT_FALSE(result.call_trigger_service);
}