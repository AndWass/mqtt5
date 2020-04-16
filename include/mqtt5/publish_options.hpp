
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/mp11/tuple.hpp>
#include <tuple>

#include "mqtt5/protocol/publish.hpp"
#include "payload_format_indicator.hpp"
#include "quality_of_service.hpp"

#include <functional>

namespace mqtt5
{
namespace publish_options
{
struct quality_of_service
{
    mqtt5::quality_of_service qos_;
    quality_of_service(mqtt5::quality_of_service qos) : qos_(qos) {
    }

    void operator()(protocol::publish &publish) const {
        publish.set_quality_of_service(qos_);
    }
};
struct retain
{
    bool retain_;
    explicit retain(bool b) : retain_(b) {
    }
    void operator()(protocol::publish &publish) const {
        publish.set_retain(retain_);
    }
};
struct response_topic
{
    std::string response_topic_;
    explicit response_topic(std::string rt) : response_topic_(std::move(rt)) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.response_topic = response_topic_;
    }
};
struct content_type
{
    std::string value_;
    explicit content_type(std::string value) : value_(std::move(value)) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.content_type = value_;
    }
};

struct correlation_data
{
    mqtt5::protocol::binary::type value_;

    template <class... Args>
    explicit correlation_data(Args &&... args) : value_(std::forward<Args>(args)...) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.correlation_data = value_;
    }
};

struct message_expiry_interval
{
    std::chrono::seconds value_;

    explicit message_expiry_interval(std::chrono::seconds value) : value_(value) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.message_expiry_interval = value_;
    }
};

struct payload_format_indicator
{
    mqtt5::payload_format_indicator value_;

    explicit payload_format_indicator(mqtt5::payload_format_indicator value) : value_(value) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.payload_format_indicator = value_;
    }
};

struct topic_alias
{
    std::uint16_t value_;
    explicit topic_alias(std::uint16_t value) : value_(value) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.topic_alias = value_;
    }
};

template <class M>
struct late_modifier
{
    M modifier_;

    late_modifier(M m) : modifier_(std::move(m)) {
    }

    void operator()(protocol::publish &pub) {
        modifier_(pub);
    }
};

template <class T>
struct is_late_modifier : std::false_type
{};

template <class T>
struct is_late_modifier<late_modifier<T>> : std::true_type
{};

template <class T>
using is_publish_option =
    std::disjunction<std::is_same<T, quality_of_service>, std::is_same<T, retain>,
                     std::is_same<T, response_topic>, std::is_same<T, content_type>,
                     std::is_same<T, topic_alias>, is_late_modifier<T>>;

namespace detail
{
template <class Options, class LateModifiers>
struct options_and_modifiers
{
    Options options;
    LateModifiers modifiers;

    options_and_modifiers() = default;
    options_and_modifiers(Options opts, LateModifiers modifiers)
        : options(std::move(opts)), modifiers(std::move(modifiers)) {
    }
};
template <class OptionsAndModifiers, class Option>
auto add_one(OptionsAndModifiers &&oam, Option &&opt) {
    using option_t = std::decay_t<Option>;

    if constexpr (std::is_same_v<option_t, mqtt5::quality_of_service>) {
        return options_and_modifiers(
            std::tuple_cat(std::move(oam.options),
                           std::tuple{publish_options::quality_of_service(opt)}),
            std::move(oam.modifiers));
    }
    else if constexpr (std::is_same_v<option_t, mqtt5::payload_format_indicator>) {
        return options_and_modifiers(
            std::tuple_cat(std::move(oam.options),
                           std::tuple{publish_options::payload_format_indicator(opt)}),
            std::move(oam.modifiers));
    }
    else if constexpr (std::is_invocable_v<option_t, mqtt5::protocol::publish &> &&
                       !is_publish_option<option_t>::value) {
        return options_and_modifiers(
            std::move(oam.options),
            std::tuple_cat(std::move(oam.modifiers),
                           std::tuple{publish_options::late_modifier{std::move(opt)}}));
    }
    else {
        return options_and_modifiers(
            std::tuple_cat(std::move(oam.options), std::tuple{std::forward<Option>(opt)}),
            std::move(oam.modifiers));
    }
}

template <class OptionsAndModifiers, class Modifier>
auto add_one(OptionsAndModifiers &&oam,
             mqtt5::publish_options::late_modifier<Modifier> &&modifier) {
    return options_and_modifiers(
        std::move(oam.options),
        std::tuple_cat(std::move(oam.modifiers), std::tuple{std::move(modifier)}));
}

template <class OptsModifiers>
auto next_separated(OptsModifiers &&oam) {
    return oam;
}

template <class OptsModifiers, class Next, class... T>
auto next_separated(OptsModifiers &&oam, Next &&next, T &&... rest) {
    return mqtt5::publish_options::detail::next_separated(
        mqtt5::publish_options::detail::add_one(std::move(oam), std::move(next)),
        std::move(rest)...);
}

template <class... Ts>
auto separate_options_modifiers(Ts... next) {
    options_and_modifiers<std::tuple<>, std::tuple<>> base;
    return mqtt5::publish_options::detail::next_separated(std::move(base), std::move(next)...);
}
} // namespace detail
} // namespace publish_options
} // namespace mqtt5