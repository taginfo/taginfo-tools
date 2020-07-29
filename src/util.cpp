
#include "util.hpp"

#include <cstdlib>
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
