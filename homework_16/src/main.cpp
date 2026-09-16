#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

//  ############ Регістри MPU-6050 ############
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_TEMP_OUT_H 0x41
#define MPU6050_REG_GYRO_XOUT_H 0x43
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_WHO_AM_I 0x75

int16_t read_i2c_word(int fd, uint8_t reg)
{
  if (write(fd, &reg, 1) != 1)
    return 0;
  uint8_t buf[2] = {0};
  if (read(fd, buf, 2) != 2)
    return 0;
  return static_cast<int16_t>((buf[0] << 8) | buf[1]);
}

int main(int argc, char* argv[])
{
  std::string i2c_bus = "/dev/i2c-1";
  int addr = 0x68;
  int count = 5;
  int rate = 5;

  //  ############ Парсинг аргументів командного рядка ############
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.rfind("/dev/", 0) == 0) {
      i2c_bus = arg;
    }
    else if (arg.rfind("0x", 0) == 0 || arg.rfind("0X", 0) == 0) {
      addr = std::strtol(arg.c_str(), nullptr, 16);
    }
    else if (arg == "--count" && i + 1 < argc) {
      count = std::atoi(argv[++i]);
    }
    else if (arg == "--rate" && i + 1 < argc) {
      rate = std::atoi(argv[++i]);
    }
  }

  // ############ Логи ініціалізації ############
  std::cout << "[LOG] I2C_BUS DEVICE....... " << i2c_bus << " [System]\n";
  std::cout << "[LOG] DEVICE адреса.... 0x" << std::hex << addr << std::dec << "  [System]\n";
  std::cout << "[LOG] Частота даних... " << rate << " Гц.  [System]\n";

  // ############ Відкриття I2C шини ############
  int fd = open(i2c_bus.c_str(), O_RDWR);
  if (fd < 0) {
    std::cerr << "[LOG] ERROR | Failed to open I2C bus.\n";
    return 1;
  }

  if (ioctl(fd, I2C_SLAVE, addr) < 0) {
    std::cerr << "[LOG] ERROR | Failed to set I2C address.\n";
    close(fd);
    return 1;
  }

  // ############ Пробудження MPU-6050 (скидуємо всі біти в 0) ######################
  // Bit 7 (DEVICE_RESET = 0): Не скидаємо регістри (залишаємо їх у поточному стані).
  // Bit 6 (SLEEP = 0): Будимо пристрій — він виходить зі сну і починає працювати.
  // Bit 5 (CYCLE = 0): Вимикаємо циклічний режим сну/вимірювання (пристрій працює в звичайному безупинному режимі).
  // Bit 3 (TEMP_DIS = 0):  Вмикаємо датчик температури, 1 - виключений; 0 - включений
  // Bits 2:0 (CLKSEL = 000): Вибираємо джерелом тактування базовий внутрішній генератор на 8 МГц.

  uint8_t pwr_cmd[2] = {MPU6050_REG_PWR_MGMT_1, 0x00};
  write(fd, pwr_cmd, 2);

  // ############ налаштовуємо режим +-8g для вимірювання прискорення до 8g ######################
  uint8_t accel_cfg[2] = {0x1C, 0x10};
  write(fd, accel_cfg, 2);

  // ############# Валідація за WHO_AM_I ###################
  uint8_t reg_who = MPU6050_REG_WHO_AM_I;
  if (write(fd, &reg_who, 1) != 1) {
    std::cerr << "[LOG] ERROR | Failed to write WHO_AM_I register address.\n";
    close(fd);
    return 1;
  }

  uint8_t who_val = 0;
  if (read(fd, &who_val, 1) != 1) {
    std::cerr << "[LOG] ERROR | Failed to read WHO_AM_I register value.\n";
    close(fd);
    return 1;
  }

  if (who_val != 0x68) {
    std::cerr << "[LOG] ERROR | Hardware validation failed! Expected 0x68, got 0x" << std::hex << (int)who_val << std::dec << "\n";
    close(fd);
    return 1;  // Завершуємо роботу, бо це не MPU-6050 або датчик несправний
  }

  std::cout << "[LOG] SUCCESS | MPU-6050 hardware validation verified. Executing sensor stream. [Main]\n";

  int delay_ms = 1000 / (rate > 0 ? rate : 1);

  for (int i = 0; i < count; ++i) {
    // Читання даних з акселерометра
    int16_t raw_ax = read_i2c_word(fd, MPU6050_REG_ACCEL_XOUT_H);
    int16_t raw_ay = read_i2c_word(fd, MPU6050_REG_ACCEL_XOUT_H + 2);
    int16_t raw_az = read_i2c_word(fd, MPU6050_REG_ACCEL_XOUT_H + 4);

    // Читання даних з температури
    int16_t raw_temp = read_i2c_word(fd, MPU6050_REG_TEMP_OUT_H);

    // Читання даних з гіроскопа
    int16_t raw_gx = read_i2c_word(fd, MPU6050_REG_GYRO_XOUT_H);
    int16_t raw_gy = read_i2c_word(fd, MPU6050_REG_GYRO_XOUT_H + 2);
    int16_t raw_gz = read_i2c_word(fd, MPU6050_REG_GYRO_XOUT_H + 4);

    // Перерахунок за даташитом MPU-6050
    // ±8g -> 4096 LSB/g
    float ax = raw_ax / 4096.0f;
    float ay = raw_ay / 4096.0f;
    float az = raw_az / 4096.0f;

    // ±250 deg/s -> 131 LSB/dps
    float gx = raw_gx / 131.0f;
    float gy = raw_gy / 131.0f;
    float gz = raw_gz / 131.0f;

    // Formula: Temp in C = (TEMP_OUT / 340.0) + 36.53
    float temp_c = (raw_temp / 340.0f) + 36.53f;

    std::cout << "[LOG] Прискорення: [" << std::setw(6) << std::fixed << std::setprecision(3) << ax << ", " << std::setw(6) << ay << ", "
              << std::setw(6) << az << "] g | Оберти: [" << std::setw(6) << std::setprecision(2) << gx << ", " << std::setw(6) << gy << ", "
              << std::setw(6) << gz << "] dps | Температура: " << std::setw(4) << std::setprecision(1) << temp_c << " C [Sensor]\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
  }

  close(fd);
  return 0;
}