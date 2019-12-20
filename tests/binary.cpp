#include <doctest/doctest.h>

#include "mqtt5/binary.hpp"

TEST_CASE("binary: Default construction")
{
    mqtt5::binary bin;
    REQUIRE(bin.empty());
    REQUIRE(bin.size() == 0);
}

TEST_CASE("binary: Construction of too big throws")
{
    std::vector<std::uint8_t> vec(65536, 1);
    SUBCASE("vector copy")
    {
        REQUIRE_THROWS_AS([&]() {
            mqtt5::binary bin(vec);
        }(), std::length_error);
    }
    SUBCASE("vector move")
    {
        REQUIRE_THROWS_AS([&]() {
            mqtt5::binary bin(std::move(vec));
        }(), std::length_error);
    }
    SUBCASE("iterator construction")
    {
        REQUIRE_THROWS_AS([&]() {
            mqtt5::binary bin(vec.begin(), vec.end());
        }(), std::length_error);
    }
}
