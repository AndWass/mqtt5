
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <nonstd/span.hpp>
#include <cstdint>

namespace mqtt5::protocol
{
struct adl_tag {};

inline nonstd::span<const std::uint8_t> take_data(nonstd::span<const std::uint8_t> data, std::size_t size) {
    if(static_cast<std::size_t>(data.size()) < size) {
        throw std::runtime_error("Not enough data");
    }
    return data.subspan(0, size);
}
}