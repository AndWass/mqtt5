#pragma once

#include <chrono>
#include <cstdint>
#include <numeric>
#include <optional>

#include "mqtt5/type/serialize.hpp"

#include "mqtt5/type/property.hpp"
#include "mqtt5/type/string.hpp"

#include "mqtt5/publish.hpp"

#include <boost/throw_exception.hpp>
#include <boost/system/system_error.hpp>

namespace mqtt5
{
namespace message
{
struct connect_flags
{
    connect_flags() = default;
    connect_flags(std::uint8_t flags) {
        clean_start = flags & 0x02;
        will_flag = flags & 0x04;
        will_qos = static_cast<publish::quality_of_service>((flags & 0x18) >> 3);
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
    bool clean_start = true;
    bool will_flag = false;
    publish::quality_of_service will_qos = publish::quality_of_service::qos0;
    bool will_retain = false;
    bool password = false;
    bool username = false;
};

struct will_data
{
    struct properties_type
    {
        type::string content_type;
        type::string response_topic;
        type::binary correlation_data;
        std::vector<type::key_value_pair> user_properties;
        std::chrono::duration<std::uint32_t> delay_interval{0};
        std::optional<std::chrono::duration<std::uint32_t>> message_expiry_interval;
        type::integer8 payload_format_indicator{0};
    };

    properties_type properties;
    type::string topic;
    type::binary payload;

    will_data &operator=(const mqtt5::publish &pub) {
        properties.content_type = pub.content_type;
        properties.response_topic = pub.response_topic;
        properties.correlation_data = pub.correlation_data;
        for (const auto &kv : pub.user_properties) {
            properties.user_properties.emplace_back(kv.first, kv.second);
        }
        // properties.user_properties = pub.user_properties;
        properties.message_expiry_interval = pub.message_expiry_interval;
        properties.payload_format_indicator = pub.payload_format_indicator;
        topic = pub.topic_name;
        payload = pub.payload;
        return *this;
    }
};

class connect
{
public:
    static constexpr std::uint8_t protocol_version_5 = 5;
    will_data will;
    connect_flags flags;
    std::chrono::seconds keep_alive = std::chrono::seconds{240};
    type::integer32 session_expiry_interval{0};
    type::integer16 receive_maximum{65535};
    type::integer32 maximum_packet_size{type::varlen_integer::max_value};
    type::integer16 topic_alias_maximum{0};
    bool request_response_information = false;
    bool request_problem_information = false;
    std::vector<type::key_value_pair> user_properties;
    type::string authentication_method;
    type::binary authentication_data;
    type::string client_id;
    type::string username;
    type::string password;

private:
};

namespace detail
{
template <class Iter>
[[nodiscard]] Iter serialize(const will_data &will, Iter out) {
    using type::property;
    using type::property_id;
    using type::binary;
    using type::key_value_pair;
    using type::string;
    using type::integer8;
    using type::integer16;
    using type::integer32;

    std::vector<property> properties;
    if (will.properties.delay_interval.count() > 0) {
        properties.emplace_back(property_id::will_delay_interval,
                                integer32{will.properties.delay_interval.count()});
    }
    if (will.properties.payload_format_indicator.value()) {
        properties.emplace_back(property_id::payload_format_indicator,
                                will.properties.payload_format_indicator);
    }
    if (will.properties.message_expiry_interval) {
        properties.emplace_back(property_id::message_expiry_interval,
                                integer32(will.properties.message_expiry_interval->count()));
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
    out = type::serialize(properties, out);
    out = type::serialize(will.topic, out);
    return type::serialize(will.payload, out);
}
}

template <class Iter>
[[nodiscard]] Iter serialize(const connect &c, Iter out) {
    using type::string;
    using type::integer8;
    using type::integer16;
    using type::integer32;
    using type::varlen_integer;

    string mqtt_string("MQTT");
    out = type::serialize(mqtt_string, out);
    *out = connect::protocol_version_5;
    ++out;
    *out = c.flags.to_flags_byte();
    out++;
    if (c.keep_alive.count() > integer16::max_value) {
        boost::throw_exception(boost::system::system_error(
                boost::system::errc::make_error_code(boost::system::errc::value_too_large)));
    }
    integer16 i16(static_cast<std::uint16_t>(c.keep_alive.count()));
    out = type::serialize(i16, out);

    std::vector<type::property> properties;
#define MAYBE_ADD_PROPERTY(X, V)                                                                   \
    if (c.X.value() != V)                                                                          \
        properties.emplace_back(type::property_id::X, c.X);
    MAYBE_ADD_PROPERTY(session_expiry_interval, 0)
    MAYBE_ADD_PROPERTY(receive_maximum, 65535)
    MAYBE_ADD_PROPERTY(maximum_packet_size, varlen_integer::max_value)
    MAYBE_ADD_PROPERTY(topic_alias_maximum, 0)
#undef MAYBE_ADD_PROPERTY
    if (c.request_response_information) {
        properties.emplace_back(type::property_id::request_response_information, integer8(1));
    }
    if (c.request_problem_information) {
        properties.emplace_back(type::property_id::request_problem_information, integer8(1));
    }
    if (!c.authentication_method.empty()) {
        properties.emplace_back(type::property_id::authentication_method, c.authentication_method);
        properties.emplace_back(type::property_id::authentication_data, c.authentication_data);
    }

    for (auto &up : c.user_properties) {
        properties.emplace_back(type::property_id::user_property, up);
    }

    out = type::serialize(properties, out);
    out = type::serialize(c.client_id, out);

    if (c.flags.will_flag) {
        out = detail::serialize(c.will, out);
    }

    if (c.flags.username) {
        out = type::serialize(c.username, out);
    }
    if (c.flags.password) {
        out = type::serialize(c.password, out);
    }

    return out;
}
} // namespace message
} // namespace mqtt5