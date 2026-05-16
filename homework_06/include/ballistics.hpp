
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

auto calculate_length(DroneInput& input) -> double;

void save_fire_coordinates(std::string& output_path, DropPoints& points);

auto calculate_flight_time(const Ammunition* selected_ammo, const DroneInput& input, double gravit = kGravit) -> double;

auto calculate_drop_points(const DroneInput& input, double dist, double length, double epsilon) -> DropPoints;

#endif