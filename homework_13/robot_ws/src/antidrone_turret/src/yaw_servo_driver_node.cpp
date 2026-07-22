#include <memory>
#include "rclcpp/rclcpp.hpp"

// Підключаємо кастомне повідомлення для горизонтального сервоприводу
#include "antidrone_turret/msg/servo_command.hpp"

class YawServoDriverNode : public rclcpp::Node {
public:
  YawServoDriverNode()
    : Node("yaw_servo_driver_node")
  {
    // Підписуємося на топік команд повороту по горизонталі (Yaw)
    cmd_sub_ = this->create_subscription<antidrone_turret::msg::ServoCommand>(
      "/servo/cmd", 10, std::bind(&YawServoDriverNode::cmdCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Yaw Servo Driver Node запущено. Очікування команд...");
  }

private:
  void cmdCallback(const antidrone_turret::msg::ServoCommand::SharedPtr msg)
  {
    // Зчитуємо поля з ServoCommand
    int8_t direction = msg->direction;
    float target_x = msg->target_x;
    float error_x = msg->error_x;

    // TODO: Передача сигналів на драйвер горизонтального сервоприводу (PWM / Serial / CAN)
    RCLCPP_INFO(this->get_logger(), "Yaw Servo Cmd -> Dir: %d, Target X: %.2f, Error X: %.2f", direction, target_x, error_x);
  }

  rclcpp::Subscription<antidrone_turret::msg::ServoCommand>::SharedPtr cmd_sub_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<YawServoDriverNode>());
  rclcpp::shutdown();
  return 0;
}