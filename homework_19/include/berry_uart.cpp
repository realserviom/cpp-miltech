// ============================================================================
// Raspberry Pi: читає UART і друкує все в консоль.
// Приймає потік від STM32 з прикладу ads1115 (або будь-який інший UART).
//
// Підключення (рівні 3.3 В — сумісні напряму, БЕЗ перехідників):
//   STM32 PA9 (TX) -> RPi GPIO15 RXD (фізичний пін 10)
//   GND STM32      -> GND RPi (наприклад, пін 6)
//
// Увімкнути UART на Raspberry Pi (один раз):
//   sudo raspi-config -> Interface Options -> Serial Port:
//     login shell over serial -> No;  serial port hardware -> Yes; перезавантажити
//
// Збірка і запуск:
//   g++ -O2 -o uart_console uart_console.cpp
//   ./uart_console             (типово /dev/serial0)
//   ./uart_console /dev/ttyUSB0 (якщо через USB-UART перехідник)
// ============================================================================
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char** argv) {
    const char* dev = (argc > 1) ? argv[1] : "/dev/serial0";

    const int fd = open(dev, O_RDONLY | O_NOCTTY);
    if (fd < 0) { perror(dev); return 1; }

    // налаштування порту: 115200, 8N1, "сирий" режим без обробки символів
    termios tio{};
    if (tcgetattr(fd, &tio) != 0) { perror("tcgetattr"); return 1; }
    cfmakeraw(&tio);                  // вимкнути луну, редагування рядка тощо
    cfsetspeed(&tio, B115200);
    tio.c_cflag |= CLOCAL | CREAD;    // ігнорувати модемні сигнали, приймати
    tio.c_cc[VMIN] = 1;               // read повертається щонайменше з 1 байтом
    tio.c_cc[VTIME] = 0;              // без таймауту
    if (tcsetattr(fd, TCSANOW, &tio) != 0) { perror("tcsetattr"); return 1; }
    tcflush(fd, TCIFLUSH);            // викинути сміття, накопичене до старту

    fprintf(stderr, "listening on %s @115200 (Ctrl+C to stop)\n", dev);

    char buf[256];
    for (;;) {
        const ssize_t n = read(fd, buf, sizeof buf);
        if (n > 0) {
            fwrite(buf, 1, size_t(n), stdout);
            fflush(stdout);
        } else if (n < 0) {
            perror("read");
            break;
        }
    }
    close(fd);
    return 0;
}