#pragma once

#include "AbstractConfigProvider.h"
#include <string>

class FileConfigLoader : public AbstractConfigProvider {
private:
  std::string m_filePath_droneConfig;

public:
  FileConfigLoader(const std::string& filePathDroneConfig);

  void tunningDrone(DroneConfig& myDrone) override;
  ~FileConfigLoader() override = default;
};




