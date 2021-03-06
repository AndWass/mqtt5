
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5/protocol/binary.hpp>
#include <mqtt5/protocol/fixed_int.hpp>
#include <mqtt5/protocol/string.hpp>
#include <mqtt5/protocol/varlen_int.hpp>

#include <p0443_v2/start.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/bind.hpp>

#include <tuple>
#include <variant>
#include <vector>

namespace mqtt5::protocol
{
struct property_ids
{
    static constexpr std::uint8_t payload_format_indicator = 1, message_expiry_interval = 2,
                                  content_type = 3, response_topic = 8, correlation_data = 9,
                                  subscription_identifier = 11, session_expiry_interval = 17,
                                  assigned_client_id = 18, server_keep_alive = 19,
                                  authentication_method = 21, authentication_data = 22,
                                  request_problem_information = 23, will_delay_interval=24,
                                  request_response_information = 25, response_information = 26,
                                  server_reference = 28, reason_string = 31, receive_maximum = 33,
                                  topic_alias_maximum = 34, topic_alias = 35, maximum_qos = 36,
                                  retain_available = 37, user_property = 38,
                                  maximum_packet_size = 39, wildcard_subscriptions_available = 40,
                                  subscription_identifiers_available = 41,
                                  shared_subscription_available = 42;
};

struct property
{
    struct varlen_value
    {
        std::uint32_t value;

        varlen_value() = default;
        varlen_value(std::uint32_t v) noexcept : value(v) {
        }

        varlen_value &operator=(std::uint32_t v) noexcept {
            value = v;
            return *this;
        }

        operator std::uint32_t() noexcept {
            return value;
        }

        friend bool operator==(const varlen_value& lhs, const varlen_value& rhs) {
            return lhs.value == rhs.value;
        }
    };

    template <class Stream>
    struct deserializer
    {
        transport::data_fetcher<Stream> data;
        void operator()(std::uint8_t &v) {
            v = fixed_int<std::uint8_t>::deserialize(data);
        }
        void operator()(std::uint16_t &v) {
            v = fixed_int<std::uint16_t>::deserialize(data);
        }
        void operator()(std::uint32_t &v) {
            v = fixed_int<std::uint32_t>::deserialize(data);
        }
        void operator()(varlen_value &v) {
            v.value = varlen_int::deserialize(data);
        }

        void operator()(std::string &v) {
            v = string::deserialize(data);
        }

        void operator()(std::vector<std::uint8_t> &v) {
            v = binary::deserialize(data);
        }

        void operator()(key_value_pair &v) {
            v = key_value_pair::deserialize(data);
        }
    };

    template <class Writer>
    struct serializer
    {
        Writer *writer;
        void operator()(std::uint8_t v) {
            fixed_int<std::uint8_t>::serialize(v, *writer);
        }
        void operator()(std::uint16_t v) {
            fixed_int<std::uint16_t>::serialize(v, *writer);
        }
        void operator()(std::uint32_t v) {
            fixed_int<std::uint32_t>::serialize(v, *writer);
        }
        void operator()(varlen_value v) {
            varlen_int::serialize(v.value, *writer);
        }

        void operator()(const std::string &v) {
            string::serialize(v, *writer);
        }

        void operator()(const std::vector<std::uint8_t> &v) {
            binary::serialize(v, *writer);
        }

        void operator()(const key_value_pair &v) {
            key_value_pair::serialize(v, *writer);
        }
    };
    using value_storage = std::variant<std::uint8_t, std::uint16_t, std::uint32_t, varlen_value,
                                       std::string, std::vector<std::uint8_t>, key_value_pair>;

    template <class Stream>
    [[nodiscard]] static property deserialize(transport::data_fetcher<Stream> data) {
        property retval;
        retval.identifier = varlen_int::deserialize(data);
        retval.activate_id(retval.identifier);
        std::visit(deserializer<Stream>{data}, retval.value_);
        return retval;
    }

    template <class Writer>
    static void serialize(const property &prop, Writer &writer) {
        varlen_int::serialize(prop.identifier, writer);
        std::visit(serializer<Writer>{std::addressof(writer)}, prop.value_);
    }

    void activate_id(std::uint8_t id) {
#define ACTIVATE(X)                                                                                \
    value_.emplace<X>();                                                                           \
    break
        switch (id) {
        case 1:
            ACTIVATE(0);
        case 2:
            ACTIVATE(2);
        case 3:
            ACTIVATE(4);
        case 8:
            ACTIVATE(4);
        case 9:
            ACTIVATE(5);
        case 11:
            ACTIVATE(5);
        case 17:
            ACTIVATE(2);
        case 18:
            ACTIVATE(4);
        case 19:
            ACTIVATE(1);
        case 21:
            ACTIVATE(4);
        case 22:
            ACTIVATE(5);
        case 23:
            ACTIVATE(0);
        case 24:
            ACTIVATE(2);
        case 25:
            ACTIVATE(0);
        case 26:
            ACTIVATE(4);
        case 28:
            ACTIVATE(4);
        case 31:
            ACTIVATE(4);
        case 33:
            ACTIVATE(1);
        case 34:
            ACTIVATE(1);
        case 35:
            ACTIVATE(1);
        case 36:
            ACTIVATE(0);
        case 37:
            ACTIVATE(0);
        case 38:
            ACTIVATE(6);
        case 39:
            ACTIVATE(2);
        case 40:
            ACTIVATE(0);
        case 41:
            ACTIVATE(0);
        case 42:
            ACTIVATE(0);
        }
#undef ACTIVATE
    }

    template <class T>
    void set_id_value(std::uint8_t id, const T &val) {
        activate_id(id);
        identifier = id;
        set_value(val);
    }

    template <class T>
    void set_value(const T &val) {
        static_assert(
            boost::mp11::mp_any_of<value_storage, boost::mp11::mp_bind_back<std::is_assignable,
                                                                            T>::template fn>::value,
            "T cannot be used to assign to any potential property value");
        std::visit([&, this](auto &elem) { this->assign_value(elem, val); }, value_);
    }

    property() = default;

    template <class T>
    property(std::uint8_t id, const T &val) {
        set_id_value(id, val);
    }

    template <class T>
    T value_as() const {
        T value;
        std::visit([&, this](const auto &v) { get_value_into(value, v); }, value_);
        return value;
    }

    friend bool operator==(const property &lhs, const property &rhs) {
        return lhs.identifier == rhs.identifier && lhs.value_ == rhs.value_;
    }

    varlen_int::type identifier;
    value_storage value_;

private:
#define ENABLE_IF(...) std::enable_if_t<__VA_ARGS__> * = nullptr
    template <class T, ENABLE_IF(std::is_integral_v<T>)>
    void assign_value(varlen_value &elem, T val) {
        elem.value = static_cast<std::uint32_t>(val);
    }

    template <class U, class V, ENABLE_IF(std::is_integral_v<U> &&std::is_integral_v<V>)>
    void assign_value(U &elem, V val) {
        elem = static_cast<U>(val);
    }

    template <class T,
              ENABLE_IF(std::is_assignable_v<std::string &, const T &> && !std::is_integral_v<T>)>
    void assign_value(std::string &elem, const T &val) {
        elem = val;
    }

    void assign_value(std::vector<std::uint8_t> &elem, const std::vector<std::uint8_t> &val) {
        elem = val;
    }

    void assign_value(key_value_pair &elem, const key_value_pair &val) {
        elem = val;
    }

    template <class U, class V>
    struct mismatched_value_types
    {
        static constexpr bool value =
            !((std::is_same_v<U, varlen_value> && std::is_integral_v<V>) ||
              (std::is_integral_v<U> && std::is_integral_v<V>) ||
              (std::is_same_v<U, std::string> && std::is_assignable_v<U &, const V &> &&
               !std::is_integral_v<V>) ||
              (std::is_same_v<U, V>));
    };

    template <class U, class V, ENABLE_IF(mismatched_value_types<U, V>::value)>
    void assign_value(U &, const V &) {
        throw std::runtime_error("Mismatched attempt to set property types");
    }

    template <class T>
    void get_value_into(T &val, const T &rhs) const {
        val = rhs;
    }

    template <class T, ENABLE_IF(std::is_integral_v<T>)>
    void get_value_into(T &val, varlen_value stored) const {
        val = static_cast<T>(stored.value);
    }

    template <class U, class V, ENABLE_IF(std::is_integral_v<U> &&std::is_integral_v<V>)>
    void get_value_into(U &val, V elem) const {
        val = static_cast<U>(elem);
    }

    void get_value_into(std::string &val, const std::string &stored) const {
        val = stored;
    }

    void get_value_into(std::vector<std::uint8_t> &val,
                        const std::vector<std::uint8_t> &stored) const {
        val = stored;
    }

    void get_value_into(key_value_pair &val, const key_value_pair &stored) const {
        val = stored;
    }

    template <class U, class V>
    struct mismatched_get_value_types
    {
        static constexpr bool value =
            !((std::is_same_v<V, varlen_value> && std::is_integral_v<U>) ||
              (std::is_integral_v<U> && std::is_integral_v<V>) ||
              (std::is_same_v<U, std::string> && std::is_same_v<U, V>) || (std::is_same_v<U, V>));
    };

    template <class U, class V, ENABLE_IF(mismatched_get_value_types<U, V>::value)>
    void get_value_into(U &, const V &) const {
        throw std::runtime_error("Mismatched attempt to get property types");
    }

#undef ENABLE_IF
};

struct properties
{
    template <class Stream>
    void deserialize(transport::data_fetcher<Stream> data) {
        varlen_int::type property_data_length = varlen_int::deserialize(data);
        properties_.clear();
        auto data_span = data.cspan(property_data_length);
        while (!data_span.empty()) {
            properties_.emplace_back(
                property::deserialize(transport::buffer_data_fetcher(data_span)));
        }
        data.consume(property_data_length);
    }

    template <class Writer>
    void serialize(Writer &writer) const {
        std::uint32_t prop_len = 0;
        auto len_finder = [&](std::uint8_t) { prop_len++; };
        // Find the length of the contained properties
        for (const auto &p : properties_) {
            property::serialize(p, len_finder);
        }

        varlen_int::serialize(prop_len, writer);
        for (const auto &p : properties_) {
            property::serialize(p, writer);
        }
    }

    void set_properties(std::vector<property> props) {
        properties_ = std::move(props);
    }

    void add_property(property prop) {
        if (prop.identifier == 38) {
            properties_.emplace_back(std::move(prop));
        }
        else {
            auto iter = std::find_if(properties_.begin(), properties_.end(),
                                     [&](auto &p) { return p.identifier == prop.identifier; });
            if (iter != properties_.end()) {
                iter->value_ = std::move(prop.value_);
            }
            else {
                properties_.emplace_back(std::move(prop));
            }
        }
    }

    template <class T>
    void add_property(std::uint8_t id, const T &value) {
        add_property(property(id, value));
    }

    auto cbegin() const noexcept {
        return properties_.cbegin();
    }

    auto cend() const noexcept {
        return properties_.cend();
    }

    auto begin() const noexcept {
        return properties_.cbegin();
    }

    auto end() const noexcept {
        return properties_.cend();
    }

    void clear() noexcept {
        properties_.clear();
    }

    std::size_t size() const {
        return properties_.size();
    }

    bool empty() const {
        return properties_.empty();
    }

private:
    std::vector<property> properties_;
};

struct properties_t_base
{
    std::vector<key_value_pair> user_property;
    std::vector<property> unknown_properties;

protected:
    void handle_property(const property &prop) {
        if (prop.identifier == property_ids::user_property) {
            user_property.emplace_back(prop.value_as<key_value_pair>());
        }
        else {
            unknown_properties.emplace_back(prop);
        }
    }

    void add_base_properties(properties& props) const
    {
        for (auto&& kv : user_property) {
            props.add_property(property_ids::user_property, kv);
        }
        for (auto&& prop : unknown_properties) {
            props.add_property(prop);
        }
    }
};

} // namespace mqtt5::protocol
