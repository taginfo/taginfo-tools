
#include "catch.hpp"

#include "util.hpp"

TEST_CASE("Valid coordinates for get_coordinate") {
    CHECK(get_coordinate("0.0", 90.0) == Approx(0.0));
    CHECK(get_coordinate("1.02", 90.0) == Approx(1.02));
    CHECK(get_coordinate("90.0", 90.0) == Approx(90.0));
    CHECK(get_coordinate("-180.0", 180.0) == Approx(-180.0));
}

TEST_CASE("Invalid coordinates for get_coordinate") {
    constexpr const double max = 90.0;
    CHECK_THROWS(get_coordinate("", max));
    CHECK_THROWS(get_coordinate("foo", max));
    CHECK_THROWS(get_coordinate("3abc", max));
    CHECK_THROWS(get_coordinate("500.0", max));
    CHECK_THROWS(get_coordinate("300", max));
    CHECK_THROWS(get_coordinate("-1000", max));
    CHECK_THROWS(get_coordinate("238427432238492347983432.73", max));
}

