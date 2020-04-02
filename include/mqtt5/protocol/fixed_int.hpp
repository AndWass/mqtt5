
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5/protocol/deserialize.hpp>
#include <mqtt5/transport/data_fetcher.hpp>
#include "detail/unconstructible.hpp"

namespace mqtt5
{
namespace protocol
{
template<class T>
struct fixed_int
{
    template<class Stream>
    [[nodiscard]] static T deserialize(transport::data_fetcher<Stream> data) {
        std::make_unsigned_t<T> value = 0;
        auto my_data = data.cspan(sizeof(T));

        for (std::size_t i = 0; i < sizeof(T); i++) {
            if constexpr(sizeof(T) > 1) {
                value <<= 8;
            }
            value += my_data[i];
        }
        data.consume(sizeof(T));

        return value;
    }

    template<class Writer>
    static void serialize(T value, Writer&& writer) {
        std::uint32_t shift_amount = 8*(sizeof(value)-1);
        std::make_unsigned_t<T> ui_value = value;
        for(std::size_t i=0; i<sizeof(T); i++) {
            writer((ui_value >> shift_amount) & 0x00FF);
            shift_amount -= 8;
        }
    }
private:
    fixed_int() = delete;
    ~fixed_int() {}
    fixed_int(const fixed_int&) = delete;
};
}
}
