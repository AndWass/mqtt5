#include <mqtt5_v2/protocol/properties.hpp>

#include <doctest/doctest.h>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <p0443_v2/sink_receiver.hpp>

TEST_CASE("property: byte inplace_deserializer")
{
    mqtt5_v2::protocol::property prop;

    boost::asio::io_context io;
    boost::beast::test::stream tx_stream(io);
    boost::beast::test::stream rx_stream(io);
    rx_stream.connect(tx_stream);
    std::uint8_t data[2]{0x28, 0x14};
    tx_stream.write_some(boost::asio::buffer(data));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;
    auto deserializer = prop.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx_stream, buffer));
    auto op = p0443_v2::connect(deserializer, p0443_v2::sink_receiver{});
    p0443_v2::start(op);
    io.run();
    REQUIRE(prop.identifier.value == 0x28);
    REQUIRE(prop.value.index() == 0);
    REQUIRE(std::get<0>(prop.value).value == 0x14);
}

TEST_CASE("property: string inplace_deserializer")
{
    mqtt5_v2::protocol::property prop;

    boost::asio::io_context io;
    boost::beast::test::stream tx_stream(io);
    boost::beast::test::stream rx_stream(io);
    rx_stream.connect(tx_stream);
    std::uint8_t data[8]{0x03, 0x0, 0x05, 'h', 'e', 'l', 'l', 'o'};
    tx_stream.write_some(boost::asio::buffer(data));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;
    auto deserializer = prop.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx_stream, buffer));
    auto op = p0443_v2::connect(deserializer, p0443_v2::sink_receiver{});
    p0443_v2::start(op);
    io.run();
    REQUIRE(prop.identifier.value == 0x3);
    REQUIRE(prop.value.index() == 4);
    REQUIRE(std::get<4>(prop.value).value() == "hello");
}

TEST_CASE("properties: inplace_deserializer")
{
    mqtt5_v2::protocol::properties props;
}