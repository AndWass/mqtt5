
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>

#include <cstdint>
#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/header.hpp>
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>
#include <mqtt5_v2/protocol/properties.hpp>

#include <p0443_v2/sequence.hpp>

namespace mqtt5_v2::protocol
{
struct connack
{
    static constexpr std::uint8_t type_value = 2;

    std::uint8_t flags;
    std::uint8_t reason_code;
    protocol::properties properties;

    connack() = default;
    template <class T>
    connack(std::in_place_t, T fetcher) {
        deserialize(fetcher);
    }

    template <class Stream>
    void deserialize(transport::data_fetcher<Stream> fetcher) {
        flags = fixed_int<std::uint8_t>::deserialize(fetcher);
        reason_code = fixed_int<std::uint8_t>::deserialize(fetcher);
        properties.deserialize(fetcher);
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        fixed_int<std::uint8_t>::serialize(flags, writer);
        fixed_int<std::uint8_t>::serialize(reason_code, writer);
        properties.serialize(writer);
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, 0, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }

    struct properties_t
    {
        std::vector<key_value_pair> user_properties;
        std::string authentication_method;
        std::vector<std::uint8_t> authentication_data;
        std::string assigned_client_id;
        std::string reason_string;
        std::string response_information;
        std::string server_reference;

        std::chrono::duration<std::uint32_t> session_expiry_interval{0};
        std::uint32_t maximum_packet_size = 0xFFFFFFFF;
        std::chrono::duration<std::uint16_t> server_keep_alive{0};

        std::uint16_t topic_alias_maximum = 0;
        std::uint16_t receive_maximum = 0xFFFF;
        std::uint8_t maximum_qos = 2;
        bool retain_available = true;
        bool wildcard_subscriptions_available = true;
        bool subscription_identifiers_available = true;
        bool shared_subscription_available = true;

        static properties_t from_properties(const protocol::properties &props) {
            properties_t retval;
            using ids = property_ids;

            for (const auto &prop : props) {
                if (prop.identifier == ids::session_expiry_interval) {
                    retval.session_expiry_interval =
                        std::chrono::seconds{std::get<std::uint32_t>(prop.value)};
                }
                else if (prop.identifier == ids::receive_maximum) {
                    retval.receive_maximum = std::get<std::uint16_t>(prop.value);
                }
                else if (prop.identifier == ids::maximum_qos) {
                    retval.maximum_qos = std::get<std::uint8_t>(prop.value);
                }
                else if (prop.identifier == ids::maximum_packet_size) {
                    retval.maximum_packet_size = std::get<std::uint32_t>(prop.value);
                }
                else if (prop.identifier == ids::topic_alias_maximum) {
                    retval.topic_alias_maximum = std::get<std::uint16_t>(prop.value);
                }
                else if (prop.identifier == ids::retain_available) {
                    retval.retain_available = std::get<std::uint8_t>(prop.value);
                }
                else if (prop.identifier == ids::user_property) {
                    retval.user_properties.emplace_back(std::get<key_value_pair>(prop.value));
                }
                else if (prop.identifier == ids::authentication_method) {
                    retval.authentication_method = std::get<std::string>(prop.value);
                }
                else if (prop.identifier == ids::authentication_data) {
                    retval.authentication_data = std::get<std::vector<std::uint8_t>>(prop.value);
                }
                else if (prop.identifier == ids::assigned_client_id) {
                    retval.assigned_client_id = std::get<std::string>(prop.value);
                }
                else if (prop.identifier == ids::reason_string) {
                    retval.reason_string = std::get<std::string>(prop.value);
                }
                else if (prop.identifier == ids::wildcard_subscriptions_available) {
                    retval.wildcard_subscriptions_available = std::get<std::uint8_t>(prop.value);
                }
                else if (prop.identifier == ids::wildcard_subscriptions_available) {
                    retval.subscription_identifiers_available = std::get<std::uint8_t>(prop.value);
                }
                else if (prop.identifier == ids::shared_subscription_available) {
                    retval.shared_subscription_available = std::get<std::uint8_t>(prop.value);
                }
                else if (prop.identifier == ids::server_keep_alive) {
                    retval.server_keep_alive =
                        std::chrono::seconds{std::get<std::uint16_t>(prop.value)};
                }
                else if (prop.identifier == ids::response_information) {
                    retval.response_information = std::get<std::string>(prop.value);
                }
                else if (prop.identifier == ids::server_reference) {
                    retval.server_reference = std::get<std::string>(prop.value);
                }
            }
            return retval;
        }

        protocol::properties to_properties() const {
            protocol::properties retval;
            properties_t dflt;
            using ids = property_ids;

            for (auto &kv : user_properties) {
                retval.add_property(ids::user_property, kv);
            }
            if (!authentication_method.empty()) {
                retval.add_property(ids::authentication_method, authentication_method);
                retval.add_property(ids::authentication_data, authentication_data);
            }
            auto maybe_add = [&](const auto &val, const auto &dflt, std::uint8_t prop) {
                if (val != dflt) {
                    retval.add_property(prop, val);
                }
            };

            maybe_add(session_expiry_interval.count(), dflt.session_expiry_interval.count(),
                      ids::session_expiry_interval);
            maybe_add(receive_maximum, dflt.receive_maximum, ids::receive_maximum);
            maybe_add(maximum_packet_size, dflt.maximum_packet_size, ids::maximum_packet_size);
            maybe_add(topic_alias_maximum, dflt.topic_alias_maximum, ids::topic_alias_maximum);
            maybe_add(retain_available, dflt.retain_available, ids::retain_available);
            maybe_add(maximum_qos, dflt.maximum_qos, ids::maximum_qos);
            maybe_add(assigned_client_id, dflt.assigned_client_id, ids::assigned_client_id);
            maybe_add(reason_string, dflt.reason_string, ids::reason_string);
            maybe_add(wildcard_subscriptions_available, dflt.wildcard_subscriptions_available,
                      ids::wildcard_subscriptions_available);
            maybe_add(subscription_identifiers_available, dflt.subscription_identifiers_available,
                      ids::subscription_identifiers_available);
            maybe_add(shared_subscription_available, dflt.shared_subscription_available,
                      ids::shared_subscription_available);

            maybe_add(server_keep_alive.count(), dflt.server_keep_alive.count(),
                      ids::server_keep_alive);
            maybe_add(response_information, dflt.response_information, ids::response_information);
            maybe_add(server_reference, dflt.server_reference, ids::server_reference);

            return retval;
        }
    };
};
} // namespace mqtt5_v2::protocol