#include <algorithm>
#include <doctest/doctest.h>

#include "mqtt5/type/integer.hpp"
#include "mqtt5/type/serialize.hpp"
#include "mqtt5/type/string.hpp"
#include <string>

TEST_CASE("count_iterator: std::copy can count") {
    mqtt5::type::count_iterator iter;
    std::string str("hello world");
    iter = std::copy(str.begin(), str.end(), iter);
    REQUIRE(iter.count == str.size());
}

TEST_CASE("count_iterator: count length of UTF string") {
    mqtt5::type::string str("A\xF0\xAA\x9B\x94");
    mqtt5::type::count_iterator iter;
    iter = serialize(str, iter);
    REQUIRE(iter.count == 7);
}

TEST_CASE("count_iterator: count length of variable length integer") {
    mqtt5::type::varlen_integer vi(100000000);
    mqtt5::type::count_iterator iter;
    iter = serialize(vi, iter);
    REQUIRE(iter.count == 4);
}