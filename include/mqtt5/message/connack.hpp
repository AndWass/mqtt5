#pragma once

#include <cstdint>

#include <boost/throw_exception.hpp>
#include <boost/system/system_error.hpp>

#include "mqtt5/type/integer.hpp"
#include "mqtt5/type/property.hpp"
#include "mqtt5/type/serialize.hpp"

#include <chrono>
#include <optional>

namespace mqtt5
{
namespace message
{
class connack
{
public:
    enum class reason : std::uint8_t {
        success,
        unspecified_error = 0x80,
        malformed_packet,
        protocol_error,
        implementation_specified_error,
        unsupported_protocol,
        invalid_client_id,
        bad_username_or_password,
        not_authorized,
        unavailable,
        busy,
        banned,
        bad_authentication_method = 140,
        topic_name_invalid = 144,
        packet_too_large = 149,
        quota_exceeded = 151,
        payload_format_invalid = 153,
        retain_not_supported,
        qos_not_supported,
        use_another_server,
        server_moved,
        connection_rate_exceeded = 159
    };
    bool session_present = false;
    reason reason_code{reason::success};
    std::optional<std::chrono::seconds> session_expiry_interval{0};
    type::integer16 receive_maximum{65535};
    type::integer8 max_qos{2};
    bool retain_available{true};
    type::integer32 maximum_packet_size{type::varlen_integer::max_value};
    type::string assigned_client_id;
    type::integer16 topic_alias_maximum{0};
    type::string reason_string;
    std::vector<type::key_value_pair> user_properties;
    bool wildcard_subscription_available{true};
    bool subscription_identifiers_available{true};
    bool shared_subscription_available{true};
    std::optional<std::chrono::seconds> keep_alive;
    type::string response_information;
    type::string server_reference;
    type::string authentication_method;
    type::binary authentication_data;

private:
};

template <class Iter>
[[nodiscard]] Iter deserialize_into(connack::reason &r, Iter begin, Iter end) {
    type::integer8 code;
    begin = type::deserialize_into(code, begin, end);
    r = static_cast<connack::reason>(code.value());
    return begin;
}

template <class Iter>
[[nodiscard]] Iter deserialize_into(connack &c, Iter begin, Iter end) {
    using property_id = type::property_id;
    using type::binary;
    using type::key_value_pair;
    using type::string;
    using type::integer8;
    using type::integer16;
    using type::integer32;

    begin = type::deserialize_into(c.session_present, begin, end);
    begin = deserialize_into(c.reason_code, begin, end);

    std::vector<mqtt5::type::property> properties;
    begin = type::deserialize_into(properties, begin, end);
    for (const auto &prop : properties) {
        switch (prop.id) {
        case property_id::session_expiry_interval:
            c.session_expiry_interval =
                std::chrono::seconds{std::get<integer32>(prop.value).value()};
            break;
        case property_id::receive_maximum:
            c.receive_maximum = std::get<integer16>(prop.value);
            break;
        case property_id::maximum_quality_of_service:
            c.max_qos = std::get<integer8>(prop.value);
            break;
        case property_id::retain_available:
            c.retain_available = std::get<integer8>(prop.value).value();
            break;
        case property_id::maximum_packet_size:
            c.maximum_packet_size = std::get<integer32>(prop.value);
            break;
        case property_id::assigned_client_id:
            c.assigned_client_id = std::get<string>(prop.value);
            break;
        case property_id::topic_alias_maximum:
            c.topic_alias_maximum = std::get<integer16>(prop.value);
            break;
        case property_id::reason_string:
            c.reason_string = std::get<string>(prop.value);
            break;
        case property_id::user_property:
            c.user_properties.push_back(std::get<key_value_pair>(prop.value));
            break;
        case property_id::wildcard_subscription_available:
            c.wildcard_subscription_available = std::get<integer8>(prop.value).value();
            break;
        case property_id::subscription_identifiers_available:
            c.subscription_identifiers_available = std::get<integer8>(prop.value).value();
            break;
        case property_id::shared_subscription_available:
            c.shared_subscription_available = std::get<integer8>(prop.value).value();
            break;
        case property_id::server_keep_alive:
            c.keep_alive = std::chrono::seconds{std::get<integer16>(prop.value).value()};
            break;
        case property_id::response_information:
            c.response_information = std::get<string>(prop.value);
            break;
        case property_id::server_reference:
            c.server_reference = std::get<string>(prop.value);
            break;
        case property_id::authentication_method:
            c.authentication_method = std::get<string>(prop.value);
            break;
        case property_id::authentication_data:
            c.authentication_data = std::get<binary>(prop.value);
            break;
        default:
            boost::throw_exception(boost::system::system_error(
                boost::system::errc::make_error_code(boost::system::errc::protocol_error)));
        }
    }
    return begin;
}
} // namespace message
} // namespace mqtt5