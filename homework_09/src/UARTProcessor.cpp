#include "UARTProcessor.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

UARTProcessor::UARTProcessor(int fd)
  : uartFd(fd)
  , running(false)
{
}

UARTProcessor::~UARTProcessor() {
    stop();
}

void UARTProcessor::start() {
  if (!running) {
    running = true;
    workerThread = std::thread(&UARTProcessor::processLoop, this);
  }
}

void UARTProcessor::stop() {
  if (running) {
    running = false;
    if (workerThread.joinable()) {
      workerThread.join();
    }
  }
}

bool UARTProcessor::isThreadReady() const
{
  return isReady;
}

void UARTProcessor::processLoop() {
    uint8_t incomingByte;
    uint8_t outType;
    uint8_t outPayload[512];
    uint8_t outLen;

    while (running) {
      // Читаємо з UART, поки є дані
      while (running && read(uartFd, &incomingByte, 1) > 0) {
        if (parser.feed(incomingByte, outType, outPayload, outLen)) {
          // Захищаємо запис м'ютексом
          std::lock_guard<std::mutex> lock(dataMutex);

          if (outType == 0x01 && outLen == sizeof(dlink::Telemetry)) {
            std::memcpy(&currentTelemetry, outPayload, sizeof(dlink::Telemetry));
            hasTelemetry = true;
          }
          else if (outType == 0x02 && outLen == sizeof(dlink::TargetPos)) {
            auto time = std::chrono::high_resolution_clock::now();

            std::memcpy(&currentTarget, outPayload, sizeof(dlink::TargetPos));

            Coord newPosition = Coord{currentTarget.x, currentTarget.y};
            velocityTargets[currentTarget.id] = lastTelemetryTime[currentTarget.id].time_since_epoch().count() == 0
                                                  ? Coord{0, 0}
                                                  : (newPosition - positionTargets[currentTarget.id]) /
                                                      std::chrono::duration<float>(time - lastTelemetryTime[currentTarget.id]).count();
            positionTargets[currentTarget.id] = newPosition;
            lastTelemetryTime[currentTarget.id] = time;
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
      usleep(1000);  // 1 мс пауза
    }
}

// Реалізація безпечних методів доступу (потокобезпечні гетери)

const dlink::AmmoCfg* UARTProcessor::getAmmoConfigPtr()
{
  std::lock_guard<std::mutex> lock(dataMutex);
  return hasAmmo ? &currentAmmo : nullptr;
}

bool UARTProcessor::getTelemetry(dlink::Telemetry& outTelemetry)
{
  std::lock_guard<std::mutex> lock(dataMutex);
  if (!hasTelemetry)
    return false;
  outTelemetry = currentTelemetry;
  return true;
}

Target UARTProcessor::getTargetPosition(const int targetId)
{
  std::lock_guard<std::mutex> lock(dataMutex);
  Target targetPos;
  targetPos.pos = positionTargets[targetId];
  targetPos.velocity = velocityTargets[targetId];

  DEBUG("Target " << std::to_string(targetId) << " pos: (" << targetPos.pos.x << ", " << targetPos.pos.y << "), velocity: ("
                  << targetPos.velocity.x << ", " << targetPos.velocity.y << ")");
  return targetPos;
}

bool UARTProcessor::getResult(dlink::Result& outResult)
{
  std::lock_guard<std::mutex> lock(dataMutex);
  if (!hasResult)
    return false;
  outResult = currentResult;
  return true;
}