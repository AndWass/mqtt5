#include <cstdint>

#include <p0443_v2/sink_receiver.hpp>

#include <doctest/doctest.h>
#include <mqtt5_v2/protocol/fixed_int.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include "vector_serialize.hpp"

TEST_CASE("fixed_int: single byte deserialized")
{
    mqtt5_v2::protocol::fixed_int<std::uint8_t> fixed_byte;
    REQUIRE(fixed_byte.value == 0);

    boost::asio::io_context io;
    boost::beast::test::stream tx_stream(io);
    boost::beast::test::stream rx_stream(io);
    rx_stream.connect(tx_stream);
    std::uint8_t data[1]{0xa9};
    tx_stream.write_some(boost::asio::buffer(data));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;
    auto deserializer = fixed_byte.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx_stream, buffer));
    auto op = p0443_v2::connect(deserializer, p0443_v2::sink_receiver{});
    p0443_v2::start(op);
    io.run();
    REQUIRE(fixed_byte.value == 0xa9);
}

TEST_CASE("fixed_int: single byte serialized")
{
    mqtt5_v2::protocol::fixed_int<std::uint8_t> fixed_byte;
    REQUIRE(fixed_byte.value == 0);
    fixed_byte.value = 129;
    auto vector = vector_serialize(fixed_byte);
    REQUIRE(vector.size() == 1);
    REQUIRE(vector[0] == 129);
}

TEST_CASE("fixed_int: 2-byte type deserialized")
{
    mqtt5_v2::protocol::fixed_int<std::uint16_t> fixed_byte;
    REQUIRE(fixed_byte.value == 0);

    boost::asio::io_context io;
    boost::beast::test::stream tx_stream(io);
    boost::beast::test::stream rx_stream(io);
    rx_stream.connect(tx_stream);
    std::uint8_t data[2]{0xa9, 0xc2};
    tx_stream.write_some(boost::asio::buffer(data));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;
    auto deserializer = fixed_byte.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx_stream, buffer));
    auto op = p0443_v2::connect(deserializer, p0443_v2::sink_receiver{});
    p0443_v2::start(op);

    io.run();
    REQUIRE(fixed_byte.value == 0xa9c2);
}

TEST_CASE("fixed_int: two byte serialized")
{
    mqtt5_v2::protocol::fixed_int<std::uint16_t> fixed_byte;
    fixed_byte.value =  0xa9c2;
    auto vector = vector_serialize(fixed_byte);
    REQUIRE(vector.size() == 2);
    REQUIRE(vector[0] == 0xa9);
    REQUIRE(vector[1] == 0xc2);
}

TEST_CASE("fixed_int: 4-byte type deserialized")
{
    mqtt5_v2::protocol::fixed_int<std::uint32_t> fixed_byte;
    REQUIRE(fixed_byte.value == 0);

    boost::asio::io_context io;
    boost::beast::test::stream tx_stream(io);
    boost::beast::test::stream rx_stream(io);
    rx_stream.connect(tx_stream);
    std::uint8_t data[4]{0xa9, 0xc2, 0x97, 0x15};
    tx_stream.write_some(boost::asio::buffer(data));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;
    auto deserializer = fixed_byte.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx_stream, buffer));
    auto op = p0443_v2::connect(deserializer, p0443_v2::sink_receiver{});
    p0443_v2::start(op);

    io.run();
    REQUIRE(fixed_byte.value == 0xa9c29715);
}

TEST_CASE("fixed_int: four byte serialized")
{
    mqtt5_v2::protocol::fixed_int<std::uint32_t> fixed_byte;
    fixed_byte.value =  0xa9c29715;
    auto vector = vector_serialize(fixed_byte);
    REQUIRE(vector.size() == 4);
    REQUIRE(vector[0] == 0xa9);
    REQUIRE(vector[1] == 0xc2);
    REQUIRE(vector[2] == 0x97);
    REQUIRE(vector[3] == 0x15);
}