#pragma once
#include "AbstractTargetProvider.h"
#include <string>

class JsonTargetProvider : public AbstractTargetProvider {
private:
public:
  JsonTargetProvider(const std::string& jsonFilePath);

  ~JsonTargetProvider() override = default;

  void setArrayTimeStep(float time) override;

  Coord getTargetPosition(const int target, const float time) override;

  void loadTargets() override;

  //  Coord getTargetNextPos(int& targetId, const int& counter);

  int getIterationByCounter(const int& counter);

  int getIterationByTime(float time, const float& arrayTimeStep);

  Coord getTargetPositionInIteration(const int& index, int& timeIteration);

  int getNextIteration(int& iteration);

  Coord getTargetPositionInCounter(int& targetId, const int& counter);
};
