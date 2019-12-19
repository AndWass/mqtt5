#include <doctest/doctest.h>

#include <mqtt5/string.hpp>

TEST_CASE("string: rejects u+0000")
{
    char ch = '\0';
    REQUIRE_THROWS_AS(mqtt5::string(&ch, &ch+1), std::invalid_argument);
}

TEST_CASE("string: rejects u+d800 up to u+dfff")
{
    for(int i=0xA0; i<= 0xBF; i++)
    {
        for(int j=0x80; j<=0xBF; j++)
        {
            REQUIRE_THROWS_AS(mqtt5::string({0xED, i, j}), std::invalid_argument);
        }
    }
}

TEST_CASE("string: serialize UTF string")
{
    mqtt5::string str("A\xF0\xAA\x9B\x94");
    std::array<std::uint8_t, 16> serialized;
    auto iter = serialize(str, serialized.begin());
    REQUIRE(serialized[0] == 0);
    REQUIRE(serialized[1] == 5);
    REQUIRE(serialized[2] == 'A');
    REQUIRE(serialized[3] == 0xF0);
    REQUIRE(serialized[4] == 0xAA);
    REQUIRE(serialized[5] == 0x9B);
    REQUIRE(serialized[6] == 0x94);
    REQUIRE(iter == serialized.begin() + 7);
}

TEST_CASE("string: serialize ascii string")
{
    mqtt5::string str("hello");
    std::array<std::uint8_t, 16> serialized;
    auto iter = serialize(str, serialized.begin());
    REQUIRE(serialized[0] == 0);
    REQUIRE(serialized[1] == 5);
    REQUIRE(serialized[2] == 'h');
    REQUIRE(serialized[3] == 'e');
    REQUIRE(serialized[4] == 'l');
    REQUIRE(serialized[5] == 'l');
    REQUIRE(serialized[6] == 'o');
    REQUIRE(iter == serialized.begin() + 7);
}