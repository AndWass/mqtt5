#include <doctest/doctest.h>

#include <p0443_v2/sink_receiver.hpp>

#include <mqtt5_v2/protocol/string.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include "vector_serialize.hpp"

TEST_CASE("string: deserialize")
{
    std::vector<std::uint8_t> buffer{
        0,5,'h','e','l','l','o'
    };
    auto string = mqtt5_v2::protocol::string::deserialize(mqtt5_v2::transport::buffer_data_fetcher(buffer));
    REQUIRE(string.size() == 5);
    REQUIRE(string == "hello");
}

TEST_CASE("string: serialize")
{
    std::string string = "hello";
    std::vector<std::uint8_t> vec;
    mqtt5_v2::protocol::string::serialize(string, [&](auto b) {
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