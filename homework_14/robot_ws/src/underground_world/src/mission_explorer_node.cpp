#include <chrono>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "underground_world/msg/local_scan.hpp"
#include "underground_world/msg/move_command.hpp"
#include "underground_world/msg/student_status.hpp"
#include "underground_world/srv/payload_trigger.hpp"

using namespace std::chrono_literals;

struct Point {
  int x;
  int y;

  bool operator<(const Point& other) const
  {
    if (x != other.x)
      return x < other.x;
    return y < other.y;
  }

  bool operator==(const Point& other) const { return x == other.x && y == other.y; }
};

class MissionExplorerNode : public rclcpp::Node {
public:
  MissionExplorerNode()
    : Node("mission_explorer")
  {
    // QoS профіль для надійної доставки сканів
    rmw_qos_profile_t qos_profile = rmw_qos_profile_sensor_data;
    qos_profile.reliability = RMW_QOS_POLICY_RELIABILITY_RELIABLE;
    auto qos = rclcpp::QoS(rclcpp::QoSInitialization::from_rmw(qos_profile), qos_profile);

    sub_scan_ = this->create_subscription<underground_world::msg::LocalScan>(
      "/robot/local_scan", qos, std::bind(&MissionExplorerNode::on_local_scan, this, std::placeholders::_1));

    pub_cmd_ = this->create_publisher<underground_world::msg::MoveCommand>("/robot/cmd_move", 10);
    pub_status_ = this->create_publisher<underground_world::msg::StudentStatus>("/student/status", 10);

    cli_trigger_ = this->create_client<underground_world::srv::PayloadTrigger>("/payload/trigger");

    RCLCPP_INFO(this->get_logger(), "Mission Explorer Node initialized.");
  }

private:
  Point current_pos_{0, 0};
  bool is_engaging_{false};

  std::set<Point> known_walkable_;    // Клітинки, де фізично можна ходити (., S, x)
  std::set<Point> visited_cells_;     // Клітинки, які вже потрапляли у наш LocalScan
  std::set<int> processed_contacts_;  // Знешкоджені вороги

  rclcpp::Subscription<underground_world::msg::LocalScan>::SharedPtr sub_scan_;
  rclcpp::Publisher<underground_world::msg::MoveCommand>::SharedPtr pub_cmd_;
  rclcpp::Publisher<underground_world::msg::StudentStatus>::SharedPtr pub_status_;
  rclcpp::Client<underground_world::srv::PayloadTrigger>::SharedPtr cli_trigger_;

  void publish_status(uint8_t state)
  {
    underground_world::msg::StudentStatus msg;
    msg.state = state;
    pub_status_->publish(msg);
  }

  void on_local_scan(const underground_world::msg::LocalScan::SharedPtr scan)
  {
    if (is_engaging_)
      return;  // Захист від повторної обробки під час сервісного виклику

    current_pos_ = {scan->robot_x, scan->robot_y};

    bool has_unhandled_contact = false;
    Point contact_pos;
    int contact_id = 0;

    for (const auto& cell : scan->cells) {
      Point p{cell.x, cell.y};
      visited_cells_.insert(p);  // Всі бачені клітинки позначаємо як відомі

      if (cell.cell_type == "." || cell.cell_type == "S" || cell.cell_type == "x") {
        known_walkable_.insert(p);
      }
      else if (cell.cell_type == "C") {
        if (processed_contacts_.find(cell.contact_id) == processed_contacts_.end()) {
          has_unhandled_contact = true;
          contact_pos = p;
          contact_id = cell.contact_id;
        }
        else {
          known_walkable_.insert(p);  // Якщо контакт уже опрацьований (стає x), туди можна ходити
        }
      }
    }

    // Якщо є незнешкоджений ворог — відпрацьовуємо PayloadTrigger
    if (has_unhandled_contact) {
      is_engaging_ = true;
      publish_status(underground_world::msg::StudentStatus::ENGAGING);
      call_payload_service(contact_id, contact_pos);
      return;
    }

    // Плануємо наступний крок
    make_next_move();
  }

  void call_payload_service(int contact_id, Point p)
  {
    if (!cli_trigger_->wait_for_service(1s)) {
      RCLCPP_ERROR(this->get_logger(), "Service /payload/trigger unavailable!");
      is_engaging_ = false;
      return;
    }

    auto req = std::make_shared<underground_world::srv::PayloadTrigger::Request>();
    req->contact_id = contact_id;
    req->x = p.x;
    req->y = p.y;

    cli_trigger_->async_send_request(req,
                                     [this, contact_id, p](rclcpp::Client<underground_world::srv::PayloadTrigger>::SharedFuture future) {
                                       auto res = future.get();
                                       if (res->accepted) {
                                         processed_contacts_.insert(contact_id);
                                         known_walkable_.insert(p);  // Тепер ця клітинка безпечна для проходу
                                       }
                                       is_engaging_ = false;
                                     });
  }

  void make_next_move()
  {
    std::vector<Point> path = find_path_to_nearest_unvisited();

    if (path.empty()) {
      publish_status(underground_world::msg::StudentStatus::DONE);
      RCLCPP_INFO(this->get_logger(), "Місія виконана: Всі уголки дослідженні!");
      return;
    }

    publish_status(underground_world::msg::StudentStatus::EXPLORING);

    Point next_step = path[0];
    underground_world::msg::MoveCommand cmd;

    if (next_step.y < current_pos_.y)
      cmd.direction = underground_world::msg::MoveCommand::UP;
    else if (next_step.y > current_pos_.y)
      cmd.direction = underground_world::msg::MoveCommand::DOWN;
    else if (next_step.x < current_pos_.x)
      cmd.direction = underground_world::msg::MoveCommand::LEFT;
    else if (next_step.x > current_pos_.x)
      cmd.direction = underground_world::msg::MoveCommand::RIGHT;

    pub_cmd_->publish(cmd);
  }

  // BFS шукає найкоротший шлях до найближчої точки, сусіди якої ще НЕ були бачені у LocalScan
  std::vector<Point> find_path_to_nearest_unvisited()
  {
    std::queue<Point> q;
    std::map<Point, Point> parent;
    std::set<Point> visited_bfs;

    q.push(current_pos_);
    visited_bfs.insert(current_pos_);

    Point target{-1, -1};
    bool found = false;

    const std::vector<Point> dirs = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};

    while (!q.empty()) {
      Point curr = q.front();
      q.pop();

      // Перевіряємо, чи є навколо цієї точки необроблені сусідні клітинки
      for (const auto& d : dirs) {
        Point neighbor{curr.x + d.x, curr.y + d.y};
        if (visited_cells_.find(neighbor) == visited_cells_.end()) {
          target = curr;  // Нам потрібно прийти в curr, щоб відкрити neighbor
          found = true;
          break;
        }
      }
      if (found)
        break;

      for (const auto& d : dirs) {
        Point neighbor{curr.x + d.x, curr.y + d.y};
        // Ходити під час пошуку шляху можна ТІЛЬКИ по перевірених прохідних клітинках
        if (known_walkable_.count(neighbor) && !visited_bfs.count(neighbor)) {
          visited_bfs.insert(neighbor);
          parent[neighbor] = curr;
          q.push(neighbor);
        }
      }
    }

    if (!found || target == current_pos_) {
      if (found && target == current_pos_) {
        // Якщо невідома клітинка вже поруч — йдемо туди
        for (const auto& d : dirs) {
          Point neighbor{current_pos_.x + d.x, current_pos_.y + d.y};
          if (visited_cells_.find(neighbor) == visited_cells_.end() && known_walkable_.count(neighbor)) {
            return {neighbor};
          }
        }
      }
      return {};
    }

    std::vector<Point> path;
    Point curr = target;
    while (!(curr == current_pos_)) {
      path.push_back(curr);
      curr = parent[curr];
    }
    std::reverse(path.begin(), path.end());
    return path;
  }
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MissionExplorerNode>());
  rclcpp::shutdown();
  return 0;
}