#include <mqtt5_v2/protocol/connect.hpp>

#include <p0443_v2/sink_receiver.hpp>

#include <doctest/doctest.h>

#include "vector_serialize.hpp"

TEST_CASE("connect: serialize")
{
    mqtt5_v2::protocol::connect connect;
    connect.keep_alive = std::chrono::seconds(10);

    connect.connect_properties.session_expiry_interval = std::chrono::seconds(10);

    auto bytes = vector_serialize(connect);
    auto byte_len = bytes.size();
    REQUIRE(byte_len == 20);
    REQUIRE(bytes[0] == 0x10);
    REQUIRE(bytes[1] == 18);
    REQUIRE(bytes[2] == 0);
    REQUIRE(bytes[3] == 4);
    REQUIRE(bytes[4] == 'M');
    REQUIRE(bytes[5] == 'Q');
    REQUIRE(bytes[6] == 'T');
    REQUIRE(bytes[7] == 'T');
    REQUIRE(bytes[8] == 5);
    REQUIRE(bytes[9] == 2);
    REQUIRE(bytes[10] == 0);
    REQUIRE(bytes[11] == 10);
    REQUIRE(bytes[12] == 5);
    REQUIRE(bytes[13] == 17);
    REQUIRE(bytes[14] == 0);
    REQUIRE(bytes[15] == 0);
    REQUIRE(bytes[16] == 0);
    REQUIRE(bytes[17] == 10);
    REQUIRE(bytes[18] == 0);
    REQUIRE(bytes[19] == 0);
}

TEST_CASE("connect: deserialize")
{
    std::vector<std::uint8_t> data = {
        0, 4, 'M', 'Q', 'T', 'T', 5, 2,
        0, 10, 5, 17, 0, 0, 0, 10, 0, 0};

    mqtt5_v2::protocol::connect packet;
    packet.deserialize(mqtt5_v2::transport::buffer_data_fetcher(data));

    REQUIRE(data.empty());
    REQUIRE(packet.keep_alive.count() == 10);
    REQUIRE(packet.connect_properties.session_expiry_interval.count() == 10);
}
