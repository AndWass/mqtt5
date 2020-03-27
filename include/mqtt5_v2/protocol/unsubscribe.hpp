
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <mqtt5_v2/protocol/properties.hpp>
#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/string.hpp>
#include <mqtt5_v2/protocol/header.hpp>
#include <mqtt5_v2/transport/data_fetcher.hpp>

namespace mqtt5_v2::protocol
{
struct unsubscribe
{
    static constexpr std::uint8_t type_value = 10;

    std::uint16_t packet_identifier;
    mqtt5_v2::protocol::properties properties;
    std::vector<std::string> topics;

    unsubscribe() = default;
    template<class DataFetcher>
    unsubscribe(std::in_place_t, header hdr, DataFetcher fetcher) {
        deserialize(hdr, fetcher);
    }

    void deserialize(transport::buffer_data_fetcher_t<nonstd::span<const std::uint8_t>> data)
    {
        packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        properties.deserialize(data);
        while(!data.empty())
        {
            topics.emplace_back();
            topics.back() = string::deserialize(data);
        }
    }

    template <class Stream>
    void deserialize(header hdr, transport::data_fetcher<Stream> data) {
        auto data_span = data.cspan(hdr.remaining_length());
        auto my_data = transport::buffer_data_fetcher(data_span);
        deserialize(my_data);
    }

    template<class Writer>
    void serialize_body(Writer&& writer) const {
        fixed_int<std::uint16_t>::serialize(packet_identifier, writer);
        properties.serialize(writer);
        for(const auto& t: topics) {
            protocol::string::serialize(t, writer);
        }
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        header hdr(type_value, 2, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};

struct unsuback
{
    static constexpr std::uint8_t type_value = 11;

    std::uint16_t packet_identifier;
    mqtt5_v2::protocol::properties properties;
    std::vector<std::uint8_t> reason_codes;

    unsuback() = default;
    template<class DataFetcher>
    unsuback(std::in_place_t, header hdr, DataFetcher fetcher) {
        deserialize(hdr, fetcher);
    }

    void deserialize(transport::buffer_data_fetcher_t<nonstd::span<const std::uint8_t>> data)
    {
        packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        properties.deserialize(data);
        auto data_span = data.cspan();
        reason_codes.resize(data.size());
        std::copy(data_span.begin(), data_span.end(), reason_codes.begin());
    }

    template <class Stream>
    void deserialize(header hdr, transport::data_fetcher<Stream> data) {
        auto data_span = data.cspan(hdr.remaining_length());
        auto my_data = transport::buffer_data_fetcher(data_span);
        deserialize(my_data);
    }

    template<class Writer>
    void serialize_body(Writer&& writer) const {
        fixed_int<std::uint16_t>::serialize(packet_identifier, writer);
        properties.serialize(writer);
        for(const auto& code: reason_codes) {
            writer(code);
        }
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        header hdr(type_value, 0, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};

}