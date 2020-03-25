
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5_v2/transport/data_fetcher.hpp>
#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>
#include <mqtt5_v2/protocol/properties.hpp>
#include <mqtt5_v2/protocol/string.hpp>
#include <mqtt5_v2/protocol/header.hpp>

#include <p0443_v2/sequence.hpp>
#include <p0443_v2/sync_wait.hpp>

#include <stdexcept>
#include <variant>
#include <vector>

namespace mqtt5_v2::protocol
{
class subscribe
{
public:
    struct topic_filter
    {
        topic_filter() = default;
        template<class T, class U>
        topic_filter(T&& t, U&& u): filter(std::forward<T>(t)), options(std::forward<U>(u)) {}
        string filter;
        fixed_int<std::uint8_t> options;
    };

private:
    fixed_int<std::uint16_t> packet_identifier_;
    properties properties_;
    std::variant<std::vector<topic_filter>, std::vector<std::uint8_t>> filters_or_raw_;

    void set_data_from_raw() {
        auto data = std::get<1>(std::move(filters_or_raw_));

        auto &filters_ref = filters_or_raw_.emplace<0>();
        auto buffer_fetcher = transport::buffer_data_fetcher(data);
        p0443_v2::sync_wait(
            p0443_v2::sequence(packet_identifier_.inplace_deserializer(buffer_fetcher),
                               properties_.inplace_deserializer(buffer_fetcher)));
        while (!data.empty()) {
            topic_filter next_filter;
            p0443_v2::sync_wait(
                p0443_v2::sequence(next_filter.filter.inplace_deserializer(buffer_fetcher),
                                   next_filter.options.inplace_deserializer(buffer_fetcher)));
            filters_ref.emplace_back(std::move(next_filter));
        }
    }
public:
    static constexpr std::uint8_t type_value = 8;
    subscribe() = default;
    subscribe(std::in_place_t, std::uint32_t remaining_length)
        : filters_or_raw_(std::in_place_index<1>, remaining_length) {
    }

    void set_packet_identifier(std::uint16_t id) {
        packet_identifier_ = id;
    }

    std::uint16_t packet_identifier() const {
        packet_identifier_.value;
    }

    void set_properties(properties props) {
        properties_ = std::move(props);
    }

    const protocol::properties& properties() const {
        return properties_;
    }

    protocol::properties& properties() {
        return properties_;
    }

    std::vector<topic_filter>& topic_filters() {
        if(filters_or_raw_.index() != 0) {
            return filters_or_raw_.template emplace<0>();
        }
        return std::get<0>(filters_or_raw_);
    }

    std::vector<topic_filter> topic_filters() const {
        if(filters_or_raw_.index() != 0) {
            return {};
        }
        return std::get<0>(filters_or_raw_);
    }

    void set_topic_filters(std::vector<topic_filter> filters) {
        filters_or_raw_.template emplace<0>(std::move(filters));
    }

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data) {
        if (filters_or_raw_.index() != 1) {
            throw std::runtime_error(
                "subscribe must be initialized with remaining length to deserialize");
        }

        return p0443_v2::transform(
            protocol::inplace_deserializer(std::get<1>(filters_or_raw_), data), [this](auto f) {
                set_data_from_raw();
                return f;
            });
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        packet_identifier_.serialize(writer);
        properties_.serialize(writer);
        if (filters_or_raw_.index() == 0) {
            for (auto &f : std::get<0>(filters_or_raw_)) {
                f.filter.serialize(writer);
                f.options.serialize(writer);
            }
        }
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, 2, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};
class suback
{
public:
    fixed_int<std::uint16_t> packet_identifier_;
    properties properties_;
    std::vector<std::uint8_t> reason_codes_;
private:
    void set_data_from_raw() {
        auto fetcher = transport::buffer_data_fetcher(reason_codes_);
        p0443_v2::sync_wait(
            p0443_v2::sequence(
                packet_identifier_.inplace_deserializer(fetcher),
                properties_.inplace_deserializer(fetcher)
            )
        );
        // reason_codes_ will now contain all the results
    }
public:
    static constexpr std::uint8_t type_value = 9;

    suback() = default;
    suback(std::in_place_t, std::uint32_t remaining_length): reason_codes_(remaining_length) {}

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data) {
        if (reason_codes_.empty()) {
            throw std::runtime_error(
                "subscribe must be initialized with remaining length to deserialize");
        }

        return p0443_v2::transform(
            protocol::inplace_deserializer(reason_codes_, data), [this](auto f) {
                set_data_from_raw();
                return f;
            });
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        packet_identifier_.serialize(writer);
        properties_.serialize(writer);
        for(auto b: reason_codes_) {
            writer(b);
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