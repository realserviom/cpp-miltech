#pragma once
#include <memory>

class Drone;  // ГАРАНТІЯ БЕЗПЕКИ: просто кажемо, що такий клас є

class IDroneState {
public:
    virtual ~IDroneState() = default;

    virtual std::unique_ptr<IDroneState> execute(Drone &curMyDrone) = 0;

    virtual const std::string name() const = 0;

    virtual int id() const = 0;
};
