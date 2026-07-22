#include "rclcpp/rclcpp.hpp"
#include "antidrone_turret/turret_logic.hpp"
#include "antidrone_turret/msg/target.hpp"
#include "antidrone_turret/msg/actuator_status.hpp"
#include "antidrone_turret/msg/gimbal_command.hpp"
#include "antidrone_turret/msg/servo_command.hpp"
#include "antidrone_turret/msg/turret_status.hpp"
#include "antidrone_turret/srv/trigger_actuator.hpp"

class TurretControllerNode : public rclcpp::Node {
public:
  TurretControllerNode()
    : Node("turret_controller_node")
  {
    // Зчитуємо параметри з YAML (із значеннями за замовчуванням)
    config_.confidence_threshold = static_cast<float>(this->declare_parameter<double>("confidence_threshold", 0.8));
    config_.max_distance_m = static_cast<float>(this->declare_parameter<double>("max_distance_m", 30.0));

    // Підписка на ціль через лямбду
    target_sub_ = this->create_subscription<antidrone_turret::msg::Target>(
      "/perception/target", 10, [this](const antidrone_turret::msg::Target::SharedPtr msg) { this->process(msg); });

    // Підписка на статус актуатора через лямбду
    status_sub_ = this->create_subscription<antidrone_turret::msg::ActuatorStatus>(
      "/actuator/status", 10, [this](const antidrone_turret::msg::ActuatorStatus::SharedPtr msg) { this->status_callback(msg); });

    // Паблішер для горизонтального серво
    servo_pub_ = this->create_publisher<antidrone_turret::msg::ServoCommand>("/servo/cmd", 10);

    // Паблішер для гімбала
    gimbal_pub_ = this->create_publisher<antidrone_turret::msg::GimbalCommand>("/gimbal/cmd", 10);

    turret_status_pub_ = this->create_publisher<antidrone_turret::msg::TurretStatus>("/turret/status", 10);

    // Клієнт для сервісу пострілу
    trigger_client_ = this->create_client<antidrone_turret::srv::TriggerActuator>("/actuator/trigger");
  }

private:
  void status_callback(const antidrone_turret::msg::ActuatorStatus::SharedPtr msg) { current_actuator_state_ = msg->state; }

  void process(const antidrone_turret::msg::Target::SharedPtr msg)
  {
    // Перекладаємо ROS-повідомлення у чисту структуру для логіки
    turret_logic::Target target_input;
    target_input.visible = msg->visible;
    target_input.x = msg->x;
    target_input.y = msg->y;
    target_input.distance_m = msg->distance_m;
    target_input.confidence = msg->confidence;

    // Викликаємо чисту C++ логіку
    auto out = turret_logic::TurretController::calculate(target_input, current_actuator_state_, config_);

    // публікуємо загальний стан турелі в /turret/status для того щоб зчитати
    antidrone_turret::msg::TurretStatus status_msg;
    status_msg.target_state = out.status.target_state;
    status_msg.action = out.status.action;
    status_msg.trigger_state = out.status.trigger_state;
    status_msg.confidence = out.status.confidence;
    status_msg.distance_m = out.status.distance_m;
    turret_status_pub_->publish(status_msg);

    // Якщо ціль захвачена — публікуємо команди для сервоприводів
    if (out.publish_commands) {
      antidrone_turret::msg::ServoCommand servo_msg;
      servo_msg.direction = out.servo.direction;
      servo_msg.target_x = out.servo.target_x;
      servo_msg.error_x = out.servo.error_x;
      servo_pub_->publish(servo_msg);

      antidrone_turret::msg::GimbalCommand gimbal_msg;
      gimbal_msg.direction = out.gimbal.direction;
      gimbal_msg.target_y = out.gimbal.target_y;
      gimbal_msg.error_y = out.gimbal.error_y;
      gimbal_pub_->publish(gimbal_msg);
    }

    // Якщо потрібен постріл і сервіс доступний — викликаємо /actuator/trigger
    if (out.call_trigger_service) {
      call_trigger_service(msg->confidence, msg->distance_m);
    }
  }

  void call_trigger_service(float confidence, float distance_m)
  {
    if (!trigger_client_->service_is_ready()) {
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000, "Trigger service /actuator/trigger is not available!");
      return;
    }

    auto request = std::make_shared<antidrone_turret::srv::TriggerActuator::Request>();
    request->confidence = confidence;
    request->distance_m = distance_m;

    // Асинхронний виклик сервісу без блокування потоку
    trigger_client_->async_send_request(request, [this](rclcpp::Client<antidrone_turret::srv::TriggerActuator>::SharedFuture future) {
      auto response = future.get();
      if (response->accepted) {
        RCLCPP_INFO(this->get_logger(), "Shot FIRED! Total count: %u", response->trigger_count);
      }
      else {
        RCLCPP_WARN(this->get_logger(), "Shot request rejected by actuator.");
      }
    });
  }

  // Змінні ноди
  turret_logic::Config config_;
  uint8_t current_actuator_state_{turret_logic::ActuatorState::READY};

  rclcpp::Subscription<antidrone_turret::msg::Target>::SharedPtr target_sub_;
  rclcpp::Subscription<antidrone_turret::msg::ActuatorStatus>::SharedPtr status_sub_;
  rclcpp::Publisher<antidrone_turret::msg::GimbalCommand>::SharedPtr gimbal_pub_;
  rclcpp::Publisher<antidrone_turret::msg::ServoCommand>::SharedPtr servo_pub_;
  rclcpp::Publisher<antidrone_turret::msg::TurretStatus>::SharedPtr turret_status_pub_;
  rclcpp::Client<antidrone_turret::srv::TriggerActuator>::SharedPtr trigger_client_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<TurretControllerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}