
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <doctest/doctest.h>

#include <p0443_v2/sink_receiver.hpp>

#include <mqtt5/protocol/string.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include "vector_serialize.hpp"

TEST_CASE("string: deserialize")
{
    std::vector<std::uint8_t> buffer{
        0,5,'h','e','l','l','o'
    };
    auto string = mqtt5::protocol::string::deserialize(mqtt5::transport::buffer_data_fetcher(buffer));
    REQUIRE(string.size() == 5);
    REQUIRE(string == "hello");
}

TEST_CASE("string: serialize")
{
    std::string string = "hello";
    std::vector<std::uint8_t> vec;
    mqtt5::protocol::string::serialize(string, [&](auto b) {
        vec.push_back(b);
    });
    REQUIRE(vec.size() == 7);
    REQUIRE(vec[0] == 0);
    REQUIRE(vec[1] == 5);
    REQUIRE(vec[2] == 'h');
    REQUIRE(vec[3] == 'e');
    REQUIRE(vec[4] == 'l');
    REQUIRE(vec[5] == 'l');
    REQUIRE(vec[6] == 'o');
}