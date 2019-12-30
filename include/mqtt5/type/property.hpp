#pragma once

#include "binary.hpp"
#include "integer.hpp"
#include "serialize.hpp"
#include "string.hpp"

#include <numeric>
#include <variant>

namespace mqtt5
{
namespace type
{
enum class property_id : std::uint8_t {
    unused = 0,
    payload_format_indicator = 1,
    message_expiry_interval = 2,
    content_type = 3,
    response_topic = 8,
    correlation_data = 9,
    subscription_identifier = 11,
    session_expiry_interval = 17,
    assigned_client_id = 18,
    server_keep_alive = 19,
    authentication_method = 21,
    authentication_data = 22,
    request_problem_information = 23,
    will_delay_interval = 24,
    request_response_information = 25,
    response_information = 26,
    server_reference = 28,
    reason_string = 31,
    receive_maximum = 33,
    topic_alias_maximum = 34,
    topic_alias = 35,
    maximum_quality_of_service = 36,
    retain_available = 37,
    user_property = 38,
    maximum_packet_size = 39,
    wildcard_subscription_available = 40,
    subscription_identifiers_available = 41,
    shared_subscription_available = 42
};
namespace detail
{
template <std::uint8_t Id>
constexpr property_id uint8_to_property_id = static_cast<property_id>(Id);
}
struct property
{
    using value_type = std::variant<integer8, integer16, integer32, varlen_integer, string,
                                    binary, key_value_pair>;

    property() noexcept = default;
    template <class T>
    property(property_id id, T value) : id(id), value(value) {
    }

    property(property_id id, bool value) : id(id), value(integer8(value ? 1 : 0)) {
    }
    property_id id = property_id::unused;
    value_type value;
};

namespace detail
{
template <property_id Id, typename T>
struct property_value_serializer
{
    template <class Iter>
    [[nodiscard]] static Iter serialize(const property &p, Iter out) {
        return type::serialize(std::get<T>(p.value), out);
    }
    template <class Iter>
    [[nodiscard]] static Iter deserialize_into(property &p, Iter begin, Iter end) {
        T value;
        begin = type::deserialize_into(value, begin, end);
        p.id = Id;
        p.value = value;
        return begin;
    }
};
template <std::uint8_t Id>
struct property_traits
{};
#define PROPERTY_TRAITS(X, T)                                                                      \
    template <>                                                                                    \
    struct property_traits<X>                                                                      \
    {                                                                                              \
        using value_type = T;                                                                      \
        using value_serializer = property_value_serializer<detail::uint8_to_property_id<X>, T>;    \
    }
PROPERTY_TRAITS(1, integer8);
PROPERTY_TRAITS(2, integer32);
PROPERTY_TRAITS(3, string);
PROPERTY_TRAITS(8, string);
PROPERTY_TRAITS(9, binary);
PROPERTY_TRAITS(11, varlen_integer);
PROPERTY_TRAITS(17, integer32);
PROPERTY_TRAITS(18, string);
PROPERTY_TRAITS(19, integer16);
PROPERTY_TRAITS(21, string);
PROPERTY_TRAITS(22, binary);
PROPERTY_TRAITS(23, integer8);
PROPERTY_TRAITS(24, integer32);
PROPERTY_TRAITS(25, integer8);
PROPERTY_TRAITS(26, string);
PROPERTY_TRAITS(28, string);
PROPERTY_TRAITS(31, string);
PROPERTY_TRAITS(33, integer16);
PROPERTY_TRAITS(34, integer16);
PROPERTY_TRAITS(35, integer16);
PROPERTY_TRAITS(36, integer8);
PROPERTY_TRAITS(37, integer8);
PROPERTY_TRAITS(38, key_value_pair);
PROPERTY_TRAITS(39, integer32);
PROPERTY_TRAITS(40, integer8);
PROPERTY_TRAITS(41, integer8);
PROPERTY_TRAITS(42, integer8);
#undef PROPERTY_TRAITS
} // namespace detail

template <class Iter>
[[nodiscard]] Iter serialize(const property &prop, Iter out) {
    varlen_integer vi(static_cast<std::uint8_t>(prop.id));
    out = type::serialize(vi, out);
#define CASE(X)                                                                                    \
    case detail::uint8_to_property_id<X>:                                                          \
        return detail::property_traits<X>::value_serializer::template serialize(prop, out)
    switch (prop.id) {
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(8);
        CASE(9);
        CASE(11);
        CASE(17);
        CASE(18);
        CASE(19);
        CASE(21);
        CASE(22);
        CASE(23);
        CASE(24);
        CASE(25);
        CASE(26);
        CASE(28);
        CASE(31);
        CASE(33);
        CASE(34);
        CASE(35);
        CASE(36);
        CASE(37);
        CASE(38);
        CASE(39);
        CASE(40);
        CASE(41);
        CASE(42);
    default:
        throw std::invalid_argument("cannot serialize unknown property");
#undef CASE
    }
}

template <class Iter>
[[nodiscard]] Iter serialize(const std::vector<property> &properties, Iter out) {
    varlen_integer property_length(
        std::accumulate(properties.begin(), properties.end(), 0u, [](auto acc, const auto &prop) {
            return acc + serialized_size_of(prop);
        }));
    out = type::serialize(property_length, out);
    for (const auto &property : properties) {
        out = type::serialize(property, out);
    }
    return out;
}

template <class Iter>
[[nodiscard]] Iter deserialize_into(property &prop, Iter begin, Iter end) {
    varlen_integer vi;
    begin = type::deserialize_into(vi, begin, end);
    prop.id = static_cast<property_id>(vi.value());
#define CASE(X)                                                                                    \
    case detail::uint8_to_property_id<X>:                                                          \
        return detail::property_traits<X>::value_serializer::deserialize_into(prop, begin, end)
    switch (prop.id) {
        CASE(1);
        CASE(2);
        CASE(3);
        CASE(8);
        CASE(9);
        CASE(11);
        CASE(17);
        CASE(18);
        CASE(19);
        CASE(21);
        CASE(22);
        CASE(23);
        CASE(24);
        CASE(25);
        CASE(26);
        CASE(28);
        CASE(31);
        CASE(33);
        CASE(34);
        CASE(35);
        CASE(36);
        CASE(37);
        CASE(38);
        CASE(39);
        CASE(40);
        CASE(41);
        CASE(42);
    default:
        throw std::invalid_argument("cannot serialize unknown property");
#undef CASE
    }
}

template <class Iter>
[[nodiscard]] Iter deserialize_into(std::vector<property> &properties, Iter begin, Iter end) {
    type::varlen_integer proplen;
    begin = type::deserialize_into(proplen, begin, end);
    end = begin + proplen.value();
    while (begin != end) {
        property prop;
        begin = type::deserialize_into(prop, begin, end);
        properties.push_back(prop);
    }
    return end;
}
} // namespace type
} // namespace mqtt5