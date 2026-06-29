#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include "drone_link.h"  // Переконайся, що цей файл лежить поруч
#include <fstream>

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

    // Нам треба створити файл 24 (або line24) в папці банки
    // std::ofstream автоматично створить файл, якщо його немає
    std::ofstream file("/tmp/my_gpio_bank/24");

    if (file.is_open()) {
      file << value;  // Записуємо '1' або '0'
      file.close();
      std::cout << "[MOCK GPIO] Створено файл і записано START = " << value << std::endl;
    }
    else {
      // Якщо не знайшло '24', спробуємо створити 'line24'
      std::ofstream file_alt("/tmp/my_gpio_bank/line24");
      if (file_alt.is_open()) {
        file_alt << value;
        file_alt.close();
        std::cout << "[MOCK GPIO] Створено файл і записано line24 = " << value << std::endl;
      }
      else {
        std::cerr << "[MOCK GPIO] Помилка: не вдалося створити файл лінії в /tmp/my_gpio_bank/" << std::endl;
      }
    }
#endif
  }

  void pulse_drop()
  {
    std::cout << "[GPIO] Виклик команди DROP!" << std::endl;
#if REAL_GPIO
    if (is_ready)
      gpiod_line_set_value(drop_line, 1);
    usleep(80000);  // тримаємо 75 мс (в межах 50-100 мс)
    if (is_ready)
      gpiod_line_set_value(drop_line, 0);
#else
    std::cout << "[MOCK GPIO] Лiнiя DROP -> 1 (Імпульс 75 мс почався)" << std::endl;
    usleep(80000);
    std::cout << "[MOCK GPIO] Лiнiя DROP -> 0 (Імпульс завершено)" << std::endl;
#endif
  }
};

// Функція налаштування UART, яку ми розбирали
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
  // 1. Відкриваємо порт.
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

        // Наприклад, якщо балістичний калькулятор порахував, що дрон над ціллю:
        if (!already_dropped) {
          gpio.pulse_drop();  // Одноразовий імпульс
          already_dropped = true;
        }
      }
    }

    // Маленька пауза, щоб не навантажувати процесор ПК на 100% у порожньому циклі.
    // В реальному симуляторі її замінить крок фізичного рушія (наприклад, usleep(1000) для 1000 Гц)
    usleep(1000);
  }

  // Закриваємо дескриптор перед виходом (хоча в цьому коді цикл нескінченний)
  close(uartFd);
  return 0;
}