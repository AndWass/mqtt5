#include <doctest/doctest.h>

#include "mqtt5/message/connack.hpp"

TEST_CASE("connack: deserialize of basic packet")
{
    std::vector<std::uint8_t> binary;
    binary.push_back(0);
    binary.push_back(0);
    binary.push_back(3);
    binary.push_back(34);
    binary.push_back(0);
    binary.push_back(10);
    mqtt5::message::connack connack;
    auto iter = mqtt5::message::deserialize_into(connack, binary.begin(), binary.end());
    REQUIRE(iter == binary.end());
    REQUIRE(connack.session_present == false);
    REQUIRE(connack.reason_code == mqtt5::message::connack::reason::success);
    REQUIRE(connack.topic_alias_maximum.value() == 10);
}
