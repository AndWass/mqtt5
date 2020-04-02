
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <stdexcept>

namespace mqtt5::protocol
{
class protocol_error: public std::runtime_error
{
public:
    explicit protocol_error(const char* error): std::runtime_error(error) {}
};
}