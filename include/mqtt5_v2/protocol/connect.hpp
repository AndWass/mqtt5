
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <iterator>

#include <mqtt5_v2/protocol/binary.hpp>
#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/header.hpp>
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>
#include <mqtt5_v2/protocol/properties.hpp>
#include <mqtt5_v2/protocol/string.hpp>

#include <p0443_v2/just.hpp>
#include <p0443_v2/sequence.hpp>
#include <p0443_v2/then.hpp>

namespace mqtt5_v2::protocol
{
using std::begin;
using std::end;
struct connect
{
    static constexpr std::uint8_t type_value = 1;

    static constexpr std::uint8_t clean_start_flag = 2, will_flag = 4, will_retain_flag = 0x20,
                                  password_flag = 0x40, username_flag = 0x80;

    static constexpr std::uint8_t will_qos_0 = 0, will_qos_1 = 0x08, will_qos_2 = 0x10;

    std::uint8_t version = 5;
    std::uint8_t flags = 0x02;
    std::uint16_t keep_alive = 240;
    properties connect_properties;
    std::string client_id;
    std::optional<properties> will_properties;
    std::optional<std::string> will_topic;
    std::optional<binary::type> will_payload;
    std::optional<std::string> username;
    std::optional<binary::type> password;

    connect() = default;
    template<class T>
    connect(std::in_place_t, T fetcher) {
        deserialize(fetcher);
    }

    template<class InputIt>
    void set_will_payload(InputIt begin, InputIt end)
    {
        will_payload.emplace(begin, end);
    }

    template<class T, class = decltype(begin(std::declval<const T&>()))>
    void set_will_payload(const T& val)
    {
        set_will_payload(begin(val), end(val));
    }

    template <class Stream>
    void deserialize(transport::data_fetcher<Stream> fetcher)
    {
        std::string always_mqtt = string::deserialize(fetcher);
        if(always_mqtt != "MQTT")
        {
            throw protocol_error("Unexpected MQTT string value");
        }
        using int8 = fixed_int<std::uint8_t>;
        using int16 = fixed_int<std::uint16_t>;

        version = int8::deserialize(fetcher);
        flags = int8::deserialize(fetcher);
        keep_alive = int16::deserialize(fetcher);
        connect_properties.deserialize(fetcher);
        client_id = string::deserialize(fetcher);
        if(flags & will_flag)
        {
            will_properties.emplace().deserialize(fetcher);
            will_topic = string::deserialize(fetcher);
            will_payload = binary::deserialize(fetcher);
        }
        if(flags & username_flag)
        {
            username = string::deserialize(fetcher);
        }
        if(flags & password_flag)
        {
            password = binary::deserialize(fetcher);
        }
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        std::string always_mqtt("MQTT");
        string::serialize(always_mqtt, writer);
        fixed_int<std::uint8_t>::serialize(version, writer);
        fixed_int<std::uint8_t>::serialize(flags, writer);
        fixed_int<std::uint16_t>::serialize(keep_alive, writer);
        connect_properties.serialize(writer);
        string::serialize(client_id, writer);
        if(will_properties)
        {
            will_properties->serialize(writer);
        }
        if(will_topic)
        {
            string::serialize(*will_topic, writer);
        }
        if(will_payload)
        {
            binary::serialize(*will_payload, writer);
        }
        if(username)
        {
            string::serialize(*username, writer);
        }
        if(password)
        {
            binary::serialize(*password, writer);
        }
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, 0, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};
} // namespace mqtt5_v2::protocol
