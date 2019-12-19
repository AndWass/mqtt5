#include <doctest/doctest.h>
#include <algorithm>

#include <string>
#include "mqtt5/serialize.hpp"
#include "mqtt5/string.hpp"

TEST_CASE("count_iterator: std::copy can count")
{
    mqtt5::count_iterator iter;
    std::string str("hello world");
    iter = std::copy(str.begin(), str.end(), iter);
    REQUIRE(iter.count == str.size());
}

TEST_CASE("string: serialize UTF string")
{
    mqtt5::string str("A\xF0\xAA\x9B\x94");
    mqtt5::count_iterator iter;
    iter = serialize(str, iter);
    REQUIRE(iter.count == 7);    
}