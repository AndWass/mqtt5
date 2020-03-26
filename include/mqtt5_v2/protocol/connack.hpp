
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

    std::uint8_t flags;
    std::uint8_t reason_code;
    protocol::properties properties;

    template <class Stream>
    void deserialize(transport::data_fetcher<Stream> fetcher) {
        flags = fixed_int<std::uint8_t>::deserialize(fetcher);
        reason_code = fixed_int<std::uint8_t>::deserialize(fetcher);
        properties.deserialize(fetcher);
    }



    template <class Writer>
    void serialize_body(Writer &&writer) const {
        fixed_int<std::uint8_t>::serialize(flags, writer);
        fixed_int<std::uint8_t>::serialize(reason_code, writer);
        properties.serialize(writer);
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        header hdr(type_value, 0, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};
}