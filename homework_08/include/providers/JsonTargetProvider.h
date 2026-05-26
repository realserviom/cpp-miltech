#pragma once
#include "AbstractTargetProvider.h"
#include <string>

class JsonTargetProvider : public AbstractTargetProvider {
private:
    std::string m_filePath;

public:
    JsonTargetProvider(const std::string& jsonFilePath);

    void loadTargets() override;
    ~JsonTargetProvider() override;
};
