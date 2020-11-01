#pragma once

#include <osmium/osm/timestamp.hpp>

#include <string>

double get_coordinate(const char* str, double max);
unsigned int get_uint(const char* str);
std::string time_string(osmium::Timestamp timestamp);

