
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <optional>
#include <iterator>

#include "protocol/string.hpp"

namespace mqtt5
{
struct last_will_t
{
    std::string topic;
    std::vector<std::uint8_t> payload;

    template<class Container>
    void set_payload(const Container& data)
    {
        using std::begin;
        using std::end;
        payload.clear();
        payload.insert(payload.begin(), begin(data), end(data));
    }

    std::string content_type;
    std::string response_topic;
    std::vector<std::uint8_t> correlation_data;
    std::vector<protocol::key_value_pair> user_property;

    std::chrono::duration<std::uint32_t> delay_interval{0};
    std::chrono::duration<std::uint32_t> message_expiry_interval{0};

    std::uint8_t retain:1;
    std::uint8_t quality_of_service: 2;
    std::uint8_t payload_format_indicator:1;


    last_will_t(): retain(0), quality_of_service(0), payload_format_indicator(0) {}
};
struct connect_options
{
    std::optional<last_will_t> last_will;
    std::string client_id;

    std::chrono::duration<std::uint16_t> keep_alive{0};
    std::string username;
    std::vector<std::uint8_t> password;

    bool clean_start = true;
};
}