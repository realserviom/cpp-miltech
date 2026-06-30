#include "UARTProcessor.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

UARTProcessor::UARTProcessor(int fd) : uartFd(fd), isRunning(false) {}

UARTProcessor::~UARTProcessor() {
    stop();
}

void UARTProcessor::start() {
    if (!isRunning) {
        isRunning = true;
        workerThread = std::thread(&UARTProcessor::processLoop, this);
    }
}

void UARTProcessor::stop() {
    if (isRunning) {
        isRunning = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
}

void UARTProcessor::processLoop() {
    uint8_t incomingByte;
    uint8_t outType;
    uint8_t outPayload[512];
    uint8_t outLen;

    while (isRunning) {
        // Читаємо з UART, поки є дані
        while (isRunning && read(uartFd, &incomingByte, 1) > 0) {
            if (parser.feed(incomingByte, outType, outPayload, outLen)) {
                
                // Захищаємо запис м'ютексом
                std::lock_guard<std::mutex> lock(dataMutex);

                if (outType == 0x01 && outLen == sizeof(dlink::Telemetry)) {
                    std::memcpy(&currentTelemetry, outPayload, sizeof(dlink::Telemetry));
                    hasTelemetry = true;
                }
                else if (outType == 0x02 && outLen == sizeof(dlink::TargetPos)) {
                    std::memcpy(&currentTarget, outPayload, sizeof(dlink::TargetPos));
                    hasTarget = true;
                }
                else if (outType == 0x03 && outLen == sizeof(dlink::AmmoCfg)) {
                    std::memcpy(&currentAmmo, outPayload, sizeof(dlink::AmmoCfg));
                    hasAmmo = true;
                }
                else if (outType == 0x04 && outLen == sizeof(dlink::Result)) {
                    std::memcpy(&currentResult, outPayload, sizeof(dlink::Result));
                    hasResult = true;
                }
            }
        }
        usleep(1000); // 1 мс пауза
    }
}

// Реалізація безпечних методів доступу (потокобезпечні гетери)

const AmmoCfg* UARTProcessor::getAmmoConfigPtr() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return hasAmmo ? &currentAmmo : nullptr;
}

bool UARTProcessor::getTelemetry(Telemetry& outTelemetry) {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (!hasTelemetry) return false;
    outTelemetry = currentTelemetry;
    return true;
}

bool UARTProcessor::getTarget(TargetPos& outTarget) {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (!hasTarget) return false;
    outTarget = currentTarget;
    return true;
}

bool UARTProcessor::getResult(Result& outResult) {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (!hasResult) return false;
    outResult = currentResult;
    return true;
}