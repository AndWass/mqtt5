
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <mqtt5/protocol/varlen_int.hpp>

#include <p0443_v2/sink_receiver.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <doctest/doctest.h>

#include "vector_serialize.hpp"

TEST_CASE("varlen_integer stream read") {
    SUBCASE("maximum value") {
        std::vector<std::uint8_t> data{0xff, 0xff, 0xff, 0x7f, 0x9b, 0x8a};
        auto buffer = mqtt5::transport::buffer_data_fetcher(data);
        std::uint32_t value = mqtt5::protocol::varlen_int::deserialize(buffer);

        REQUIRE(value == 268'435'455);
        REQUIRE(data.size() == 2);
        auto *ptr = static_cast<const std::uint8_t *>(buffer.cdata());
        REQUIRE(ptr[0] == 0x9b);
        REQUIRE(ptr[1] == 0x8a);
    }
    SUBCASE("single byte value") {
        std::vector<std::uint8_t> data{0x7f, 0xa8, 0xb9};
        auto buffer = mqtt5::transport::buffer_data_fetcher(data);
        std::uint32_t value = mqtt5::protocol::varlen_int::deserialize(buffer);
        REQUIRE(value == 0x7f);
        REQUIRE(buffer.size() == 2);
        auto *ptr = static_cast<const std::uint8_t *>(buffer.cdata());
        REQUIRE(ptr[0] == 0xa8);
        REQUIRE(ptr[1] == 0xb9);
    }
}

TEST_CASE("varlen_int: serialize") {
    std::uint32_t value;
    SUBCASE("maximum value") {
        value = 268'435'455;
        std::vector<std::uint8_t> vector;
        mqtt5::protocol::varlen_int::serialize(value, [&](auto b) {
            vector.push_back(b);
        });
        REQUIRE(vector.size() == 4);
        REQUIRE(vector[0] == 0xff);
        REQUIRE(vector[1] == 0xff);
        REQUIRE(vector[2] == 0xff);
        REQUIRE(vector[3] == 0x7f);
    }

    SUBCASE("single value") {
        value = 125;
        std::vector<std::uint8_t> vector;
        mqtt5::protocol::varlen_int::serialize(value, [&](auto b) {
            vector.push_back(b);
        });
        REQUIRE(vector.size() == 1);
        REQUIRE(vector[0] == 125);
    }

    SUBCASE("error on too big value") {
        value = 268'435'455+1;
        std::vector<std::uint8_t> vector;
        REQUIRE_THROWS_AS(
        mqtt5::protocol::varlen_int::serialize(value, [&](auto b) {
        }), mqtt5::protocol::protocol_error);
    }
}