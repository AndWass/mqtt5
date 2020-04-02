
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <mqtt5/protocol/properties.hpp>
#include <mqtt5/protocol/fixed_int.hpp>
#include <mqtt5/protocol/string.hpp>
#include <mqtt5/protocol/header.hpp>
#include <mqtt5/transport/data_fetcher.hpp>

namespace mqtt5::protocol
{
struct unsubscribe
{
    struct properties_t: properties_t_base
    {
        template<class Stream>
        [[nodiscard]] static properties_t deserialize(transport::data_fetcher<Stream> data)
        {
            properties_t retval;
            protocol::properties props;
            props.deserialize(data);
            for (auto& prop: props) {
                retval.handle_property(prop);
            }
            return retval;
        }

        template<class Writer>
        void serialize(Writer&& writer) const
        {
            protocol::properties props;
            this->add_base_properties(props);
            props.serialize(writer);
        }
    };
    static constexpr std::uint8_t type_value = 10;

    std::uint16_t packet_identifier;
    properties_t properties;
    std::vector<std::string> topics;

    unsubscribe() = default;
    template<class DataFetcher>
    unsubscribe(std::in_place_t, header hdr, DataFetcher fetcher) {
        deserialize(hdr, fetcher);
    }

    void deserialize(transport::buffer_data_fetcher_t<nonstd::span<const std::uint8_t>> data)
    {
        packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        properties = properties_t::deserialize(data);
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
    struct properties_t : properties_t_base
    {
        std::string reason_string;

        template <class Stream>
        [[nodiscard]] static properties_t deserialize(transport::data_fetcher<Stream> stream) {
            protocol::properties props;
            props.deserialize(stream);
            properties_t retval;
            for (auto &p : props) {
                if (p.identifier == property_ids::reason_string) {
                    retval.reason_string = p.value_as<std::string>();
                }
                else {
                    retval.handle_property(p);
                }
            }
            return retval;
        }

        template <class Writer>
        void serialize(Writer &&writer) const {
            protocol::properties props;
            if (!reason_string.empty()) {
                props.add_property(property_ids::reason_string, reason_string);
            }
            this->add_base_properties(props);
            props.serialize(writer);
        }
    };

    static constexpr std::uint8_t type_value = 11;

    std::uint16_t packet_identifier;
    properties_t properties;
    std::vector<std::uint8_t> reason_codes;

    unsuback() = default;
    template<class DataFetcher>
    unsuback(std::in_place_t, header hdr, DataFetcher fetcher) {
        deserialize(hdr, fetcher);
    }

    void deserialize(transport::buffer_data_fetcher_t<nonstd::span<const std::uint8_t>> data)
    {
        packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        properties = properties_t::deserialize(data);
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