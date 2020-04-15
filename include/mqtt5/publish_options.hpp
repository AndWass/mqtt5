
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/mp11/tuple.hpp>
#include <tuple>

#include "mqtt5/protocol/publish.hpp"
#include "quality_of_service.hpp"

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
    retain(bool b) : retain_(b) {
    }
    void operator()(protocol::publish &publish) const {
        publish.set_retain(retain_);
    }
};
struct response_topic
{
    std::string response_topic_;
    response_topic(const std::string &rt) : response_topic_(rt) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.response_topic = response_topic_;
    }
};
struct content_type
{
    std::string value_;
    content_type(const std::string &value) : value_(value) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.content_type = value_;
    }
};
struct topic_alias
{
    std::uint16_t value_;
    topic_alias(std::uint16_t value) : value_(value) {
    }
    void operator()(protocol::publish &publish) const {
        publish.properties.topic_alias = value_;
    }
};

template<class M>
struct modifier
{
    M modifier_;

    modifyer(const M& m): modifier_(m) {}
    modifier(M&& m): modifier_(std::move(m)) {}
};

template<class T>
struct is_modifier: std::false_type {};

template<class T>
struct is_modifier<modifier<T>>: std::true_type {};

template <class T>
using is_publish_option =
    std::disjunction<std::is_same<T, quality_of_service>, std::is_same<T, retain>,
                     std::is_same<T, response_topic>, std::is_same<T, content_type>,
                     std::is_same<T, topic_alias>,
                     is_modifier<T>
    >;

template <class... Options>
struct option_bag
{
    std::tuple<Options...> options_;
    option_bag(Options &&... values) : options_(std::forward<Options>(values)...) {
    }

    void operator()(protocol::publish &publish) const {
        boost::mp11::tuple_for_each(options_, [&](auto &opt) { opt(publish); });
    }
};

template <class... Options>
auto make_options(Options &&... options) {
    return option_bag<p0443_v2::remove_cvref_t<Options>...>(std::forward<Options>(options)...);
}
} // namespace publish_options
} // namespace mqtt5