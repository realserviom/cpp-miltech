#include "debug.h"
#include "types.h"
#include <vector>
#include <fstream>

bool loadNrkSteps(std::vector<NrkStep>& steps, const std::string& fileName)
{
  std::ifstream file(fileName);

  if (!file.is_open()) {
    printf("Помилка: Не вдалося відкрити файл %s\n", fileName.c_str());
    return false;
  }

  NrkStep temp_step;

  while (file >> temp_step.timestamp_ms >> temp_step.fl_ticks >> temp_step.fr_ticks >> temp_step.bl_ticks >> temp_step.br_ticks) {
    steps.push_back(temp_step);
  }

  return true;
}