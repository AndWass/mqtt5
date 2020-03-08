#include <mqtt5_v2/protocol/connect.hpp>

#include <doctest/doctest.h>

#include "vector_serialize.hpp"

TEST_CASE("connect")
{
    mqtt5_v2::protocol::connect connect;
    connect.version.value = 5;
    connect.flags.value = 0x02;
    connect.keep_alive.value = 10;

    connect.connect_properties.add_property(17, 10);

    auto bytes = vector_serialize(connect);
    auto byte_len = bytes.size();
    REQUIRE_FALSE(bytes.empty());
}