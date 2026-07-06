#pragma once
#include "Types.h"
#include "interfaces/ITargetProvider.h"
#include <atomic>

class AbstractTargetProvider : public ITargetProvider {
protected:
  // кількість цілей
  int m_targetCount = 0;
  // кількість часових кроків для кожної цілі
  int m_timeSteps = 0;

  // кількість ітерацій в одному часовому кроці
  int m_numberCounterInTimeSpot;

  std::vector<std::vector<Coord>> m_targets;

  float arrayTimeStep = 0.0f;
  float targetTimeStep = 0.0f;
  float timeScale = 1.0f;

  std::atomic<bool> running{false};
  std::atomic<bool> isReady{false};

public:
  std::string m_filePath;

  explicit AbstractTargetProvider(const std::string &jsonFilePath)
    : m_filePath(jsonFilePath)
  {
  }

  ~AbstractTargetProvider() override = default;

  virtual void loadTargets() = 0;

  int getTargetCount() override;

  virtual void init(int &numberCounterInTimeSpot) override;

   std::vector<std::vector<Coord>> getTargets() override;

  void setArrayTimeStep(float time) override;

  float getArrayTimeStep() const override;

  void setTargetTimeStep(float time) override;

  float getTargetTimeStep() const override;

  void setTimeScale(float time) override;

  float getTimeScale() const override;

  void setRunningTrue() override;
};