
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "header.hpp"
#include "properties.hpp"

#include <chrono>
#include <mqtt5/disconnect_reason.hpp>

namespace mqtt5::protocol
{
struct disconnect
{
    struct properties_t : properties_t_base
    {
        std::chrono::duration<std::uint32_t> session_expiry_interval{0};
        std::string reason_string;
        std::string server_reference;

        template <class Stream>
        [[nodiscard]] static properties_t deserialize(transport::data_fetcher<Stream> stream) {
            protocol::properties props;
            props.deserialize(stream);
            properties_t retval;
            for (auto &p : props) {
                if (p.identifier == property_ids::session_expiry_interval) {
                    retval.session_expiry_interval =
                        decltype(retval.session_expiry_interval){p.value_as<std::uint32_t>()};
                }
                else if (p.identifier == property_ids::reason_string) {
                    retval.reason_string = p.value_as<std::string>();
                }
                else if (p.identifier == property_ids::server_reference) {
                    retval.server_reference = p.value_as<std::string>();
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
            if (session_expiry_interval > std::chrono::seconds{0}) {
                props.add_property(property_ids::session_expiry_interval,
                                   session_expiry_interval.count());
            }
            if (!reason_string.empty()) {
                props.add_property(property_ids::reason_string, reason_string);
            }
            if (!server_reference.empty()) {
                props.add_property(property_ids::server_reference, server_reference);
            }
            this->add_base_properties(props);
            props.serialize(writer);
        }

        friend bool operator==(const properties_t &lhs, const properties_t &rhs) {
            return lhs.reason_string == rhs.reason_string &&
                   lhs.server_reference == rhs.server_reference &&
                   lhs.session_expiry_interval == rhs.session_expiry_interval &&
                   lhs.user_property == rhs.user_property &&
                   lhs.unknown_properties == rhs.unknown_properties;
        }
    };

    static constexpr std::uint8_t type_value = 14;

    mqtt5::disconnect_reason reason = mqtt5::disconnect_reason::normal;
    properties_t properties;

    disconnect() = default;

    disconnect(mqtt5::disconnect_reason reason) : reason(reason) {
    }

    template <class DataFetcher>
    disconnect(std::in_place_t, header hdr, DataFetcher fetcher) {
        deserialize(hdr, fetcher);
    }

    void deserialize(transport::buffer_data_fetcher_t<nonstd::span<const std::uint8_t>> data) {
        if (data.empty()) {
            reason = mqtt5::disconnect_reason::normal;
            properties = properties_t{};
        }
        else {
            reason = static_cast<mqtt5::disconnect_reason>(
                protocol::fixed_int<std::uint8_t>::deserialize(data));
            if (data.size() > 1) {
                properties = properties_t::deserialize(data);
            }
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
        if (reason == mqtt5::disconnect_reason::normal) {
            if (properties == properties_t{}) {
                // No need to perform any more serialization
                return;
            }
        }

        protocol::fixed_int<std::uint8_t>::serialize(static_cast<std::uint8_t>(reason), writer);
        properties.serialize(writer);
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, 0, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};
} // namespace mqtt5::protocol