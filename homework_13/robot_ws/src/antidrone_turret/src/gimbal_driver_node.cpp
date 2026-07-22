#include <memory>
#include "rclcpp/rclcpp.hpp"

// Підключаємо ваше кастомне повідомлення
// Припускаємо, що файл називається GimbalCommand.msg у пакеті antidrone_turret
#include "antidrone_turret/msg/gimbal_command.hpp"

class GimbalDriverNode : public rclcpp::Node {
public:
  GimbalDriverNode()
    : Node("gimbal_driver_node")
  {
    // Підписуємося на топік з вашим типом повідомлення
    cmd_sub_ = this->create_subscription<antidrone_turret::msg::GimbalCommand>(
      "/gimbal/cmd", 10, std::bind(&GimbalDriverNode::cmdCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Gimbal Driver Node запущено та очікує команди!");
  }

private:
  void cmdCallback(const antidrone_turret::msg::GimbalCommand::SharedPtr msg)
  {
    // Доступ до констант та полів вашого повідомлення:
    int8_t dir = msg->direction;
    float target_y = msg->target_y;
    float error_y = msg->error_y;

    // Приклад перевірки напрямку за допомогою констант з повідомлення
    using Cmd = antidrone_turret::msg::GimbalCommand;

    std::string dir_str = "CENTER";
    if (dir == Cmd::UP)
      dir_str = "UP";
    else if (dir == Cmd::DOWN)
      dir_str = "DOWN";

    RCLCPP_INFO(this->get_logger(), "Команда: dir=%s (%d), target_y=%.2f, error_y=%.2f", dir_str.c_str(), dir, target_y, error_y);

    // TODO: Тут логіка відправки команд на сервоприводи / контролер підвісу
  }

  rclcpp::Subscription<antidrone_turret::msg::GimbalCommand>::SharedPtr cmd_sub_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<GimbalDriverNode>());
  rclcpp::shutdown();
  return 0;
}