#pragma once
#include "Types.h"

class IConfigLoader {
public:
    virtual void debug(DroneConfig &myDrone) = 0;
    virtual void init(DroneConfig &myDrone) = 0;
    virtual ~IConfigLoader() {}
};