
#ifndef BALLISTICS_HPP
#define BALLISTICS_HPP

#include "types.hpp"
#include <stdlib.h>
#include <string>

auto find_ammunition(std::string_view name) -> const Ammunition*;

auto read_input_data(std::string_view filename, DroneInput& data) -> bool;

// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers, readability-identifier-length)
auto calculate_dist(double t, double v0, double m, double d, double l, double g = kGravit) -> double;
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers, readability-identifier-length)

auto calculate_length(float target_x, float target_y, float x_d, float y_d) -> double;

void save_fire_coordinates(std::string& output_path, double fire_x, double fire_y, double xd_i = 0, double yd_i = 0);

auto calculate_flight_time(const Ammunition* selected_ammo, const DroneInput& input, double gravit = kGravit) -> double;

#endif