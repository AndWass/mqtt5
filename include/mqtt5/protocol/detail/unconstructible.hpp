
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace mqtt5::protocol::detail
{
class unconstructible
{
private:
    unconstructible() = delete;
    unconstructible(const unconstructible&) = delete;
    unconstructible(unconstructible&&) = delete;
};
}