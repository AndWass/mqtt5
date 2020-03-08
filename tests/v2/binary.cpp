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
    boost::asio::io_context io;
    boost::beast::test::stream rx(io);
    boost::beast::test::stream tx(io);
    std::uint8_t buffer[9]{
        0,5,1,2,3,4,5,6,7
    };
    rx.connect(tx);

    tx.write_some(boost::asio::buffer(buffer));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> rx_buffer;

    mqtt5_v2::protocol::binary bin;
    auto op = p0443_v2::connect(bin.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx, rx_buffer)), p0443_v2::sink_receiver{});
    p0443_v2::start(op);
    io.run();
    auto val = bin.value();
    REQUIRE(val.size() == 5);
    REQUIRE(val[0] == 1);
    REQUIRE(val[1] == 2);
    REQUIRE(val[2] == 3);
    REQUIRE(val[3] == 4);
    REQUIRE(val[4] == 5);
    REQUIRE(rx_buffer.size() == 2);
}

TEST_CASE("binary: inplace_deserialize")
{
    boost::asio::io_context io;
    boost::beast::test::stream rx(io);
    boost::beast::test::stream tx(io);
    std::uint8_t buffer[9]{
        0,5,1,2,3,4,5,6,7
    };
    rx.connect(tx);

    tx.write_some(boost::asio::buffer(buffer));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> rx_buffer;

    mqtt5_v2::protocol::binary bin;
    auto op = p0443_v2::connect(inplace_deserializer(bin, mqtt5_v2::transport::data_fetcher(rx, rx_buffer)), p0443_v2::sink_receiver{});
    p0443_v2::start(op);
    io.run();
    auto val = bin.value();
    REQUIRE(val.size() == 5);
    REQUIRE(val[0] == 1);
    REQUIRE(val[1] == 2);
    REQUIRE(val[2] == 3);
    REQUIRE(val[3] == 4);
    REQUIRE(val[4] == 5);
    REQUIRE(rx_buffer.size() == 2);
}

TEST_CASE("binary: serialize")
{
    mqtt5_v2::protocol::binary bin;
    std::uint8_t data[5] = {1,2,3,4,5};
    bin = data;

    auto vec = vector_serialize(bin);
    REQUIRE(vec.size() == 7);
    REQUIRE(vec[0] == 0);
    REQUIRE(vec[1] == 5);
    REQUIRE(vec[2] == 1);
    REQUIRE(vec[3] == 2);
    REQUIRE(vec[4] == 3);
    REQUIRE(vec[5] == 4);
    REQUIRE(vec[6] == 5);
}