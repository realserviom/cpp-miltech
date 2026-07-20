#include "rclcpp/rclcpp.hpp"
#include "antidrone_turret/turret_logic.hpp"
#include "antidrone_turret/msg/target.hpp"
#include "antidrone_turret/msg/actuator_status.hpp"
#include "antidrone_turret/msg/gimbal_command.hpp"
#include "antidrone_turret/msg/servo_command.hpp"
#include "antidrone_turret/srv/trigger_actuator.hpp"

class TurretControllerNode : public rclcpp::Node {
public:
  TurretControllerNode()
    : Node("turret_controller_node")
  {
    // Підписка на ціль через лямбду
    target_sub_ = this->create_subscription<antidrone_turret::msg::Target>(
      "/perception/target", 10, [this](const antidrone_turret::msg::Target::SharedPtr msg) { this->process(msg); });

    // Підписка на статус актуатора через лямбду
    status_sub_ = this->create_subscription<antidrone_turret::msg::ActuatorStatus>(
      "/actuator/status", 10, [this](const antidrone_turret::msg::ActuatorStatus::SharedPtr msg) { this->status_callback(msg); });

    // Паблішер для горизонтального серво
    yaw_pub_ = this->create_publisher<antidrone_turret::msg::ServoCommand>("/servo/cmd", 10);

    // Паблішер для гімбала
    gimbal_pub_ = this->create_publisher<antidrone_turret::msg::GimbalCommand>("/gimbal/cmd", 10);

    // Клієнт для сервісу пострілу
    trigger_client_ = this->create_client<antidrone_turret::srv::TriggerActuator>("/actuator/trigger");
  }

private:
  void status_callback(const antidrone_turret::msg::ActuatorStatus::SharedPtr msg)
  {
    current_actuator_status_ = static_cast<ActuatorStatus>(msg->state);
  }

  void process(const antidrone_turret::msg::Target::SharedPtr msg)
  {
    Target t{msg->distance, msg->azimuth, msg->elevation, msg->locked};

    // Виклик чистої логіки
    auto result = TurretController::calculate(t, current_actuator_status_);

    // Публікуємо команди гімбала
    auto gimbal_msg = antidrone_turret::msg::GimbalCommand();
    gimbal_msg.azimuth = result.gimbal.target_azimuth;
    gimbal_msg.elevation = result.gimbal.target_elevation;
    gimbal_pub_->publish(gimbal_msg);

    // Якщо логіка каже стріляти — викликаємо сервіс
    if (result.servo.trigger_fire) {
      call_trigger_service(msg->distance);
    }
  }

  void call_trigger_service(double distance)
  {
    if (!trigger_client_->wait_for_service(std::chrono::milliseconds(100))) {
      RCLCPP_WARN(this->get_logger(), "Service /actuator/trigger not available");
      return;
    }

    auto request = std::make_shared<antidrone_turret::srv::TriggerActuator::Request>();
    request->distance_m = distance;
    // Можна додати інші поля за потреби

    trigger_client_->async_send_request(request);
  }

  rclcpp::Subscription<antidrone_turret::msg::Target>::SharedPtr target_sub_;
  rclcpp::Subscription<antidrone_turret::msg::ActuatorStatus>::SharedPtr status_sub_;
  rclcpp::Publisher<antidrone_turret::msg::GimbalCommand>::SharedPtr gimbal_pub_;
  rclcpp::Publisher<antidrone_turret::msg::ServoCommand>::SharedPtr yaw_pub_;
  rclcpp::Client<antidrone_turret::srv::TriggerActuator>::SharedPtr trigger_client_;

  ActuatorStatus current_actuator_status_ = ActuatorStatus::READY;
};