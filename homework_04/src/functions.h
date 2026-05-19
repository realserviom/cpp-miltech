#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "types.h"
#include <cstdio>
#include "constants.h"
#include <vector>
#include <iostream>


bool loadNrkSteps(std::vector<NrkStep> &steps, const std::string& fileName);

void calculateOdometry(const std::vector<NrkStep>& steps, double wheel_radius, double wheelbase, int ticks_per_rev);

#endif