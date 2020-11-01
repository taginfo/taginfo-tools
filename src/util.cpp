
#include "util.hpp"

#include <cstdlib>
#include <limits>
#include <stdexcept>

double get_coordinate(const char* str, double max) {
    if (*str == '\0') {
        throw std::range_error{"coordinate out of range"};
    }

    char* end = nullptr;
    const auto value = std::strtod(str, &end);

    if (*end != '\0' || value > max || value < -max) {
        throw std::range_error{"coordinate out of range"};
    }

    return value;
}

unsigned int get_uint(const char* str) {
    if (*str == '\0') {
        throw std::range_error{"invalid value"};
    }

    char* end = nullptr;
    const auto value = std::strtoul(str, &end, 10);

    if (*end != '\0' || value > std::numeric_limits<unsigned int>::max()) {
        throw std::range_error{"invalid value"};
    }

    return static_cast<unsigned int>(value);
}

std::string time_string(osmium::Timestamp timestamp) {
    std::string ts{timestamp.to_iso_all()};
    assert(ts.size() > 10U);
    ts.replace(10, 1, " "); // replace "T" with space
    ts.pop_back(); // remove "Z" at end
    return ts;
}

