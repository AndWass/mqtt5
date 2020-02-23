#include <cstdint>

#include <p0443_v2/sink_receiver.hpp>

#include <doctest/doctest.h>
#include <mqtt5_v2/protocol/header.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

TEST_CASE("header: inplace_deserialize")
{
    boost::asio::io_context io;
    boost::beast::test::stream tx_stream(io);
    boost::beast::test::stream rx_stream(io);
    rx_stream.connect(tx_stream);
    SUBCASE("maximum length") {
        std::uint8_t data[7]{0x59, 0xff, 0xff, 0xff, 0x7f, 0x9b, 0x8a};
        tx_stream.write_some(boost::asio::buffer(data));

        boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;

        mqtt5_v2::protocol::header value;
        auto op = p0443_v2::connect(
            value.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx_stream, buffer)),
            p0443_v2::sink_receiver{});
        p0443_v2::start(op);

        io.run();

        REQUIRE(value.remaining_length() == 268'435'455);
        REQUIRE(value.type() == 0x05);
        REQUIRE(value.flags() == 0x09);

        REQUIRE(buffer.size() == 2);
        auto *ptr = static_cast<const std::uint8_t*>(buffer.cdata().data());
        REQUIRE(ptr[0] == 0x9b);
        REQUIRE(ptr[1] == 0x8a);
    }
    SUBCASE("single byte value") {
        std::uint8_t data[4]{0x59, 0x7f, 0xa8, 0xb9};
        tx_stream.write_some(boost::asio::buffer(data));

        boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffer;

        mqtt5_v2::protocol::header value;
        auto op = p0443_v2::connect(
            value.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx_stream, buffer)),
            p0443_v2::sink_receiver{});
        p0443_v2::start(op);

        io.run();
        REQUIRE(value.type() == 0x05);
        REQUIRE(value.flags() == 0x09);
        REQUIRE(value.remaining_length() == 0x7f);
        REQUIRE(buffer.size() == 2);
        auto *ptr = static_cast<const std::uint8_t*>(buffer.cdata().data());
        REQUIRE(ptr[0] == 0xa8);
        REQUIRE(ptr[1] == 0xb9);
    }
}