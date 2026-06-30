/******************************************************************************

Welcome to GDB Online.
GDB online is an online compiler and debugger tool for C, C++, Python, Java, PHP, Ruby, Perl,
C#, OCaml, VB, Swift, Pascal, Fortran, Haskell, Objective-C, Assembly, HTML, CSS, JS, SQLite, Prolog.
Code, Compile, Run and Debug online from anywhere in world.

*******************************************************************************/
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <cmath>
#include "Types.h"
#include "config/ComponentFactory.h"
#include "MissionProcessor.h"
#include <iomanip>
#include "GPIOController.h"
#include "functions.h"
#include "UARTProcessor.h"
#include "drone_link.h"

// Визначення константи Пі, якщо її немає в cmath
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main()
{
  try {
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

    // Створюємо uartProcessor який буде працювати в окремому потоці і обробляти дані з UART

    auto uartProcessor = std::make_shared<UARTProcessor>(uartFd);

    // Створюємо аналитичний балістичний калькулятор табличного типу
    std::shared_ptr<IBallisticSolver> analyticalSolver = createSolver(SolverType::TABLE);

    std::shared_ptr<IConfigLoader> configLoader = createLoader(LoaderType::FILE, "../data/config.json");

    MissionProcessor processor(uartProcessor, analyticalSolver, configLoader, uartFd);

    // std::cout << "Запуск головного циклу опитування..." << std::endl;

    // // START — на ніжці підняти вольтаж на 1 одразу на старті й тримати. (сигнал про готовність дрона)
    // gpio.set_start(1);

    // Головний цикл цього потоку (Балістичний калькулятор + Скид)
    // while (true) {

    //   // КРОК C: Логіка виконання скиду
    //   auto currentTime = std::chrono::high_resolution_clock::now();
    //   if (!already_dropped && currentTime >= nextTimePoint) {
    //     // Викликаємо GPIO з цього потоку! Потік UART при цьому не блокується на 80 мс
    //     gpio.pulse_drop();
    //     already_dropped = true;
    //     std::cout << "[Ballistics] Команду DROP виконано!" << std::endl;
    //   }

    //   // Крок балістичного циклу (наприклад, 100 Гц або 1000 Гц)
    //   usleep(10000);  // 10 мс
    // }

    uartProcessor->start();
    processor.start();

    while (!uartProcessor->isThreadReady() || !processor.isThreadReady()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    processor.missionThread.join();

    // curMyDrone.stop();
    uartProcessor->stop();
  }
  catch (const std::runtime_error& e) {
    std::cout << e.what() << std::endl;
    return -1;
  }

  return 0;
}