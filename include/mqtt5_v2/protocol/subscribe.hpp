
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
} // namespace mqtt5_v2::protocol