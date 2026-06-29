#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include "drone_link.h"  // Переконайся, що цей файл лежить поруч

using namespace dlink;

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
  // const char* uartDevice = "/dev/ttyAMA1";
  // sudo apt update && sudo apt install -y socat - встановлення
  // socat -d -d pty,raw,echo=0,link=/tmp/ttyA pty,raw,echo=0,link=/tmp/ttyB - налаштування
  const char* uartDevice = "/tmp/ttyA";  // це socat
  int uartFd = openUart(uartDevice);

  if (uartFd < 0) {
    return 1;  // Виходимо, якщо порт не відкрився
  }

  std::cout << "UART порт " << uartDevice << " успішно налаштовано на 115200 бод." << std::endl;
  std::cout << "Запуск головного циклу опитування..." << std::endl;

  // 2. Створюємо екземпляр парсера та змінні для вихідних даних
  Parser parser;

  uint8_t outType = 0;
  uint8_t outLen = 0;
  uint8_t outPayload[260];

  // Головний робочий цикл програми
  while (true) {
    // --- 3. ПАРАЛЕЛЬНА КОРИСНА РОБОТА ---
    // Тут твій дрон може рахувати фізику, траєкторію чи балістику.
    // Завдяки неблокуючому UART, цей код виконується без затримок.

    // --- 4. НЕБЛОКУЮЧЕ ЧИТАННЯ З UART ---
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