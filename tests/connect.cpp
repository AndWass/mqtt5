#include "mqtt5/message/connect.hpp"
#include "mqtt5/connection.hpp"

#include <doctest/doctest.h>
#include <iostream>
#include <string_view>
#include <set>

TEST_CASE("connect: serializing of non-normative example")
{
    mqtt5::message::connect example;
    example.flags.username = true;
    example.flags.password = true;
    example.flags.will_flag = true;
    example.flags.will_qos = mqtt5::publish::quality_of_service::qos1;
    example.flags.clean_start = true;
    example.keep_alive = std::chrono::seconds{10};
    example.session_expiry_interval = 10;
    example.client_id = "hello_world";
    std::vector<std::uint8_t> serialized_example;
    (void)mqtt5::message::serialize(example, std::back_inserter(serialized_example));
    REQUIRE(serialized_example.size() >= 16+2+example.client_id.byte_size());
    std::size_t i=0;
    REQUIRE(serialized_example[i++] == 0);
    REQUIRE(serialized_example[i++] == 4);
    REQUIRE(serialized_example[i++] == 'M');
    REQUIRE(serialized_example[i++] == 'Q');
    REQUIRE(serialized_example[i++] == 'T');
    REQUIRE(serialized_example[i++] == 'T');
    REQUIRE(serialized_example[i++] == 5);
    REQUIRE(serialized_example[i++] == 0b11001110);
    REQUIRE(serialized_example[i++] == 0);
    REQUIRE(serialized_example[i++] == 10);
    REQUIRE(serialized_example[i++] == 5);
    REQUIRE(serialized_example[i++] == 17);
    REQUIRE(serialized_example[i++] == 0);
    REQUIRE(serialized_example[i++] == 0);
    REQUIRE(serialized_example[i++] == 0);
    REQUIRE(serialized_example[i++] == 10);
    // packet payload starts here
    REQUIRE(serialized_example[i++] == 0);
    REQUIRE(serialized_example[i++] == example.client_id.byte_size());
    for(auto ch: example.client_id) {
        REQUIRE(serialized_example[i++] == ch);
    }
}

TEST_CASE("check client_id generation implementation")
{
    std::set<std::string> client_ids;
    for(int i=0; i<10'000; i++) {
        auto id = mqtt5::generate_client_id();
        REQUIRE(id.size() <= 23);
        REQUIRE(std::all_of(id.begin(), id.end(), [](auto ch) {
            using namespace std::string_view_literals;
             static auto valid_characters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"sv;
             return valid_characters.find(ch) != std::string_view::npos;
        }));
        client_ids.emplace(id);
    }
    REQUIRE(client_ids.size() == 10'000);
}