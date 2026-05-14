
#ifndef BALLISTICS_HPP
#define BALLISTICS_HPP

#include "types.hpp"
#include <stdlib.h>
#include <string>

auto findAmmunition(std::string_view name) -> const Ammunition*;

auto readInputData(std::string_view filename, DroneInput& data) -> bool;

auto calculateDist(double t, double V0, double m, double d, double l, double g = GRAVIT) -> double;

auto calculateLength(float targetX, float targetY, float xd, float yd) -> double;

void saveFireCoordinates(std::string outputPath, double fireX, double fireY, double xd_i = 0, double yd_i = 0);

auto calculateFlightTime(const Ammunition* selectedAmmo, const DroneInput& input, double gravit = GRAVIT) -> double;

#endif