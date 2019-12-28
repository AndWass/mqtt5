#include <doctest/doctest.h>

#include "mqtt5/type/binary.hpp"

TEST_CASE("binary: Default construction")
{
    mqtt5::type::binary bin;
    REQUIRE(bin.empty());
    REQUIRE(bin.size() == 0);
}

TEST_CASE("binary: Construction of too big throws")
{
    std::vector<std::uint8_t> vec(65536, 1);
    SUBCASE("vector copy")
    {
        REQUIRE_THROWS_AS([&]() {
            mqtt5::type::binary bin(vec);
        }(), std::length_error);
    }
    SUBCASE("vector move")
    {
        REQUIRE_THROWS_AS([&]() {
            mqtt5::type::binary bin(std::move(vec));
        }(), std::length_error);
    }
    SUBCASE("iterator construction")
    {
        REQUIRE_THROWS_AS([&]() {
            mqtt5::type::binary bin(vec.begin(), vec.end());
        }(), std::length_error);
    }
}

TEST_CASE("binary: insert that grows too big")
{
    std::vector<std::uint8_t> vec(65535, 1);
    mqtt5::type::binary bin(vec);
    SUBCASE("insert 1 value")
    {
        REQUIRE_THROWS_AS(bin.insert(bin.end(), 10), std::length_error);
    }
    SUBCASE("insert initializer list")
    {
        mqtt5::type::binary bin2(vec.begin(), vec.begin() + 65530);
        REQUIRE_THROWS_AS(bin2.insert(bin2.end(), {1,2,3,4,5,6,7,8}), std::length_error);
    }
    SUBCASE("insert initializer iterator range")
    {
        mqtt5::type::binary bin2(vec.begin(), vec.begin() + 65530);
        REQUIRE_THROWS_AS(bin2.insert(bin2.end(), vec.begin(), vec.begin() + 10), std::length_error);
    }
    SUBCASE("push_back")
    {
        REQUIRE_THROWS_AS(bin.push_back(10), std::length_error);
    }
    SUBCASE("push_front")
    {
        REQUIRE_THROWS_AS(bin.push_front(10), std::length_error);
    }
}

TEST_CASE("binary: serialize")
{
    mqtt5::type::binary bin({1,2,3,4,5});
    std::vector<std::uint8_t> out_vec;
    auto iter = serialize(bin, std::back_inserter(out_vec));
    REQUIRE(out_vec.size() == 7);
    REQUIRE(out_vec[0] == 0);
    REQUIRE(out_vec[1] == 5);
    REQUIRE(out_vec[2] == 1);
    REQUIRE(out_vec[3] == 2);
    REQUIRE(out_vec[4] == 3);
    REQUIRE(out_vec[5] == 4);
    REQUIRE(out_vec[6] == 5);
}
