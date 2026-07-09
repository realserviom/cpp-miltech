#include "c2_controller.hpp"
#include "fc_link.hpp"     // MAVSDK обгортка, API описано у fc_link.hpp
#include "udp_socket.hpp"  // UDP прийом, API описано у udp_socket.hpp

#include <json.hpp>  // Розбiр JSON з точками маршруту вiд auto_stub

#include <fstream>
#include <iostream>
#include <string>

static constexpr uint16_t STUB_PORT = 14560;

// Функція для конвертації C2State в рядок для логування
inline std::string to_string(C2State state)
{
  switch (state) {
    case C2State::DISARMED:
      return "DISARMED";
    case C2State::ARMED_HOLD:
      return "ARMED_HOLD";
    case C2State::ARMED_GUIDED:
      return "ARMED_GUIDED";
    case C2State::ARMED_MANUAL:
      return "ARMED_MANUAL";
  }
  return "UNKNOWN";
}

struct C2Controller::Impl {
  C2State state = C2State::DISARMED;

  // Компоненти зв'язку
  FcLink fc;
  UdpSocket socket;

  // Логування
  std::ofstream log_file;

  Impl(uint16_t fc_port)
    : fc(fc_port)
    , socket(STUB_PORT)
  {
    // Відкриваємо лог-файл у режимі додавання (append)
    log_file.open("/var/log/c2/c2.log", std::ios::app);
    if (!log_file.is_open()) {
      std::cerr << "[C2] Warning: Failed to open log file /var/log/c2/c2.log" << std::endl;
    }
  }

  void log_message(const std::string& msg)
  {
    // Вивід у stdout
    std::cout << msg << std::endl;
    // Запис у файл
    if (log_file.is_open()) {
      log_file << msg << std::endl;
    }
  }

  void transition(C2State next)
  {
    if (next != state) {
      std::string log_msg = "[C2] state: " + to_string(state) + " -> " + to_string(next);
      log_message(log_msg);

      state = next;

      // Один раз на вхід у стан ARMED_HOLD надсилаємо fc.hold()
      if (state == C2State::ARMED_HOLD) {
        fc.hold();
      }
    }
  }
};

C2Controller::C2Controller(uint16_t fc_port)
  : impl_(std::make_unique<Impl>(fc_port))
{
}

C2Controller::~C2Controller() = default;

void C2Controller::tick()
{
  // Оновлення стану на основі FC
  if (!impl_->fc.is_armed()) {
    impl_->transition(C2State::DISARMED);
  }
  else {
    switch (impl_->fc.flight_mode()) {
      case FcLink::FlightMode::Guided:
        impl_->transition(C2State::ARMED_GUIDED);
        break;
      case FcLink::FlightMode::Hold:
        impl_->transition(C2State::ARMED_HOLD);
        break;
      case FcLink::FlightMode::Manual:
      default:
        impl_->transition(C2State::ARMED_MANUAL);
        break;
    }
  }

  // Читання всіх UDP-пакетів від auto_stub
  std::vector<char> buffer(2048);
  while (true) {
    ssize_t bytes_received = impl_->socket.recv(buffer.data(), buffer.size() - 1);

    if (bytes_received < 0) {
      break;
    }

    if (bytes_received == 0) {
      continue;
    }

    buffer[bytes_received] = '\0';  // Гарантуємо null-terminated string для JSON парсера
    std::string json_str(buffer.data(), bytes_received);

    try {
      auto json_data = nlohmann::json::parse(json_str);

      // Перевіряємо наявність обов'язкових полів у JSON
      if (json_data.contains("north_m") && json_data.contains("east_m")) {
        float north = json_data["north_m"].get<float>();
        float east = json_data["east_m"].get<float>();

        // Арбітраж команди залежно від поточного стану C2
        if (current_state() == C2State::ARMED_GUIDED) {
          // Передаємо команду в польотний контролер
          impl_->fc.go_to_ned(north, east, 0.0f);

          std::string log_msg = "[C2] fwd: north=" + std::to_string(north) + " east=" + std::to_string(east);
          impl_->log_message(log_msg);
        }
        else {
          // Блокуємо передачу точки
          std::string log_msg = "[C2] blocked: waypoint in " + to_string(current_state());
          impl_->log_message(log_msg);
        }
      }
    }
    catch (const nlohmann::json::parse_error& e) {
      // Ігноруємо биті пакети або логуємо за потреби дебагу
      // std::cerr << "[C2] JSON parse error: " << e.what() << std::endl;
    }
  }

  // Реалізація вимоги щодо створення прапорця здоров'я сервісу (/tmp/c2_healthy)
  // в докер файлі зроблена провірка на обновлення файлу
  if (impl_->fc.is_connected()) {
    std::ofstream health_file("/tmp/c2_healthy");
    if (health_file.is_open()) {
      health_file << "OK\n";
    }
  }
}

C2State C2Controller::current_state() const
{
  return impl_->state;
}
