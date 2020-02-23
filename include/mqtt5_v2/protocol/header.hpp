
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/varlen_int.hpp>

#include <p0443_v2/sequence.hpp>

namespace mqtt5_v2::protocol
{
struct header
{
    std::uint8_t type() {
        return type_flags_.value >> 4;
    }
    std::uint8_t flags() {
        return type_flags_.value & 0x0F;
    }
    std::uint32_t remaining_length() {
        return remaining_length_.value;
    }

    template<class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        return p0443_v2::sequence(type_flags_.inplace_deserializer(data_fetcher), remaining_length_.inplace_deserializer(data_fetcher));
    }

    nonstd::span<const std::uint8_t> set_from_bytes(nonstd::span<const std::uint8_t> data) {
        data = type_flags_.set_from_bytes(data);
        return remaining_length_.set_from_bytes(data);
    }
private:
    fixed_int<std::uint8_t> type_flags_;
    varlen_int remaining_length_;
};
}