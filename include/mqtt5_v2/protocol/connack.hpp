
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/properties.hpp>
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>
#include <mqtt5_v2/protocol/header.hpp>

#include <p0443_v2/sequence.hpp>

namespace mqtt5_v2::protocol
{
struct connack
{
    static constexpr std::uint8_t type_value = 2;

    fixed_int<std::uint8_t> flags;
    fixed_int<std::uint8_t> reason_code;
    protocol::properties properties;

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        return p0443_v2::sequence(protocol::inplace_deserializer(flags, data_fetcher),
                                  protocol::inplace_deserializer(reason_code, data_fetcher),
                                  protocol::inplace_deserializer(properties, data_fetcher)
                                  );
    }



    template <class Writer>
    void serialize_body(Writer &&writer) const {
        flags.serialize(writer);
        reason_code.serialize(writer);
        properties.serialize(writer);
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        header hdr(type_value, 0, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }

    /*operator control_packet() const {
        control_packet retval;
        retval.fixed_sized_header.set_type(2);
        retval.fixed_sized_header.set_flags(0);
        retval.set_payload_from_packet(*this);
        return retval;
    }*/
};
}