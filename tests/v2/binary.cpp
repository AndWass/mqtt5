#include <mqtt5_v2/protocol/binary.hpp>

#include <p0443_v2/sink_receiver.hpp>

#include <mqtt5_v2/protocol/string.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/beast/_experimental/test/stream.hpp>

#include <doctest/doctest.h>

/*TEST_CASE("string: inplace_deserialize")
{
    boost::asio::io_context io;
    boost::beast::test::stream rx(io);
    boost::beast::test::stream tx(io);
    std::uint8_t buffer[7]{
        0,5,'h','e','l','l','o'
    };
    rx.connect(tx);

    tx.write_some(boost::asio::buffer(buffer));

    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> rx_buffer;

    mqtt5_v2::protocol::string string;
    auto op = p0443_v2::connect(string.inplace_deserializer(mqtt5_v2::transport::data_fetcher(rx, rx_buffer)), p0443_v2::sink_receiver{});
    p0443_v2::start(op);
    io.run();
    REQUIRE(string.is_valid());
    REQUIRE(string.value() == "hello");
}*/

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