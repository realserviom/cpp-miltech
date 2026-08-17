#pragma once

#include <iostream>
#include <cstring>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include "mavlink/common/mavlink.h"

class MavlinkTelemetry {
private:
  int socket_fd_ = -1;
  sockaddr_in target_addr_{};

  const uint8_t sysid_ = 1;
  const uint8_t compid_ = MAV_COMP_ID_AUTOPILOT1;

  // Константи для геоприв'язки
  static constexpr double LAT0 = 50.4501;
  static constexpr double LON0 = 30.5234;

  // Таймери відправки (в мс)
  uint32_t last_heartbeat_ms_ = 0;
  uint32_t last_telemetry_ms_ = 0;

  // Логіка повтору COMMAND_LONG (скид)
  bool drop_active_ = false;   // скинутий і в процесі відповіді
  bool drop_acked_ = false;    // отримали відповідь
  int drop_retries_left_ = 0;  // кількість спроб що залишилося щоб отримати відповідь
  std::chrono::steady_clock::time_point last_cmd_sent_time_;
  mavlink_command_long_t pending_drop_cmd_{};  // об'єкт який відправляється коли ми скидуємо боєприпас

public:
  MavlinkTelemetry() = default;

  ~MavlinkTelemetry()
  {
    if (socket_fd_ >= 0) {
      close(socket_fd_);
    }
  }

  bool getDropAcked() { return drop_acked_; }

  bool init(const std::string& ip = "127.0.0.1", int port = 14550)
  {
    socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd_ < 0) {
      std::cerr << "[MAVLink] Failed to create UDP socket" << std::endl;
      return false;
    }

    // Не блокуючий режим сокета
    int flags = fcntl(socket_fd_, F_GETFL, 0);
    fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);

    target_addr_.sin_family = AF_INET;
    target_addr_.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &target_addr_.sin_addr) <= 0) {
      std::cerr << "[MAVLink] Invalid IP address" << std::endl;
      return false;
    }

    std::cout << "[MAVLink] Telemetry initialized -> " << ip << ":" << port << std::endl;
    return true;
  }

  void pollIncomingPackets()
  {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);

    while (true) {
      ssize_t bytes_rx = recvfrom(socket_fd_, buf, sizeof(buf), 0, (struct sockaddr*)&src_addr, &addr_len);
      if (bytes_rx <= 0)
        break;  // Даних у неблокуючому сокеті більше немає

      mavlink_message_t msg;
      mavlink_status_t status;

      for (ssize_t i = 0; i < bytes_rx; ++i) {
        if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) {
          if (msg.msgid == MAVLINK_MSG_ID_COMMAND_ACK) {
            mavlink_command_ack_t ack;
            mavlink_msg_command_ack_decode(&msg, &ack);

            if (ack.command == MAV_CMD_USER_1 && ack.result == MAV_RESULT_ACCEPTED) {
              std::cout << "[MAVLink] SUCCESS: COMMAND_ACK received!" << std::endl;
              drop_acked_ = true;
              drop_active_ = false;
            }
          }
        }
      }
    }
  }

  // Головний метод відправки
  void processTelemetry(float x, float y, float z, float vx, float vy, float dir_rad, uint32_t t_ms)
  {
    // Обробка HEARTBEAT (1 Гц, раз на 1 с)
    if (t_ms - last_heartbeat_ms_ >= 1000 || last_heartbeat_ms_ == 0) {
      sendHeartbeat();

      // std::this_thread::sleep_for(std::chrono::seconds(1));

      std::cout << "[MAVLink] COMMAND sendHeartbeat sent. " << std::endl;
      last_heartbeat_ms_ = t_ms;
    }

    // Обробка TELEMETRY (10 Гц, раз на 100 мс)
    if (t_ms - last_telemetry_ms_ >= 100 || last_telemetry_ms_ == 0) {
      sendGlobalPositionAndAttitude(x, y, z, vx, vy, dir_rad, t_ms);
      std::cout << "[MAVLink] COMMAND sendGlobalPositionAndAttitude sent. " << std::endl;
      last_telemetry_ms_ = t_ms;
    }

    // Зчитуємо вхідні пакети (ACK)
    // pollIncomingPackets();

    // Менеджер повторів скиду вантажу
    // processDropRetries();
  }

  // Викликаємо при настанні моменту скиду
  void triggerCargoDrop(float drop_x, float drop_y, float drop_z)
  {
    if (drop_acked_)
      return;  // Вже успішно скинуто і підтверджено

    double drop_lat = LAT0 + (drop_y / 111320.0);
    double drop_lon = LON0 + (drop_x / (111320.0 * std::cos(LAT0 * M_PI / 180.0)));

    pending_drop_cmd_.command = MAV_CMD_USER_1;
    pending_drop_cmd_.param1 = 0;
    pending_drop_cmd_.param2 = 0;
    pending_drop_cmd_.param3 = 0;
    pending_drop_cmd_.param4 = 0;
    pending_drop_cmd_.param5 = static_cast<float>(drop_lat);
    pending_drop_cmd_.param6 = static_cast<float>(drop_lon);
    pending_drop_cmd_.param7 = drop_z;

    drop_active_ = true;
    drop_retries_left_ = 5;

    sendDropCommandLong();
  }

private:
  void sendBuffer(const mavlink_message_t& msg)
  {
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    sendto(socket_fd_, buf, len, 0, (struct sockaddr*)&target_addr_, sizeof(target_addr_));
  }

  void sendHeartbeat()
  {
    mavlink_message_t msg;
    mavlink_msg_heartbeat_pack(sysid_,
                               compid_,
                               &msg,
                               MAV_TYPE_QUADROTOR,
                               MAV_AUTOPILOT_GENERIC,
                               MAV_MODE_FLAG_SAFETY_ARMED | MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
                               0,
                               MAV_STATE_ACTIVE);
    sendBuffer(msg);
  }

  void sendGlobalPositionAndAttitude(float x, float y, float z, float vx, float vy, float dir_rad, uint32_t t_ms)
  {
    // Перерахунок локальних метрів у GPS (градуси)
    double lat = LAT0 + (y / 111320.0);
    double lon = LON0 + (x / (111320.0 * std::cos(LAT0 * M_PI / 180.0)));

    int32_t lat_int = static_cast<int32_t>(lat * 1e7);
    int32_t lon_int = static_cast<int32_t>(lon * 1e7);
    int32_t alt_mm = static_cast<int32_t>(z * 1000.0f);

    // Курс hdg (0..35999 сотих градуса)
    double deg = dir_rad * (180.0 / M_PI);
    while (deg < 0)
      deg += 360.0;
    while (deg >= 360)
      deg -= 360.0;
    uint16_t hdg_cdeg = static_cast<uint16_t>(deg * 100.0);

    // GLOBAL_POSITION_INT
    mavlink_message_t msg_pos;
    mavlink_msg_global_position_int_pack(sysid_,
                                         compid_,
                                         &msg_pos,
                                         t_ms,
                                         lat_int,
                                         lon_int,
                                         alt_mm,                             // alt (MSL)
                                         alt_mm,                             // relative_alt
                                         static_cast<int16_t>(vx * 100.0f),  // cm/s
                                         static_cast<int16_t>(vy * 100.0f),  // cm/s
                                         0,
                                         hdg_cdeg);
    sendBuffer(msg_pos);

    // ATTITUDE
    mavlink_message_t msg_att;
    mavlink_msg_attitude_pack(sysid_,
                              compid_,
                              &msg_att,
                              t_ms,
                              0.0f,
                              0.0f,
                              dir_rad,  // roll=0, pitch=0, yaw=dir_rad
                              0.0f,
                              0.0f,
                              0.0f  // rates
    );
    sendBuffer(msg_att);
  }

  void sendDropCommandLong()
  {
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(sysid_,
                                  compid_,
                                  &msg,
                                  1,
                                  MAV_COMP_ID_ALL,
                                  pending_drop_cmd_.command,
                                  0,  // confirmation
                                  pending_drop_cmd_.param1,
                                  pending_drop_cmd_.param2,
                                  pending_drop_cmd_.param3,
                                  pending_drop_cmd_.param4,
                                  pending_drop_cmd_.param5,
                                  pending_drop_cmd_.param6,
                                  pending_drop_cmd_.param7);
    sendBuffer(msg);
    last_cmd_sent_time_ = std::chrono::steady_clock::now();
    drop_retries_left_--;
    std::cout << "[MAVLink] COMMAND_LONG (MAV_CMD_USER_1) sent. Retries left: " << drop_retries_left_ << std::endl;
  }

  void processDropRetries()
  {
    if (!drop_active_ || drop_acked_)
      return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cmd_sent_time_).count();

    // Таймаут повтору: 500 мс
    if (elapsed >= 500) {
      if (drop_retries_left_ > 0) {
        sendDropCommandLong();
      }
      else {
        std::cout << "[MAVLink] ACK not received after 5 attempts." << std::endl;
        drop_active_ = false;  // Вичерпали спроби
      }
    }
  }
};