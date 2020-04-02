#include <cstdint>

#include <p0443_v2/sink_receiver.hpp>

#include <doctest/doctest.h>
#include <mqtt5/protocol/fixed_int.hpp>
#include <mqtt5/protocol/inplace_deserializer.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#if 0
TEST_CASE("empty optional not deserialized")
{
    std::optional<mqtt5::protocol::fixed_int<std::uint32_t>> fixed_byte;

    boost::asio::io_context io;
    boost::beast::test::stream tx_stream(io);
    boost::beast::test::stream rx_stream(io);
    rx_stream.connect(tx_stream);
    std::uint8_t data[4]{0xa9, 0xc2, 0x97, 0x15};
    tx_stream.write_some(boost::asio::buffer(data));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;
    auto deserializer = mqtt5::protocol::inplace_deserializer(fixed_byte, mqtt5::transport::data_fetcher(rx_stream, buffer));
    auto op = p0443_v2::connect(deserializer, p0443_v2::sink_receiver{});
    p0443_v2::start(op);

    io.run();
    REQUIRE_FALSE(fixed_byte.has_value());
    REQUIRE(rx_stream.nread_bytes() == 0);
}

TEST_CASE("non-empty optional deserialized")
{
    std::optional<mqtt5::protocol::fixed_int<std::uint32_t>> fixed_byte;
    fixed_byte.emplace(0);

    boost::asio::io_context io;
    boost::beast::test::stream tx_stream(io);
    boost::beast::test::stream rx_stream(io);
    rx_stream.connect(tx_stream);
    std::uint8_t data[4]{0xa9, 0xc2, 0x97, 0x15};
    tx_stream.write_some(boost::asio::buffer(data));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;
    auto deserializer = mqtt5::protocol::inplace_deserializer(fixed_byte, mqtt5::transport::data_fetcher(rx_stream, buffer));
    auto op = p0443_v2::connect(deserializer, p0443_v2::sink_receiver{});
    p0443_v2::start(op);

    io.run();
    REQUIRE(fixed_byte.has_value());
    REQUIRE(rx_stream.nread_bytes() == 4);
    REQUIRE(fixed_byte->value == 0xa9c29715);
}
#endif