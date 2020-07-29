
#include "catch.hpp"

#include "hash.hpp"

TEST_CASE("Hash of C string") {
    djb2_hash hash{};

    CHECK(hash("") == 0x1505U);
    CHECK(hash("highway") == 0xd0b345d8e056U);
    CHECK(hash("amenity") == 0xd0b13434b9bcU);
    CHECK(hash("highway=primary") == 0xe6a29d95ad46ba17U);
    CHECK(hash("highway=secondary") == 0x19c05d09afab3e7bU);
    CHECK(hash("some where") == 0x7272f658d26ce8b4U);
}

TEST_CASE("Hash of std::pair of C strings") {
    djb2_hash hash{};

    CHECK(hash(std::make_pair("", "")) == 0x2b5e2U);
    CHECK(hash(std::make_pair("highway", "")) == 0x1ae71c00f4eb53);
    CHECK(hash(std::make_pair("highway", "primary")) == 0xe6a29d95ad46ba17U);
    CHECK(hash(std::make_pair("highway", "secondary")) == 0x19c05d09afab3e7bU);
}

TEST_CASE("C string comparison") {
    eqstr eq{};

    CHECK(eq(nullptr, nullptr));

    const char *str = "foo";
    CHECK(eq(str, str));
    CHECK(eq(str, "foo"));
    CHECK(eq("foo", "foo"));
    CHECK(eq("", ""));
    CHECK_FALSE(eq("foo", "bar"));
    CHECK_FALSE(eq("foo", ""));
}
TEST_CASE("comparison of std::pair of C strings") {
    eqstr eq{};

    CHECK(eq(std::make_pair(nullptr, nullptr), std::make_pair(nullptr, nullptr)));

    const char *str1 = "foo";
    const char *str2 = "bar";
    CHECK(eq(std::make_pair(str1, str2), std::make_pair(str1, str2)));
    CHECK(eq(std::make_pair(str1, str2), std::make_pair("foo", "bar")));
    CHECK(eq(std::make_pair("foo", "bar"), std::make_pair("foo", "bar")));
    CHECK_FALSE(eq(std::make_pair("foo", "baz"), std::make_pair("foo", "bar")));
    CHECK_FALSE(eq(std::make_pair("fo0", "bar"), std::make_pair("foo", "bar")));
}
