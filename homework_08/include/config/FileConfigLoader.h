#pragma once

#include "AbstractConfigProvider.h"
#include <string>

class FileConfigLoader : public AbstractConfigProvider {
private:
    static constexpr int MAX_AMMO_TYPES = 5;
    std::string m_filePath_droneConfig;
    std::string m_filePath_listAmmo;

public:
    FileConfigLoader(const std::string& filePathDroneConfig, const std::string& filePathListAmmo);

    void loadAmmo() override;
    void tunningDrone(DroneConfig &myDrone) override;
    ~FileConfigLoader() override = default;
};




