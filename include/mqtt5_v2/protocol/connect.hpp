
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
namespace detail
{
struct connect_connack_properties_t : properties_t_base
{
    std::string authentication_method;
    std::vector<std::uint8_t> authentication_data;
    std::chrono::duration<std::uint32_t> session_expiry_interval{0};
    std::uint32_t maximum_packet_size = 0xFFFFFFFF;
    std::uint16_t topic_alias_maximum = 0;
    std::uint16_t receive_maximum = 0xFFFF;

protected:
    void handle_property(const property &prop) {
        if (prop.identifier == property_ids::authentication_method) {
            authentication_method = prop.value_as<std::string>();
        }
        else if (prop.identifier == property_ids::authentication_data) {
            authentication_data = prop.value_as<std::vector<std::uint8_t>>();
        }
        else if (prop.identifier == property_ids::session_expiry_interval) {
            session_expiry_interval = std::chrono::seconds{prop.value_as<std::uint32_t>()};
        }
        else if (prop.identifier == property_ids::maximum_packet_size) {
            maximum_packet_size = prop.value_as<std::uint32_t>();
        }
        else if (prop.identifier == property_ids::topic_alias_maximum) {
            topic_alias_maximum = prop.value_as<std::uint16_t>();
        }
        else if (prop.identifier == property_ids::receive_maximum) {
            receive_maximum = prop.value_as<std::uint16_t>();
        }
        else {
            properties_t_base::handle_property(prop);
        }
    }

    void add_base_properties(properties &props) const {
        connect_connack_properties_t dflt;
        using c_ = connect_connack_properties_t;
        using ids = property_ids;
        if (!authentication_method.empty()) {
            props.add_property(property_ids::authentication_method, authentication_method);
            props.add_property(property_ids::authentication_data, authentication_data);
        }
        if (session_expiry_interval.count() != 0) {
            props.add_property(ids::session_expiry_interval, session_expiry_interval.count());
        }

        auto maybe_add = [&, this](auto Ptr, std::uint8_t prop) {
            if ((*this).*Ptr != dflt.*Ptr) {
                props.add_property(prop, (*this).*Ptr);
            }
        };

        maybe_add(&c_::maximum_packet_size, ids::maximum_packet_size);
        maybe_add(&c_::topic_alias_maximum, ids::topic_alias_maximum);
        maybe_add(&c_::receive_maximum, ids::receive_maximum);

        properties_t_base::add_base_properties(props);
    }
};
} // namespace detail
struct connect
{
    static constexpr std::uint8_t type_value = 1;

    static constexpr std::uint8_t clean_start_flag = 2, will_flag = 4, will_retain_flag = 0x20,
                                  password_flag = 0x40, username_flag = 0x80;

    static constexpr std::uint8_t will_qos_0 = 0, will_qos_1 = 0x08, will_qos_2 = 0x10;

    struct properties_t : detail::connect_connack_properties_t
    {
        bool request_response_information = false;
        bool request_problem_information = false;

        template <class Stream>
        static properties_t deserialize(transport::data_fetcher<Stream> data) {
            protocol::properties props;
            props.deserialize(data);
            properties_t retval;
            for (const auto &prop : props) {
                if (prop.identifier == property_ids::request_response_information) {
                    retval.request_response_information = prop.value_as<std::uint8_t>();
                }
                else if (prop.identifier == property_ids::request_problem_information) {
                    retval.request_problem_information = prop.value_as<std::uint8_t>();
                }
                else {
                    retval.handle_property(prop);
                }
            }
            return retval;
        }

        template <class Writer>
        void serialize(Writer &&writer) const {
            protocol::properties retval;
            properties_t dflt;
            auto maybe_add = [&](auto val, auto dflt, std::uint8_t prop) {
                if (val != dflt) {
                    retval.add_property(prop, val);
                }
            };
            using ids = property_ids;
            maybe_add(request_response_information, dflt.request_response_information,
                      ids::request_response_information);
            maybe_add(request_problem_information, dflt.request_problem_information,
                      ids::request_problem_information);
            add_base_properties(retval);
            retval.serialize(writer);
        }
    };

    std::uint8_t version = 5;
    std::uint8_t flags = 0x02;
    std::chrono::duration<std::uint16_t> keep_alive{240};
    properties_t connect_properties;
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
        connect_properties = properties_t::deserialize(fetcher);
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

struct connack
{
    struct properties_t: detail::connect_connack_properties_t
    {
        std::string assigned_client_id;
        std::string reason_string;
        std::string response_information;
        std::string server_reference;

        std::chrono::duration<std::uint16_t> server_keep_alive{0};

        std::uint8_t maximum_qos = 2;
        bool retain_available = true;
        bool wildcard_subscriptions_available = true;
        bool subscription_identifiers_available = true;
        bool shared_subscription_available = true;

        template<class Stream>
        static properties_t deserialize(transport::data_fetcher<Stream> data) {
            protocol::properties props;
            props.deserialize(data);
            properties_t retval;
            using ids = property_ids;

            for (const auto &prop : props) {
                if (prop.identifier == ids::maximum_qos) {
                    retval.maximum_qos = prop.value_as<std::uint8_t>();
                }
                else if (prop.identifier == ids::retain_available) {
                    retval.retain_available = prop.value_as<std::uint8_t>();
                }
                else if (prop.identifier == ids::assigned_client_id) {
                    retval.assigned_client_id = prop.value_as<std::string>();
                }
                else if (prop.identifier == ids::reason_string) {
                    retval.reason_string = prop.value_as<std::string>();
                }
                else if (prop.identifier == ids::wildcard_subscriptions_available) {
                    retval.wildcard_subscriptions_available = prop.value_as<std::uint8_t>();
                }
                else if (prop.identifier == ids::subscription_identifiers_available) {
                    retval.subscription_identifiers_available = prop.value_as<std::uint8_t>();
                }
                else if (prop.identifier == ids::shared_subscription_available) {
                    retval.shared_subscription_available = prop.value_as<std::uint8_t>();
                }
                else if (prop.identifier == ids::server_keep_alive) {
                    retval.server_keep_alive =
                        std::chrono::seconds{prop.value_as<std::uint16_t>()};
                }
                else if (prop.identifier == ids::response_information) {
                    retval.response_information = prop.value_as<std::string>();
                }
                else if (prop.identifier == ids::server_reference) {
                    retval.server_reference = prop.value_as<std::string>();
                }
                else {
                    retval.handle_property(prop);
                }
            }
            return retval;
        }

        template<class Writer>
        void serialize(Writer&& writer) const {
            protocol::properties retval;
            properties_t dflt;
            using ids = property_ids;

            auto maybe_add = [&](const auto &val, const auto &dflt, std::uint8_t prop) {
                if (val != dflt) {
                    retval.add_property(prop, val);
                }
            };

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

            this->add_base_properties(retval);

            retval.serialize(writer);
        }
    };

    static constexpr std::uint8_t type_value = 2;

    std::uint8_t flags;
    std::uint8_t reason_code;
    properties_t properties;

    connack() = default;
    template <class T>
    connack(std::in_place_t, T fetcher) {
        deserialize(fetcher);
    }

    template <class Stream>
    void deserialize(transport::data_fetcher<Stream> fetcher) {
        flags = fixed_int<std::uint8_t>::deserialize(fetcher);
        reason_code = fixed_int<std::uint8_t>::deserialize(fetcher);
        properties = properties_t::deserialize(fetcher);
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
};
} // namespace mqtt5_v2::protocol
