#pragma once

#include <chrono>
#include <cstdint>
#include <numeric>
#include <optional>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "binary_packet.hpp"
#include "property.hpp"
#include "serialize.hpp"
#include "string.hpp"

namespace mqtt5
{
enum class quality_of_service : std::uint8_t { qos0 = 0, qos1, qos2, reserved };

struct connect_flags
{
    connect_flags() = default;
    connect_flags(std::uint8_t flags) {
        clean_start = flags & 0x02;
        will_flag = flags & 0x04;
        will_qos = static_cast<quality_of_service>((flags & 0x18) >> 3);
        will_retain = flags & 0x20;
        password = flags & 0x40;
        username = flags & 0x80;
    }

    std::uint8_t to_flags_byte() const noexcept {
        auto encode_bool = [](bool b, std::uint8_t shift_amount) {
            return b ? (1 << shift_amount) : 0;
        };
        std::uint8_t retval = encode_bool(clean_start, 1);
        retval += encode_bool(will_flag, 2);
        retval += encode_bool(will_retain, 5);
        retval += encode_bool(password, 6);
        retval += encode_bool(username, 7);
        retval += (static_cast<std::uint8_t>(will_qos) << 3);
        return retval;
    }
    bool clean_start = false;
    bool will_flag = false;
    quality_of_service will_qos = quality_of_service::qos0;
    bool will_retain = false;
    bool password = false;
    bool username = false;
};

struct will_data
{
    struct properties_type
    {
        string content_type;
        string response_topic;
        binary correlation_data;
        std::vector<key_value_pair> user_properties;
        std::chrono::duration<std::uint32_t> delay_interval{0};
        std::optional<std::chrono::duration<std::uint32_t>> message_expiry_interval;
        std::uint8_t payload_format_indicator{0};
    };

    properties_type properties;
    string topic;
    binary payload;
};

class connect
{
public:
    static constexpr std::uint8_t protocol_version_5 = 5;
    will_data will;
    connect_flags flags;
    std::chrono::seconds keep_alive = std::chrono::seconds{240};
    integer32 session_expiry_interval{0};
    integer16 receive_maximum{65535};
    integer32 maximum_packet_size{varlen_integer::max_value};
    integer16 topic_alias_maximum{0};
    bool request_response_information = false;
    bool request_problem_information = false;
    std::vector<key_value_pair> user_properties;
    string authentication_method;
    binary authentication_data;
    string client_id;
    string username;
    string password;

    static string generate_client_id() noexcept {
        static const std::string_view string_characters =
            "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        auto uuid1 = boost::uuids::random_generator()();
        auto uuid2 = boost::uuids::random_generator()();
        std::string retval;

        auto add_uuid = [&retval](const auto &uuid) {
            for (auto b : uuid) {
                if (retval.size() == 23) {
                    break;
                }
                retval += string_characters[b % string_characters.size()];
            }
        };

        add_uuid(uuid1);
        add_uuid(uuid2);

        return string{retval};
    }

private:
};

template <class Iter>
[[nodiscard]] Iter serialize(const will_data &will, Iter out) {
    std::vector<property> properties;
    if (will.properties.delay_interval.count() > 0) {
        properties.emplace_back(property_id::will_delay_interval,
                                integer32{will.properties.delay_interval.count()});
    }
    if (will.properties.payload_format_indicator) {
        properties.emplace_back(property_id::payload_format_indicator,
                                will.properties.payload_format_indicator);
    }
    if (will.properties.message_expiry_interval) {
        properties.emplace_back(property_id::message_expiry_interval,
                                integer32{*will.properties.message_expiry_interval});
    }
    if (!will.properties.content_type.empty()) {
        properties.emplace_back(property_id::content_type, will.properties.content_type);
    }
    if (!will.properties.response_topic.empty()) {
        properties.emplace_back(property_id::response_topic, will.properties.response_topic);
    }
    if (!will.properties.correlation_data.empty()) {
        properties.emplace_back(property_id::correlation_data, will.properties.correlation_data);
    }
    for (auto &up : will.properties.user_properties) {
        properties.emplace_back(property_id::user_property, up);
    }
    out = mqtt5::serialize(properties, out);
    out = mqtt5::serialize(will.topic, out);
    return mqtt5::serialize(will.payload, out);
}

template <class Iter>
[[nodiscard]] Iter serialize(const connect &c, Iter out) {
    string mqtt_string("MQTT");
    out = mqtt5::serialize(mqtt_string, out);
    *out = connect::protocol_version_5;
    ++out;
    *out = c.flags.to_flags_byte();
    out++;
    if (c.keep_alive.count() > integer16::max_value) {
        throw std::invalid_argument("Keep alive must not be greater than 65535");
    }
    integer16 i16(c.keep_alive.count());
    out = mqtt5::serialize(i16, out);

    std::vector<property> properties;
#define MAYBE_ADD_PROPERTY(X, V)                                                                   \
    if (c.X.value() != V)                                                                          \
        properties.emplace_back(property_id::X, c.X);
    MAYBE_ADD_PROPERTY(session_expiry_interval, 0)
    MAYBE_ADD_PROPERTY(receive_maximum, 65535)
    MAYBE_ADD_PROPERTY(maximum_packet_size, varlen_integer::max_value)
    MAYBE_ADD_PROPERTY(topic_alias_maximum, 0)
#undef MAYBE_ADD_PROPERTY
    if (c.request_response_information) {
        properties.emplace_back(property_id::request_response_information, std::uint8_t(1));
    }
    if (c.request_problem_information) {
        properties.emplace_back(property_id::request_problem_information, std::uint8_t(1));
    }
    if (!c.authentication_method.empty()) {
        properties.emplace_back(property_id::authentication_method, c.authentication_method);
        properties.emplace_back(property_id::authentication_data, c.authentication_data);
    }

    for (auto &up : c.user_properties) {
        properties.emplace_back(property_id::user_property, up);
    }

    out = mqtt5::serialize(properties, out);
    out = mqtt5::serialize(c.client_id, out);

    return out;
}
} // namespace mqtt5