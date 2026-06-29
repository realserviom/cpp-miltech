#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include "drone_link.h"  // Переконайся, що цей файл лежить поруч
#include <fstream>
#include <chrono>
#include <thread>

// Перевіряємо, чи ми на Raspberry Pi (зазвичай там доступний заголовок gpiod.h)
// Якщо ти тестуєш в контейнері, де gpiod.h взагалі не встановлено,
// ми просто закоментуємо цей include для ПК.
// #cmakedefine HAS_GPIOD 0  // або використовуй звичайний флаг компілятора

#if __arm__ || __aarch64__
#include <gpiod.h>
#define REAL_GPIO 1
#else
#define REAL_GPIO 0
#endif

using namespace dlink;

// Клас для керування GPIO (імітація + реальність)
class GPIOController {
private:
  bool is_ready = false;
#if REAL_GPIO
  struct gpiod_chip* chip = nullptr;
  struct gpiod_line* start_line = nullptr;
  struct gpiod_line* drop_line = nullptr;
#endif

public:
  GPIOController()
  {
#if REAL_GPIO
    // Код для реальної Малинки
    chip = gpiod_chip_open_by_name("gpiochip4");  // уточни номер чипа для своєї плати
    if (!chip) {
      std::perror("[GPIO] Помилка відкриття gpiod_chip");
      return;
    }
    // Наприклад, лінії 17 та 27 (номери пінів змініть на ваші з ТЗ)
    start_line = gpiod_chip_get_line(chip, 17);
    drop_line = gpiod_chip_get_line(chip, 27);

    gpiod_line_request_output(start_line, "drone_start", 0);
    gpiod_line_request_output(drop_line, "drone_drop", 0);
    is_ready = true;
#else
    // Код для ПК (Devcontainer)
    std::cout << "[MOCK GPIO] Режим імітації. Віртуальний чип створено." << std::endl;
    is_ready = true;
#endif
  }

  ~GPIOController()
  {
#if REAL_GPIO
    if (chip) {
      gpiod_line_release(start_line);
      gpiod_line_release(drop_line);
      gpiod_chip_close(chip);
    }
#endif
  }

  void set_start(int value)
  {
#if REAL_GPIO
    if (is_ready)
      gpiod_line_set_value(start_line, value);
#else
    std::cout << "[MOCK GPIO] Лiнiя START -> " << value << " (" << (value ? "ГОТОВИЙ" : "ВИМК") << ")" << std::endl;
    std::ofstream file("/tmp/my_gpio_bank/sim_gpio24/value");

    if (file.is_open()) {
      file << value;  // Записуємо '1' або '0'
      file.close();
      std::cout << "[MOCK GPIO] Створено файл і записано START = " << value << std::endl;
    }
    else {
      std::cerr << "[MOCK GPIO] Помилка: не вдалося створити файл лінії в /tmp/my_gpio_bank/sim_gpio24/value" << std::endl;
    }
#endif
  }

  void pulse_drop()
  {
    std::cout << "[GPIO] Виклик команди DROP!" << std::endl;
#if REAL_GPIO
    std::cout << "[MOCK GPIO] Лiнiя START -> " << value << " (" << (value ? "ГОТОВИЙ" : "ВИМК") << ")" << std::endl;
    if (is_ready)
      gpiod_line_set_value(drop_line, 1);
    usleep(80000);  // тримаємо 80 мс (в межах 50-100 мс)
    if (is_ready)
      gpiod_line_set_value(drop_line, 0);
#else
    std::ofstream file("/tmp/my_gpio_bank/sim_gpio23/value");

    if (file.is_open()) {
      file << 1;  // Записуємо '1' або '0'
      file.close();

      std::cout << "[MOCK GPIO] Створено файл і записано START = " << 1 << std::endl;

      usleep(80000);

      std::ofstream file("/tmp/my_gpio_bank/sim_gpio23/value");

      file << 0;
      file.close();
    }
    else {
      std::cerr << "[MOCK GPIO] Помилка: не вдалося створити файл лінії в /tmp/my_gpio_bank/sim_gpio23/value" << std::endl;
    }
#endif
  }
};

// Функція налаштування UART
int openUart(const char* dev)
{
  int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) {
    std::perror("Помилка відкриття UART");
    return -1;
  }

  termios tio{};
  tcgetattr(fd, &tio);
  cfmakeraw(&tio);  // 8N1, сирий бінарний режим
  cfsetispeed(&tio, B115200);
  cfsetospeed(&tio, B115200);  // швидкість 115200
  tio.c_cflag |= (CLOCAL | CREAD);
  tcsetattr(fd, TCSANOW, &tio);

  return fd;
}

int main()
{
  // Відкриваємо порт
  // Якщо тестуєш на комп'ютері з віртуальним портом, заміни на "/tmp/ttyA"

  // Налаштування socat для тестування:
  // sudo apt update && sudo apt install -y socat - встановлення
  // socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB - налаштування

  // const char* uartDevice = "/dev/ttyAMA1"; - це для Raspberry Pi
  const char* uartDevice = "/tmp/ttyA";  // це socat
  int uartFd = openUart(uartDevice);

  if (uartFd < 0) {
    return 1;  // Виходимо, якщо порт не відкрився
  }

  std::cout << "UART порт " << uartDevice << " успішно налаштовано на 115200 бод." << std::endl;
  std::cout << "Запуск головного циклу опитування..." << std::endl;

  // Створюємо контролер. В контейнері він автоматично увімкне імітацію!
  GPIOController gpio;

  // START — підняти у 1 одразу на старті й тримати.
  gpio.set_start(1);

  // 2. Створюємо екземпляр парсера та змінні для вихідних даних
  Parser parser;

  uint8_t outType = 0;
  uint8_t outLen = 0;
  uint8_t outPayload[260];
  bool already_dropped = false;

  auto startTime = std::chrono::high_resolution_clock::now();
  auto nextTimePoint = startTime + std::chrono::seconds(20);

  // Головний робочий цикл програми
  while (true) {
    // --- НЕБЛОКУЮЧЕ ЧИТАННЯ З UART ---
    uint8_t incomingByte;

    // Читаємо строго по 1 байту.
    // Якщо даних немає, read() миттєво поверне -1 або 0, і ми вийдемо з цього while
    while (read(uartFd, &incomingByte, 1) > 0) {
      // "Годуємо" парсер поточним байтом
      if (parser.feed(incomingByte, outType, outPayload, outLen)) {
        // УРА! parser.feed() повернув true — знайдено цілий валідний кадр з чистим CRC
        std::cout << "\n[Парсер] Зловлено новий пакет!" << std::endl;
        std::cout << " -> Тип пакета (TYPE): 0x" << std::hex << (int)outType << std::dec << std::endl;
        std::cout << " -> Довжина (LEN): " << (int)outLen << " байтів" << std::endl;

        // Тут можна зробити switch(outType) і розбирати payload
        // залежно від того, чи це телеметрія (0x01), чи ціль (0x02)
        if (outType == 0x01) {
          std::cout << " -> Оброблено пакет ТЕЛЕМЕТРІЇ" << std::endl;
          if (outLen == sizeof(Telemetry)) {
            Telemetry dron;
            // Копіюємо байти у структуру
            std::memcpy(&dron, outPayload, sizeof(Telemetry));

            // Виводимо параметри балістики в консоль
            std::cout << "\n[Парсер] Отримано параметри дрону!" << std::endl;
            std::cout << " -> Час від старту: " << dron.t_ms << std::endl;
            std::cout << " -> Позиція дрона: " << dron.x << "," << dron.y << std::endl;
            std::cout << " -> Висота дрона: " << dron.z << std::endl;
            std::cout << " -> Швидкість vx, vy дрона: " << dron.vx << "," << dron.vy << std::endl;
            std::cout << " -> Швидкість speed дрона: " << dron.speed << std::endl;
            std::cout << " -> Курс дрона: " << dron.dir << std::endl;
            std::cout << " -> State дрона: " << dron.state << std::endl;
          }
        }

        // позиція цілі
        if (outType == 0x02) {
          std::cout << " -> Оброблено пакет ЦІЛІ" << std::endl;
          if (outLen == sizeof(TargetPos)) {
            TargetPos target;

            std::memcpy(&target, outPayload, sizeof(TargetPos));

            // Виводимо параметри балістики в консоль
            std::cout << "\n[Парсер] Отримано параметри цілі!" << std::endl;
            std::cout << "ID цілі -> : " << target.id << std::endl;
            std::cout << "X положеня цілі -> : " << target.x << std::endl;
            std::cout << "Y положеня цілі -> : " << target.y << std::endl;
          }
        }

        if (outType == 0x03) {  // PKT_AMMO
          // Перевіряємо, чи розмір payload збігається з розміром нашої структури
          if (outLen == sizeof(AmmoCfg)) {
            AmmoCfg ammo;
            // Копіюємо байти у структуру
            std::memcpy(&ammo, outPayload, sizeof(AmmoCfg));

            // Виводимо параметри балістики в консоль
            std::cout << "\n[Парсер] Отримано параметри боєприпасу!" << std::endl;
            std::cout << " -> Назва: " << ammo.name << std::endl;
            std::cout << " -> Маса (mass): " << ammo.mass << " кг" << std::endl;
            std::cout << " -> Опір (drag): " << ammo.drag << std::endl;
            std::cout << " -> Радіус ураження (hitRadius): " << ammo.hitRadius << " м" << std::endl;
            std::cout << " -> Кількість цілей (nTargets): " << (int)ammo.nTargets << std::endl;

            // ТЕПЕР твоя програма знає константи для балістичного калькулятора!
          }
        }

        // позиція цілі
        if (outType == 0x04) {
          std::cout << " -> Вердикт чекера " << std::endl;
          if (outLen == sizeof(Result)) {
            Result result;

            std::memcpy(&result, outPayload, sizeof(Result));

            // Виводимо параметри балістики в консоль
            std::cout << "\n[Result] Отримано результати попадання!" << std::endl;
            std::cout << "Влучив чи промах -> : " << result.hit << std::endl;
            std::cout << "targetId цілі -> : " << result.targetId << std::endl;
            std::cout << "Промах, м. -> : " << result.miss_m << std::endl;
            std::cout << "Час коли скид спрацював, с. -> : " << result.drop_t_ms << std::endl;
          }
        }
      }
    }

    auto currentTime = std::chrono::high_resolution_clock::now();

    // Наприклад, якщо балістичний калькулятор порахував, що дрон над ціллю:
    if (!already_dropped && currentTime >= nextTimePoint) {
      gpio.pulse_drop();  // Одноразовий імпульс
      already_dropped = true;
    }

    // Маленька пауза, щоб не навантажувати процесор ПК на 100% у порожньому циклі.
    // В реальному симуляторі її замінить крок фізичного рушія (наприклад, usleep(1000) для 1000 Гц)
    usleep(1000);
  }

  // Закриваємо дескриптор перед виходом (хоча в цьому коді цикл нескінченний)
  close(uartFd);
  return 0;
}