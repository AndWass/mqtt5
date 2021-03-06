
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "mqtt5/protocol/fixed_int.hpp"
#include "mqtt5/protocol/varlen_int.hpp"
#include <mqtt5/protocol/header.hpp>
#include <mqtt5/protocol/properties.hpp>
#include <mqtt5/protocol/string.hpp>
#include <mqtt5/transport/data_fetcher.hpp>

#include <mqtt5/payload_format_indicator.hpp>
#include <mqtt5/puback_reason_code.hpp>
#include <mqtt5/quality_of_service.hpp>

#include <chrono>
#include <functional>
#include <iterator>
#include <optional>
#include <variant>

namespace mqtt5::protocol
{
using std::begin;
using std::end;
class publish
{
public:
    struct properties_t : properties_t_base
    {
        std::string response_topic;
        std::string content_type;

        protocol::binary::type correlation_data;

        std::uint32_t subscription_identifier = 0;
        std::chrono::duration<std::uint32_t> message_expiry_interval{0};

        std::uint16_t topic_alias{0};
        mqtt5::payload_format_indicator payload_format_indicator{
            mqtt5::payload_format_indicator::unspecified};

        template <class Stream>
        [[nodiscard]] static properties_t deserialize(transport::data_fetcher<Stream> data) {
            properties_t retval;
            protocol::properties props;
            props.deserialize(data);
            using ids = property_ids;
            auto set_value = [&](auto &value, const protocol::property &prop) {
                value = prop.value_as<std::remove_reference_t<decltype(value)>>();
            };
            for (auto &prop : props) {
                if (prop.identifier == ids::response_topic) {
                    set_value(retval.response_topic, prop);
                }
                else if (prop.identifier == ids::content_type) {
                    set_value(retval.content_type, prop);
                }
                else if (prop.identifier == ids::correlation_data) {
                    set_value(retval.correlation_data, prop);
                }
                else if (prop.identifier == ids::subscription_identifier) {
                    retval.subscription_identifier = prop.value_as<std::uint32_t>();
                }
                else if (prop.identifier == ids::message_expiry_interval) {
                    retval.message_expiry_interval =
                        decltype(retval.message_expiry_interval){prop.value_as<std::uint16_t>()};
                }
                else if (prop.identifier == ids::topic_alias) {
                    set_value(retval.topic_alias, prop);
                }
                else if (prop.identifier == ids::payload_format_indicator) {
                    retval.payload_format_indicator =
                        static_cast<mqtt5::payload_format_indicator>(prop.value_as<std::uint8_t>());
                }
                else {
                    retval.handle_property(prop);
                }
            }
            return retval;
        }

        template <class Writer>
        void serialize(Writer &&writer) const {
            protocol::properties props;
            properties_t dflt;
            auto maybe_add = [&, this](auto Ptr, std::uint8_t prop) {
                auto &val = (*this).*Ptr;
                if (val != dflt.*Ptr) {
                    props.add_property(prop, val);
                }
            };
            using ids = property_ids;
            maybe_add(&properties_t::response_topic, ids::response_topic);
            maybe_add(&properties_t::content_type, ids::content_type);
            maybe_add(&properties_t::correlation_data, ids::correlation_data);
            for (auto &kv : user_property) {
                props.add_property(ids::user_property, kv);
            }
            maybe_add(&properties_t::subscription_identifier, ids::subscription_identifier);
            if (message_expiry_interval != dflt.message_expiry_interval) {
                props.add_property(ids::message_expiry_interval, message_expiry_interval.count());
            }
            maybe_add(&properties_t::topic_alias, ids::topic_alias);
            if(payload_format_indicator != dflt.payload_format_indicator) {
                props.add_property(ids::payload_format_indicator, static_cast<std::uint8_t>(payload_format_indicator));
            }
            this->add_base_properties(props);

            props.serialize(writer);
        }
    };

private:
    std::uint8_t header_flags = 0;

public:
    std::string topic;
    std::uint16_t packet_identifier = 0;
    properties_t properties;
    std::vector<std::uint8_t> payload;

    static constexpr std::uint8_t type_value = 3;

    template <class InputIt>
    void set_payload(InputIt begin, InputIt end) {
        payload.clear();
        std::copy(begin, end, std::back_inserter(payload));
    }

    template <class T, class = decltype(begin(std::declval<const T &>()))>
    void set_payload(const T &t) {
        set_payload(begin(t), end(t));
    }

    publish() = default;
    template <class T>
    publish(std::in_place_t, header hdr, T fetcher) {
        deserialize(hdr, fetcher);
    }

    void set_duplicate(bool dup) {
        header_flags = (header_flags & 0x07) + ((dup ? 1 : 0) << 3);
    }

    bool duplicate_flag() const {
        return header_flags & 0x08;
    }

    void set_quality_of_service(mqtt5::quality_of_service qos) {
        if (qos == quality_of_service::qos0) {
            set_duplicate(false);
        }
        header_flags = (header_flags & 0x09) + (static_cast<std::uint8_t>(qos) << 1);
    }

    quality_of_service quality_of_service() const {
        return static_cast<mqtt5::quality_of_service>((header_flags >> 1) & 0x03);
    }

    void set_retain(bool retain) {
        header_flags = (header_flags & 14) + (retain ? 1 : 0);
    }

    bool retain_flag() const {
        return header_flags & 0x01;
    }

    void deserialize(transport::span_byte_data_fetcher_t data) {
        topic = string::deserialize(data);
        if (quality_of_service() != mqtt5::quality_of_service::qos0) {
            packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        }
        else {
            packet_identifier = 0;
        }
        properties = properties_t::deserialize(data);
        auto rest = data.cspan();
        payload.resize(rest.size());
        std::copy(rest.begin(), rest.end(), payload.begin());
    }

    template <class Stream>
    void deserialize(header hdr, transport::data_fetcher<Stream> fetcher) {
        header_flags = hdr.flags();
        auto span = fetcher.cspan(hdr.remaining_length());
        deserialize(transport::span_byte_data_fetcher_t{span});
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        string::serialize(topic, writer);
        if (packet_identifier) {
            fixed_int<std::uint16_t>::serialize(packet_identifier, writer);
        }
        properties.serialize(writer);

        for (auto b : payload) {
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
namespace detail
{
template<class CodeT, std::uint8_t TypeValue, std::uint8_t Flags = 0>
struct puback_base
{
    using code_type = CodeT;
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
    code_type reason_code = code_type::success;
    properties_t properties;

    static constexpr std::uint8_t type_value = TypeValue;

    puback_base() = default;
    template <class T>
    puback_base(std::in_place_t, header hdr, T fetcher) {
        deserialize(hdr.remaining_length(), fetcher);
    }

    template <class Stream>
    void deserialize(std::uint32_t remaining_length, transport::data_fetcher<Stream> data) {
        packet_identifier = fixed_int<std::uint16_t>::deserialize(data);
        if (remaining_length > 2) {
            reason_code =
                static_cast<code_type>(fixed_int<std::uint8_t>::deserialize(data));
        }
        else {
            reason_code = code_type::success;
        }

        if (remaining_length >= 4) {
            properties = properties_t::deserialize(data);
        }
        else {
            properties = properties_t{};
        }
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        fixed_int<std::uint16_t>::serialize(packet_identifier, writer);
        bool serialize_properties =
            !properties.user_property.empty() || !properties.reason_string.empty();
        if (reason_code != code_type::success || serialize_properties) {
            fixed_int<std::uint8_t>::serialize(static_cast<std::uint8_t>(reason_code), writer);
            properties.serialize(writer);
        }
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, Flags, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};
}
struct puback: detail::puback_base<mqtt5::puback_reason_code, 4>
{
    using detail::puback_base<mqtt5::puback_reason_code, 4>::puback_base;
};

struct pubrec: detail::puback_base<mqtt5::pubrec_reason_code, 5>
{
    using detail::puback_base<mqtt5::puback_reason_code, 5>::puback_base;
};

struct pubrel: detail::puback_base<pubrel_reason_code, 6, 2>
{
    using detail::puback_base<pubrel_reason_code, 6, 2>::puback_base;
};

struct pubcomp: detail::puback_base<pubcomp_reason_code, 7>
{
    using detail::puback_base<pubcomp_reason_code, 7>::puback_base;
};
} // namespace mqtt5::protocol
