#include <mqtt5_v2/protocol/properties.hpp>

#include <doctest/doctest.h>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <p0443_v2/sink_receiver.hpp>

TEST_CASE("property: byte deserializer")
{
    std::vector<std::uint8_t> data{0x28, 0x14};
    auto prop = mqtt5_v2::protocol::property::deserialize(mqtt5_v2::transport::buffer_data_fetcher(data));
    REQUIRE(prop.identifier == 0x28);
    REQUIRE(prop.value_.index() == 0);
    REQUIRE(prop.value_as<int>() == 0x14);
}

TEST_CASE("property: string deserializer")
{
    std::vector<std::uint8_t> data{0x03, 0x0, 0x05, 'h', 'e', 'l', 'l', 'o'};
    auto prop = mqtt5_v2::protocol::property::deserialize(mqtt5_v2::transport::buffer_data_fetcher(data));
    REQUIRE(prop.identifier == 0x3);
    REQUIRE(prop.value_.index() == 4);
    REQUIRE(prop.value_as<std::string>() == "hello");
}