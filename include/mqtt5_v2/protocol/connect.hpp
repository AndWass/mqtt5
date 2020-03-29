
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iterator>
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
using std::begin;
using std::end;
struct connect
{
    static constexpr std::uint8_t type_value = 1;

    static constexpr std::uint8_t clean_start_flag = 2, will_flag = 4, will_retain_flag = 0x20,
                                  password_flag = 0x40, username_flag = 0x80;

    static constexpr std::uint8_t will_qos_0 = 0, will_qos_1 = 0x08, will_qos_2 = 0x10;

    struct properties_t
    {
        std::vector<key_value_pair> user_properties;
        std::string authentication_method;
        std::vector<std::uint8_t> authentication_data;

        std::chrono::duration<std::uint32_t> session_expiry_interval{0};
        std::uint16_t receive_maximum = 0xFFFF;
        std::uint32_t maximum_packet_size = 0xFFFFFFFF;
        std::uint16_t topic_alias_maximum = 0;
        bool request_response_information = false;
        bool request_problem_information = false;

        static properties_t from_properties(const protocol::properties &props) {
            properties_t retval;
            for (const auto &prop : props) {
                if (prop.identifier == property_ids::session_expiry_interval) {
                    retval.session_expiry_interval =
                        std::chrono::seconds{std::get<std::uint32_t>(prop.value)};
                }
                else if (prop.identifier == property_ids::receive_maximum) {
                    retval.receive_maximum = std::get<std::uint16_t>(prop.value);
                }
                else if (prop.identifier == property_ids::maximum_packet_size) {
                    retval.maximum_packet_size = std::get<std::uint32_t>(prop.value);
                }
                else if (prop.identifier == property_ids::topic_alias_maximum) {
                    retval.topic_alias_maximum = std::get<std::uint16_t>(prop.value);
                }
                else if (prop.identifier == property_ids::request_response_information) {
                    retval.request_response_information = std::get<std::uint8_t>(prop.value);
                }
                else if (prop.identifier == property_ids::request_problem_information) {
                    retval.request_problem_information = std::get<std::uint8_t>(prop.value);
                }
                else if (prop.identifier == property_ids::user_property) {
                    retval.user_properties.emplace_back(std::get<key_value_pair>(prop.value));
                }
                else if (prop.identifier == property_ids::authentication_method) {
                    retval.authentication_method = std::get<std::string>(prop.value);
                }
                else if (prop.identifier == property_ids::authentication_data) {
                    retval.authentication_data = std::get<std::vector<std::uint8_t>>(prop.value);
                }
            }
            return retval;
        }

        protocol::properties to_properties() const {
            protocol::properties retval;
            properties_t dflt;
            for (auto &kv : user_properties) {
                retval.add_property(property_ids::user_property, kv);
            }
            if (!authentication_method.empty()) {
                retval.add_property(property_ids::authentication_method, authentication_method);
                retval.add_property(property_ids::authentication_data, authentication_data);
            }
            auto maybe_add = [&](auto val, auto dflt, std::uint8_t prop) {
                if (val != dflt) {
                    retval.add_property(prop, val);
                }
            };
            using ids = property_ids;
            maybe_add(session_expiry_interval.count(), dflt.session_expiry_interval.count(),
                      ids::session_expiry_interval);
            maybe_add(receive_maximum, dflt.receive_maximum, ids::receive_maximum);
            maybe_add(maximum_packet_size, dflt.maximum_packet_size, ids::maximum_packet_size);
            maybe_add(topic_alias_maximum, dflt.topic_alias_maximum, ids::topic_alias_maximum);
            maybe_add(request_response_information, dflt.request_response_information,
                      ids::request_response_information);
            maybe_add(request_problem_information, dflt.request_problem_information,
                      ids::request_problem_information);

            return retval;
        }
    };

    std::uint8_t version = 5;
    std::uint8_t flags = 0x02;
    std::chrono::duration<std::uint16_t> keep_alive{240};
    properties connect_properties;
    std::string client_id;
    std::optional<properties> will_properties;
    std::optional<std::string> will_topic;
    std::optional<binary::type> will_payload;
    std::optional<std::string> username;
    std::optional<binary::type> password;

    connect() = default;
    template <class T>
    connect(std::in_place_t, T fetcher) {
        deserialize(fetcher);
    }

    template <class InputIt>
    void set_will_payload(InputIt begin, InputIt end) {
        will_payload.emplace(begin, end);
    }

    template <class T, class = decltype(begin(std::declval<const T &>()))>
    void set_will_payload(const T &val) {
        set_will_payload(begin(val), end(val));
    }

    template <class Stream>
    void deserialize(transport::data_fetcher<Stream> fetcher) {
        std::string always_mqtt = string::deserialize(fetcher);
        if (always_mqtt != "MQTT") {
            throw protocol_error("Unexpected MQTT string value");
        }
        using int8 = fixed_int<std::uint8_t>;
        using int16 = fixed_int<std::uint16_t>;

        version = int8::deserialize(fetcher);
        flags = int8::deserialize(fetcher);
        keep_alive = decltype(keep_alive){int16::deserialize(fetcher)};
        connect_properties.deserialize(fetcher);
        client_id = string::deserialize(fetcher);
        if (flags & will_flag) {
            will_properties.emplace().deserialize(fetcher);
            will_topic = string::deserialize(fetcher);
            will_payload = binary::deserialize(fetcher);
        }
        if (flags & username_flag) {
            username = string::deserialize(fetcher);
        }
        if (flags & password_flag) {
            password = binary::deserialize(fetcher);
        }
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        std::string always_mqtt("MQTT");
        string::serialize(always_mqtt, writer);
        fixed_int<std::uint8_t>::serialize(version, writer);
        fixed_int<std::uint8_t>::serialize(flags, writer);
        fixed_int<std::uint16_t>::serialize(keep_alive.count(), writer);
        connect_properties.serialize(writer);
        string::serialize(client_id, writer);
        if (will_properties) {
            will_properties->serialize(writer);
        }
        if (will_topic) {
            string::serialize(*will_topic, writer);
        }
        if (will_payload) {
            binary::serialize(*will_payload, writer);
        }
        if (username) {
            string::serialize(*username, writer);
        }
        if (password) {
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
