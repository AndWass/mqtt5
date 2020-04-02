
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <cstdint>

#include <p0443_v2/sink_receiver.hpp>

#include <doctest/doctest.h>
#include <mqtt5/protocol/fixed_int.hpp>
#include <mqtt5/protocol/inplace_deserializer.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include "vector_serialize.hpp"

namespace prot = mqtt5::protocol;
namespace tran = mqtt5::transport;

TEST_CASE("fixed_int: single byte deserialized")
{
    std::vector<std::uint8_t> data{0xa9};
    auto val = prot::fixed_int<std::uint8_t>::deserialize(tran::buffer_data_fetcher(data));

    REQUIRE(val == 0xa9);
}

TEST_CASE("fixed_int: 2-byte type deserialized")
{
    std::vector<std::uint8_t> data{0xa9, 0xc2};
    int value = prot::fixed_int<std::uint16_t>::deserialize(tran::buffer_data_fetcher(data));
    
    REQUIRE(value == 0xa9c2);
}

TEST_CASE("fixed_int: two byte serialized")
{
    std::uint16_t number = 0xa9c2;
    std::vector<std::uint8_t> vector;
    prot::fixed_int<std::uint16_t>::serialize(number, [&](auto b) {
        vector.push_back(b);
    });
    REQUIRE(vector.size() == 2);
    REQUIRE(vector[0] == 0xa9);
    REQUIRE(vector[1] == 0xc2);
}

TEST_CASE("fixed_int: 4-byte type deserialized")
{
    std::vector<std::uint8_t> data{0xa9, 0xc2, 0x97, 0x15};
    auto value = prot::fixed_int<std::uint32_t>::deserialize(tran::buffer_data_fetcher(data));
    REQUIRE(value == 0xa9c29715);
}

TEST_CASE("fixed_int: four byte serialized")
{
    std::uint32_t value =  0xa9c29715;
    std::vector<std::uint8_t> vector;
    prot::fixed_int<std::uint32_t>::serialize(value, [&](auto b) {
        vector.push_back(b);
    });
    REQUIRE(vector.size() == 4);
    REQUIRE(vector[0] == 0xa9);
    REQUIRE(vector[1] == 0xc2);
    REQUIRE(vector[2] == 0x97);
    REQUIRE(vector[3] == 0x15);
}
