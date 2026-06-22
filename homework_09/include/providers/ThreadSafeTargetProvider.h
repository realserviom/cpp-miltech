#pragma once
#include "interfaces/ITargetProvider.h"
#include <memory>
#include <mutex>
#include "AbstractTargetProvider.h"
#include <string>


class ThreadSafeTargetProvider : public AbstractTargetProvider {
private:
public:

  ThreadSafeTargetProvider(const std::string& jsonFilePath);

  Coord getTargetPosition(const int target, const float time) override;

  void loadTargets() override;

  ~ThreadSafeTargetProvider();

  // М'ютекс для синхронізації доступу з різних потоків
  mutable std::mutex m_mutex;

  void setArrayTimeStep(float time) override;
  float getArrayTimeStep() const override;

};