#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "underground_world/srv/payload_trigger.hpp"
#include "underground_world/msg/enemy_down.hpp"

class PayloadActionNode : public rclcpp::Node {
public:
  PayloadActionNode()
    : Node("payload_action")
  {
    pub_enemy_down_ = this->create_publisher<underground_world::msg::EnemyDown>("/payload/enemy_down", 10);

    srv_trigger_ = this->create_service<underground_world::srv::PayloadTrigger>(
      "/payload/trigger", std::bind(&PayloadActionNode::handle_trigger, this, std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(this->get_logger(), "Payload Action Service Node initialized.");
  }

private:
  rclcpp::Publisher<underground_world::msg::EnemyDown>::SharedPtr pub_enemy_down_;
  rclcpp::Service<underground_world::srv::PayloadTrigger>::SharedPtr srv_trigger_;

  void handle_trigger(const std::shared_ptr<underground_world::srv::PayloadTrigger::Request> request,
                      std::shared_ptr<underground_world::srv::PayloadTrigger::Response> response)
  {
    RCLCPP_INFO(this->get_logger(), "Отримали наказ на знищення ворога: %d at (%d, %d)", request->contact_id, request->x, request->y);

    underground_world::msg::EnemyDown msg;
    msg.contact_id = request->contact_id;
    msg.x = request->x;
    msg.y = request->y;
    pub_enemy_down_->publish(msg);

    response->accepted = true;
    response->reason = "Ціль вражено!!!";
  }
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PayloadActionNode>());
  rclcpp::shutdown();
  return 0;
}