
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "mqtt5_v2/protocol/fixed_int.hpp"
#include "mqtt5_v2/protocol/varlen_int.hpp"
#include <mqtt5_v2/transport/data_fetcher.hpp>
#include <mqtt5_v2/protocol/header.hpp>
#include <mqtt5_v2/protocol/string.hpp>
#include <mqtt5_v2/protocol/properties.hpp>

#include <optional>
#include <variant>

namespace mqtt5_v2::protocol
{
using std::begin;
using std::end;
class publish
{
    std::uint8_t header_flags = 0;
public:
    std::string topic;
    std::uint16_t packet_identifier = 0;
    properties properties;
    std::vector<std::uint8_t> payload;

    static constexpr std::uint8_t type_value = 3;

    template<class InputIt>
    void set_payload(InputIt begin, InputIt end)
    {
        payload.clear();
        std::copy(begin, end, std::back_inserter(payload));
    }

    template<class T, class = decltype(begin(std::declval<T>()))>
    void set_payload(const T& t) {
        set_payload(begin(t), end(t));
    }

    publish() = default;
    template<class T>
    publish(std::in_place_t, header hdr, T fetcher) {
        deserialize(hdr, fetcher);
    }

    void set_duplicate(bool dup) {
        header_flags = (header_flags & 0x07) + ((dup ? 1:0) << 3);
    }

    bool duplicate_flag() const {
        return header_flags & 0x08;
    }

    void set_quality_of_service(std::uint8_t qos) {
        qos &= 0x03;
        if(qos == 0) {
            set_duplicate(false);
        }
        header_flags = (header_flags & 0x09) + (qos << 1);
    }

    std::uint8_t quality_of_service() const {
        return ((header_flags >> 1) & 0x03);
    }

    void set_retain(bool retain) {
        header_flags = (header_flags & 14) + (retain ? 1:0);
    }

    bool retain_flag() const {
        return header_flags & 0x01;
    }

    void deserialize(transport::span_byte_data_fetcher_t data)
    {
        topic = string::deserialize(data);
        if(quality_of_service() > 0) {
            packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        }
        else {
            packet_identifier = 0;
        }
        properties.deserialize(data);
        auto rest = data.cspan();
        payload.resize(rest.size());
        std::copy(rest.begin(), rest.end(), payload.begin());
    }

    template <class Stream>
    void deserialize(header hdr, transport::data_fetcher<Stream> fetcher)
    {
        header_flags = hdr.flags();
        auto span = fetcher.cspan(hdr.remaining_length());
        deserialize(transport::span_byte_data_fetcher_t{span});
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        string::serialize(topic, writer);
        if(packet_identifier) {
            fixed_int<std::uint16_t>::serialize(packet_identifier, writer);
        }
        properties.serialize(writer);

        for(auto b: payload) {
            writer(b);
        }
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, header_flags, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};

class puback
{
public:
    std::uint16_t packet_identifier;
    std::uint8_t  reason_code;
    properties properties;

    static constexpr std::uint8_t type_value = 4;

    static constexpr std::uint8_t success=0,
        no_matching_subscribers=16,
        unspecified=128,
        implementation_specific_error=131,
        not_authorized=135,
        topic_name_invalid=144,
        packet_identifier_in_use=145,
        quota_exceeded=151,
        payload_format_invalid=153;

    puback() = default;
    template<class T>
    puback(std::in_place_t, std::uint32_t remaining_length, T fetcher) {
        deserialize(remaining_length, fetcher);
    }

    template<class Stream>
    void deserialize(std::uint32_t remaining_length, transport::data_fetcher<Stream> data) {
        packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        if(remaining_length > 2) {
            reason_code = fixed_int<std::uint8_t>::deserialize(data);
        }
        else {
            reason_code = 0;
        }

        if(remaining_length >= 4)
        {
            properties.deserialize(data);
        }
        else {
            properties.clear();
        }
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        fixed_int<std::uint16_t>::serialize(packet_identifier, writer);
        if(reason_code || !properties.empty()) {
            fixed_int<std::uint8_t>::serialize(reason_code, writer);
            if(!properties.empty()) {
                properties.serialize(writer);
            }
        }
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, 0, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};
}
