#include <doctest/doctest.h>

#include <boost/beast/_experimental/test/stream.hpp>

#include <mqtt5/type/string.hpp>

#include "stream.hpp"

TEST_CASE("string: rejects u+0000")
{
    char ch = '\0';
    REQUIRE_THROWS_AS(mqtt5::type::string(&ch, &ch+1), boost::system::system_error);
}

TEST_CASE("string: rejects u+d800 up to u+dfff")
{
    for(int i=0xA0; i<= 0xBF; i++)
    {
        for(int j=0x80; j<=0xBF; j++)
        {
            REQUIRE_THROWS_AS(mqtt5::type::string({0xED, i, j}), boost::system::system_error);
        }
    }
}

TEST_CASE("string: serialize UTF string")
{
    using namespace mqtt5;
    using namespace mqtt5::type;
    mqtt5::type::string str("A\xF0\xAA\x9B\x94");
    std::array<std::uint8_t, 16> serialized;
    auto iter = serialize(str, serialized.begin());
    REQUIRE(serialized[0] == 0);
    REQUIRE(serialized[1] == 5);
    REQUIRE(serialized[2] == 'A');
    REQUIRE(serialized[3] == 0xF0);
    REQUIRE(serialized[4] == 0xAA);
    REQUIRE(serialized[5] == 0x9B);
    REQUIRE(serialized[6] == 0x94);
    REQUIRE(iter == serialized.begin() + 7);
}

TEST_CASE("string: serialize ascii string")
{
    using namespace mqtt5;
    using namespace mqtt5::type;
    mqtt5::type::string str("hello");
    std::array<std::uint8_t, 16> serialized;
    auto iter = serialize(str, serialized.begin());
    REQUIRE(serialized[0] == 0);
    REQUIRE(serialized[1] == 5);
    REQUIRE(serialized[2] == 'h');
    REQUIRE(serialized[3] == 'e');
    REQUIRE(serialized[4] == 'l');
    REQUIRE(serialized[5] == 'l');
    REQUIRE(serialized[6] == 'o');
    REQUIRE(iter == serialized.begin() + 7);
}

TEST_CASE("string: assignment operator")
{
    mqtt5::type::string a("hello");
    mqtt5::type::string b("world");
    a = b;
    REQUIRE(a == "world");
    a = "hello";
    REQUIRE(a == "hello");
    std::string world = "world";
    a = world;
    REQUIRE(a == "world");
}

TEST_CASE("string: string_from_stream")
{
    boost::asio::io_context io;
    boost::beast::test::stream rx(io);
    boost::beast::test::stream tx(io);
    std::uint8_t buffer[7]{
        0,5,'h','e','l','l','o'
    };
    rx.connect(tx);

    tx.write_some(boost::asio::buffer(buffer));

    mqtt5::type::string str;
    boost::system::error_code error;
    mqtt5::type::string_from_stream(rx, value_receiver{str, error});
    io.run();
    REQUIRE_FALSE(error);
    REQUIRE(str.to_string() == "hello");
}