
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <vector>

template<class T>
std::vector<std::uint8_t> vector_serialize(const T&t) {
    std::vector<std::uint8_t> retval;
    t.serialize([&](std::uint8_t b) {
        retval.push_back(b);
    });
    return retval;
}