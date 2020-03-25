
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

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
struct connect
{
    static constexpr std::uint8_t type_value = 1;

    static constexpr std::uint8_t clean_start_flag = 2, will_flag = 4, will_retain_flag = 0x20,
                                  password_flag = 0x40, username_flag = 0x80;

    static constexpr std::uint8_t will_qos_0 = 0, will_qos_1 = 0x08, will_qos_2 = 0x10;

    fixed_int<std::uint8_t> version = 5;
    fixed_int<std::uint8_t> flags = 0x02;
    fixed_int<std::uint16_t> keep_alive = 240;
    properties connect_properties;
    string client_id;
    std::optional<properties> will_properties;
    std::optional<string> will_topic;
    std::optional<binary> will_payload;
    std::optional<string> username;
    std::optional<binary> password;

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        auto always_mqtt_deserializer =
            p0443_v2::transform(client_id.inplace_deserializer(data_fetcher), [this](auto fetcher) {
                if (client_id.value() != "MQTT") {
                    throw std::runtime_error("Unexpected string value");
                }
                client_id = "";
                return fetcher;
            });

        return p0443_v2::sequence(std::move(always_mqtt_deserializer),
                                  protocol::inplace_deserializer(version, data_fetcher),
                                  protocol::inplace_deserializer(flags, data_fetcher),
                                  protocol::inplace_deserializer(keep_alive, data_fetcher),
                                  protocol::inplace_deserializer(connect_properties, data_fetcher),
                                  protocol::inplace_deserializer(client_id, data_fetcher),
                                  protocol::inplace_deserializer(will_properties, data_fetcher),
                                  protocol::inplace_deserializer(will_topic, data_fetcher),
                                  protocol::inplace_deserializer(will_payload, data_fetcher),
                                  protocol::inplace_deserializer(username, data_fetcher),
                                  protocol::inplace_deserializer(password, data_fetcher));
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        string always_mqtt("MQTT");
        always_mqtt.serialize(writer);
        version.serialize(writer);
        flags.serialize(writer);
        keep_alive.serialize(writer);
        connect_properties.serialize(writer);
        client_id.serialize(writer);

        auto optional_writer = [&](const auto &member) {
            if (member) {
                member->serialize(writer);
            }
        };
        optional_writer(will_properties);
        optional_writer(will_topic);
        optional_writer(will_payload);
        optional_writer(username);
        optional_writer(password);
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, 0, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};
} // namespace mqtt5_v2::protocol
