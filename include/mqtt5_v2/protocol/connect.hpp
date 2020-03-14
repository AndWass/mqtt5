
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
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>
#include <mqtt5_v2/protocol/properties.hpp>
#include <mqtt5_v2/protocol/string.hpp>

#include <p0443_v2/sequence.hpp>

namespace mqtt5_v2::protocol
{
struct connect
{
    string always_mqtt;
    fixed_int<std::uint8_t> version =  5;
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
        return p0443_v2::sequence(protocol::inplace_deserializer(always_mqtt, data_fetcher),
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
    void serialize(Writer &&writer) const {
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
};
} // namespace mqtt5_v2::protocol
