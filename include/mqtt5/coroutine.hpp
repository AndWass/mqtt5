
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <p0443_v2/await_sender.hpp>

namespace mqtt5
{
template<class T>
auto operator co_await(T&& t)  {
    return p0443_v2::awaitable_for(std::forward<T>(t));
}
}