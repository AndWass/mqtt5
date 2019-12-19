#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <mqtt5/integer.hpp>
#include <array>
#include <nonstd/span.hpp>

TEST_CASE("mqtt5: integer default-construction construction")
{
    mqtt5::integer<std::uint16_t> i16;
    REQUIRE_EQ(i16, 0);
    REQUIRE_FALSE(i16 != 0);
    REQUIRE(i16 <= 0);
    REQUIRE(i16 >= 0);
    REQUIRE_FALSE(i16 < 0);
    REQUIRE_FALSE(i16 > 0);

    REQUIRE_EQ(0, i16);
    REQUIRE_FALSE(0 != i16);
    REQUIRE(0 <= i16);
    REQUIRE(0 >= i16);
    REQUIRE_FALSE(0 < i16);
    REQUIRE_FALSE(0 > i16);
}

TEST_CASE("mqtt5: integer-integer comparison")
{
    mqtt5::integer<std::uint16_t> i16;
    mqtt5::integer<std::uint32_t> i32;

    REQUIRE_EQ(i16, i32);
    REQUIRE_FALSE(i16 != i32);
    REQUIRE(i16 <= i32);
    REQUIRE(i16 >= i32);
    REQUIRE_FALSE(i16 < i32);
    REQUIRE_FALSE(i16 > i32);

    REQUIRE_EQ(i32, i16);
    REQUIRE_FALSE(i32 != i16);
    REQUIRE(i32 <= i16);
    REQUIRE(i32 >= i16);
    REQUIRE_FALSE(i32 < i16);
    REQUIRE_FALSE(i32 > i16);
}

TEST_CASE("mqtt5: span<const std::uint8_t> construction")
{
    const std::array<std::uint8_t, 2> data{0xab, 0xcd};
    SUBCASE("direct span construction")
    {
        nonstd::span<const std::uint8_t> dataspan(data);
        mqtt5::integer<std::uint16_t> i16(dataspan);
        REQUIRE(i16 == 0xabcd);
    }
    SUBCASE("indirect span construction")
    {
        mqtt5::integer<std::uint16_t> i16(data);
        REQUIRE(i16 == 0xabcd);
    }
}

TEST_CASE("mqtt5: Integer32 serialization")
{
    std::array<std::uint8_t, 4> data;
    mqtt5::integer<std::uint32_t> i32(0x12345678);
    auto end = serialize(i32, data.begin());
    REQUIRE(end == data.end());
    REQUIRE(data[0] == 0x12);
    REQUIRE(data[1] == 0x34);
    REQUIRE(data[2] == 0x56);
    REQUIRE(data[3] == 0x78);
}

TEST_CASE("mqtt5: variable length integer serialization")
{
    std::array<std::uint8_t, 4> data;
    SUBCASE("encode 0")
    {
        mqtt5::varlen_integer i32(0);
        auto end = serialize(i32, data.begin());
        REQUIRE(end == (data.begin() + 1));
        REQUIRE(data[0] == 0x00);
    }
    SUBCASE("encode 127")
    {
        mqtt5::varlen_integer i32(127);
        auto end = serialize(i32, data.begin());
        REQUIRE(end == (data.begin() + 1));
        REQUIRE(data[0] == 127);
    }
    SUBCASE("encode 128")
    {
        mqtt5::varlen_integer i32(128);
        auto end = serialize(i32, data.begin());
        REQUIRE(end == (data.begin() + 2));
        REQUIRE(data[0] == 0x80);
        REQUIRE(data[1] == 1);
    }
    SUBCASE("encode 16383")
    {
        mqtt5::varlen_integer i32(16383);
        auto end = serialize(i32, data.begin());
        REQUIRE(end == (data.begin() + 2));
        REQUIRE(data[0] == 0xff);
        REQUIRE(data[1] == 0x7f);
    }
}