
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <nonstd/span.hpp>

#include <mqtt5_v2/transport/data_fetcher.hpp>
#include <mqtt5_v2/transport/data_writer.hpp>
#include <mqtt5_v2/protocol/error.hpp>

#include <p0443_v2/transform.hpp>

namespace mqtt5_v2
{
namespace protocol
{
template<class T>
struct fixed_int
{
    T value = 0;

    fixed_int() noexcept = default;
    fixed_int(T v): value(v) {}

    operator T() const noexcept {
        return value;
    }

    template<class U, std::enable_if_t<std::is_integral_v<U>>* = nullptr>
    operator U() const noexcept {
        return static_cast<U>(value);
    }

    template<class U, std::enable_if_t<std::is_integral_v<U>>* = nullptr>
    friend bool operator==(const fixed_int& lhs, const U& rhs) {
        return lhs.value == rhs;
    }

    template<class U, std::enable_if_t<std::is_integral_v<U>>* = nullptr>
    friend bool operator==(const U& lhs, const fixed_int& rhs) {
        return lhs == rhs.value;
    }

    template<class U>
    friend bool operator==(const fixed_int<T>& lhs, const fixed_int<U>& rhs) {
        return lhs.value == rhs.value;
    }

    template<class U, std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    fixed_int& operator=(U val) {
        value = static_cast<T>(val);
        return *this;
    }

    nonstd::span<const std::uint8_t> set_from_bytes(nonstd::span<const std::uint8_t> data) {
        if(data.size() < sizeof(value)) {
            throw protocol_error("not enough bytes to convert to fixed_int");
        }
        set_from_data(data);
        return data.subspan(sizeof(T));
    }

    void set_from_data(nonstd::span<const std::uint8_t> data) {
        value = 0;
        for (std::size_t i = 0; i < sizeof(T); i++) {
            if constexpr(sizeof(T) > 1) {
                value <<= 8;
            }
            value += data[i];
        }
    }

    template<class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        return p0443_v2::transform(data_fetcher.get_data(sizeof(T)), [this](transport::data_fetcher<Stream> data) {
            this->set_from_data(data.cspan());
            data.buffer->consume(sizeof(T));
            return data;
        });
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        std::uint32_t shift_amount = 8*(sizeof(value)-1);
        for(std::size_t i=0; i<sizeof(T); i++) {
            writer((value >> shift_amount) & 0x00FF);
            shift_amount -= 8;
        }
    }
};
}
}