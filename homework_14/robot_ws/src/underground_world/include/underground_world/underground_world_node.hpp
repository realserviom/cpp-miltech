#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"

#include "underground_world/msg/cell_observation.hpp"
#include "underground_world/msg/enemy_down.hpp"
#include "underground_world/msg/local_scan.hpp"
#include "underground_world/msg/move_command.hpp"
#include "underground_world/msg/robot_metrics.hpp"
#include "underground_world/msg/robot_result.hpp"
#include "underground_world/scenario_loader.hpp"
#include "underground_world/state_qos.hpp"
#include "underground_world/underground_world_node.hpp"

class UndergroundWorldNode final : public rclcpp::Node {
public:
  UndergroundWorldNode();

private:
  std::chrono::milliseconds read_move_commit_period();
  void on_move(const underground_world::msg::MoveCommand& msg);
  void on_enemy_down(const underground_world::msg::EnemyDown& msg);
  void commit_next_move();
  void publish_state();

  std::string scenario_path_;
  std::chrono::milliseconds move_commit_period_;
  underground_world::WorldModel world_;
  std::deque<std::uint8_t> pending_moves_;

  rclcpp::Publisher<underground_world::msg::LocalScan>::SharedPtr scan_pub_;
  rclcpp::Publisher<underground_world::msg::RobotMetrics>::SharedPtr metrics_pub_;
  rclcpp::Publisher<underground_world::msg::RobotResult>::SharedPtr result_pub_;

  rclcpp::Subscription<underground_world::msg::MoveCommand>::SharedPtr move_sub_;
  rclcpp::Subscription<underground_world::msg::EnemyDown>::SharedPtr enemy_down_sub_;

  rclcpp::TimerBase::SharedPtr move_commit_timer_;
  rclcpp::TimerBase::SharedPtr initial_publish_timer_;
};