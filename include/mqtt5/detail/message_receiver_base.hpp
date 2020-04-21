
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <exception>

namespace mqtt5::detail
{
template<class...Values>
struct message_receiver_base
{
    virtual ~message_receiver_base() = default;

    virtual void set_value(Values...) = 0;
    virtual void set_done() = 0;
    virtual void set_error(std::exception_ptr) = 0;
};
}