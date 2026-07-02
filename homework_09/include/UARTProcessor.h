#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include "Types.h"
#include "drone_link.h"
#include <optional>
#include <unordered_map>

class UARTProcessor {
private:
    int uartFd;
    dlink::Parser parser;

    std::thread workerThread;
    std::atomic<bool> running;
    std::atomic<bool> isReady{false};

    std::mutex dataMutex;

    // Сховища для даних
    dlink::Telemetry currentTelemetry;
    dlink::TargetPos currentTarget;
    dlink::AmmoCfg currentAmmo;
    dlink::Result currentResult;

    std::unordered_map<uint8_t, Coord> velocityTargets;
    std::unordered_map<uint8_t, Coord> positionTargets;
    std::unordered_map<uint8_t, std::chrono::time_point<std::chrono::high_resolution_clock>> lastTelemetryTime;

    // Прапорці наявності даних
    bool hasTelemetry = false;
    bool hasAmmo = false;
    bool hasResult = false;
    bool hasTarget = false;

    void processLoop();

  public:
    UARTProcessor(int fd);
    ~UARTProcessor();

    void start();
    void stop();

    // Повертає вказівник на конфіг боєприпасу (якщо прийшов, інакше nullptr)
    const dlink::AmmoCfg* getAmmoConfigPtr();

    // Повертає true, якщо телеметрія є, і копіює її у вашу змінну
    bool getTelemetry(DroneTelemetry& outTelemetry);

    std::optional<Target> getTargetPosition(const uint8_t targetId);

    // Повертає true, якщо прийшов вердикт від чекера
    bool getResult(dlink::Result& outResult);

    bool isThreadReady() const;

    void sendControl(float accel, float turnRate);

    bool getTargetPosition2(dlink::TargetPos& target);
};