#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include "drone_link.h"  // Переконайся, що цей файл лежить поруч


class UARTProcessor {
private:
    int uartFd;
    dlink::Parser parser;

    std::thread workerThread;
    std::atomic<bool> isRunning;
    std::mutex dataMutex;

    // Сховища для даних
    dlink::Telemetry currentTelemetry;
    dlink::TargetPos currentTarget;
    dlink::AmmoCfg currentAmmo;
    dlink::Result currentResult;

    // Прапорці наявності даних
    bool hasTelemetry = false;
    bool hasTarget = false;
    bool hasAmmo = false;
    bool hasResult = false;

    void processLoop();

public:
    UARTProcessor(int fd);
    ~UARTProcessor();

    void start();
    void stop();

    // --- БЕЗПЕЧНІ МЕТОДИ ДЛЯ ІНШИХ ПОТОКІВ ---
    
    // Повертає вказівник на конфіг боєприпасу (якщо прийшов, інакше nullptr)
    const dlink::AmmoCfg* getAmmoConfigPtr();

    // Повертає true, якщо телеметрія є, і копіює її у вашу змінну
    bool getTelemetry(dlink::Telemetry& outTelemetry);

    // Повертає true, якщо ціль є, і копіює її у вашу змінну
    bool getTarget(dlink::TargetPos& outTarget);

    // Повертає true, якщо прийшов вердикт від чекера
    bool getResult(dlink::Result& outResult);
};