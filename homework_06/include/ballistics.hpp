
#ifndef BALLISTICS_HPP
#define BALLISTICS_HPP

#include "types.hpp"
#include <stdlib.h>
#include <string>

const Ammunition* findAmmunition(const char* name);

bool readInputData(const char* filename, DroneInput& data);

double calculateDist(double t, double V0, double m, double d, double l, double g = 9.81);

double calculateLength(float targetX, float targetY, float xd, float yd);

void saveFireCoordinates(std::string outputPath, double fireX, double fireY, double xd_i = 0, double yd_i = 0);

double calculateFlightTime(const Ammunition* selectedAmmo, const DroneInput& input, double gravit = 9.81);

#endif