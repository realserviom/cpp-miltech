#pragma once
#include "AbstractTargetProvider.h"
#include <string>

class JsonTargetProvider : public AbstractTargetProvider {
private:
public:
  JsonTargetProvider(const std::string& jsonFilePath);

  Coord getTargetPosition(const int target, const float time) override;

  void loadTargets() override;

  ~JsonTargetProvider();

  int getIterationByCounter(const int& counter);

  //  Coord getTargetNextPos(int& targetId, const int& counter);

  int getIterationByTime(float time, const float& arrayTimeStep);

  Coord getTargetPositionInIteration(const int& index, int& timeIteration);

  int getNextIteration(int& iteration);

  Coord getTargetPositionInCounter(int& targetId, const int& counter);
};
