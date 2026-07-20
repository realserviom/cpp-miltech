#include <gtest/gtest.h>
#include "antidrone_turret/turret_logic.hpp"

TEST(TurretLogicTest, ReloadingStatusWhenActuatorBusy)
{
  Target t{10.0, 0.0, 0.0, true};
  auto result = TurretController::calculate(t, ActuatorStatus::RELOADING);

  EXPECT_EQ(result.status, TurretStatus::TRIGGER_RELOADING);
  EXPECT_FALSE(result.servo.trigger_fire);
}

TEST(TurretLogicTest, ReadyToFireWhenInRange)
{
  Target t{10.0, 0.0, 0.0, true};
  auto result = TurretController::calculate(t, ActuatorStatus::READY);

  EXPECT_EQ(result.status, TurretStatus::TRIGGER_READY);
  EXPECT_TRUE(result.servo.trigger_fire);
}