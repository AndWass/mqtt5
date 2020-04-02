
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <mqtt5/protocol/binary.hpp>

#include <p0443_v2/sink_receiver.hpp>

#include <mqtt5/protocol/string.hpp>
#include <mqtt5/protocol/inplace_deserializer.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <doctest/doctest.h>

#include "vector_serialize.hpp"

TEST_CASE("binary: inplace_deserialize")
{
    std::vector<std::uint8_t> buffer{
        0,5,1,2,3,4,5,6,7
    };
    auto bin = mqtt5::protocol::binary::deserialize(mqtt5::transport::buffer_data_fetcher(buffer));
    REQUIRE(bin.size() == 5);
    REQUIRE(bin[0] == 1);
    REQUIRE(bin[1] == 2);
    REQUIRE(bin[2] == 3);
    REQUIRE(bin[3] == 4);
    REQUIRE(bin[4] == 5);
    REQUIRE(buffer.size() == 2);
}

TEST_CASE("binary: serialize")
{
    mqtt5::protocol::binary::type bin{1,2,3,4,5};

    std::vector<std::uint8_t> vec;
    mqtt5::protocol::binary::serialize(bin, [&](auto b) {
        vec.push_back(b);
    });
    REQUIRE(vec.size() == 7);
    REQUIRE(vec[0] == 0);
    REQUIRE(vec[1] == 5);
    REQUIRE(vec[2] == 1);
    REQUIRE(vec[3] == 2);
    REQUIRE(vec[4] == 3);
    REQUIRE(vec[5] == 4);
    REQUIRE(vec[6] == 5);
}