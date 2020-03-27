#include <mqtt5_v2/protocol/binary.hpp>

#include <p0443_v2/sink_receiver.hpp>

#include <mqtt5_v2/protocol/string.hpp>
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <doctest/doctest.h>

#include "vector_serialize.hpp"

TEST_CASE("binary: inplace_deserialize")
{
    std::vector<std::uint8_t> buffer{
        0,5,1,2,3,4,5,6,7
    };
    auto bin = mqtt5_v2::protocol::binary::deserialize(mqtt5_v2::transport::buffer_data_fetcher(buffer));
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
    mqtt5_v2::protocol::binary::type bin{1,2,3,4,5};

    std::vector<std::uint8_t> vec;
    mqtt5_v2::protocol::binary::serialize(bin, [&](auto b) {
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