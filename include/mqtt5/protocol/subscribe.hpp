
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5/transport/data_fetcher.hpp>
#include <mqtt5/protocol/fixed_int.hpp>
#include <mqtt5/protocol/inplace_deserializer.hpp>
#include <mqtt5/protocol/properties.hpp>
#include <mqtt5/protocol/string.hpp>
#include <mqtt5/protocol/header.hpp>

#include <p0443_v2/sequence.hpp>
#include <p0443_v2/sync_wait.hpp>

#include <stdexcept>
#include <variant>
#include <vector>

namespace mqtt5::protocol
{
class subscribe
{
public:
    struct topic_filter
    {
        topic_filter() = default;
        template<class T, class U>
        topic_filter(T&& t, U&& u): topic(std::forward<T>(t)), options(std::forward<U>(u)) {}

        std::string topic;
        std::uint8_t options;
    };

    struct properties_t : properties_t_base
    {
        varlen_int::type subscription_identifier{ 0 };

        template<class Stream>
        [[nodiscard]] static properties_t deserialize(transport::data_fetcher<Stream> data)
        {
            properties_t retval;
            protocol::properties props;
            props.deserialize(data);
            for (auto& prop : props) {
                if (prop.identifier == property_ids::subscription_identifier) {
                    retval.subscription_identifier = prop.value_as<varlen_int::type>();
                }
                else {
                    retval.handle_property(prop);
                }
            }
            return retval;
        }

        template<class Writer>
        void serialize(Writer&& writer) const
        {
            protocol::properties props;
            if (subscription_identifier != 0) {
                props.add_property(property_ids::subscription_identifier, subscription_identifier);
            }
            add_base_properties(props);
            props.serialize(writer);
        }
    };

    std::uint16_t packet_identifier = 0;
    properties_t properties;
    std::vector<topic_filter> topics;

    static constexpr std::uint8_t type_value = 8;

    subscribe() = default;
    template<class T>
    subscribe(std::in_place_t, header hdr, T fetcher)
    {
        deserialize(hdr, fetcher);
    }

    void deserialize(transport::buffer_data_fetcher_t<nonstd::span<const std::uint8_t>> data)
    {
        packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        properties = properties_t::deserialize(data);
        while(!data.empty())
        {
            topic_filter topic;
            topic.topic = string::deserialize(data);
            topic.options = fixed_int<std::uint8_t>::deserialize(data);
            topics.emplace_back(std::move(topic));
        }
    }

    template <class Stream>
    void deserialize(header hdr, transport::data_fetcher<Stream> data) {
        auto data_span = data.cspan(hdr.remaining_length());
        auto my_data = transport::buffer_data_fetcher(data_span);
        deserialize(my_data);
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        fixed_int<std::uint16_t>::serialize(packet_identifier, writer);
        properties.serialize(writer);
        for(auto &f: topics)
        {
            string::serialize(f.topic, writer);
            fixed_int<std::uint8_t>::serialize(f.options, writer);
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
    std::uint16_t packet_identifier;
    properties_t properties;
    std::vector<std::uint8_t> reason_codes;
public:
    static constexpr std::uint8_t type_value = 9;

    suback() = default;
    
    template<class DataFetcher>
    suback(std::in_place_t, header hdr, DataFetcher fetcher) {
        deserialize(hdr, fetcher);
    }

    void deserialize(transport::buffer_data_fetcher_t<nonstd::span<const std::uint8_t>> data)
    {
        packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        properties = properties_t::deserialize(data);
        auto rest = data.cspan();
        reason_codes.clear();

        std::copy(rest.begin(), rest.end(), std::back_inserter(reason_codes));
    }

    template <class Stream>
    auto deserialize(header hdr, transport::data_fetcher<Stream> data) {
        auto data_span = data.cspan(hdr.remaining_length());
        auto my_data = transport::buffer_data_fetcher(data_span);
        deserialize(my_data);
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        fixed_int<std::uint16_t>::serialize(packet_identifier, writer);
        properties.serialize(writer);
        for(auto b: reason_codes) {
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
} // namespace mqtt5::protocol