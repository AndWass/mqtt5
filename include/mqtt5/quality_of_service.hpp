
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

namespace mqtt5
{
enum class quality_of_service: std::uint8_t
{
    qos0,
    qos1,
    qos2
};

constexpr quality_of_service operator ""_qos(unsigned long long n) {
    return static_cast<quality_of_service>(n);
}

namespace literals
{
using mqtt5::operator""_qos;
}
}