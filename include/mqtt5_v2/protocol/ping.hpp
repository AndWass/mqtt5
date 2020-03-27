
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

namespace mqtt5_v2::protocol
{
struct pingreq
{
    constexpr static std::uint8_t type_value=12;

    template<class Writer>
    void serialize(Writer&& writer) const {
        writer(type_value << 4);
        writer(0);
    } 
};

struct pingresp
{
    constexpr static std::uint8_t type_value=13;

    template<class Writer>
    void serialize(Writer&& writer) const {
        writer(type_value << 4);
        writer(0);
    } 
};
}