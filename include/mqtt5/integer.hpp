#pragma once

#include <cstdint>
#include <exception>
#include <nonstd/span.hpp>
#include <type_traits>
#include <limits>

#include "serialize.hpp"

namespace mqtt5
{
struct variable_length_tag
{
};

template <class BackingType>
class integer
{
public:
    using value_type = BackingType;
    static constexpr BackingType max_value = std::numeric_limits<BackingType>::max();
    constexpr integer() noexcept = default;
    constexpr explicit integer(value_type value) noexcept : value_(value) {
    }
    constexpr explicit integer(nonstd::span<const std::uint8_t> data) {
        value_ = 0;
        if (static_cast<std::size_t>(data.size()) < sizeof(value_type)) {
            throw std::length_error("not enought data to deserialize");
        }
        for (std::size_t i = 0; i < sizeof(value_type); i++) {
            value_ <<= 8;
            value_ += data[i];
        }
    }

    template <class Integer>
    constexpr integer(integer<Integer> value) noexcept : value_(value.value()) {
    }

    constexpr BackingType value() const noexcept {
        return value_;
    }

    integer operator=(value_type value) {
        value_ = value;
        return *this;
    }

private:
    static_assert(std::disjunction_v<std::is_same<BackingType, std::uint16_t>,
                                     std::is_same<BackingType, std::uint32_t>>);
    value_type value_ = 0;
};

template <>
class integer<variable_length_tag>
{
public:
    using value_type = std::uint32_t;
    static constexpr std::uint32_t max_value = 268'435'455;
    constexpr integer() noexcept = default;
    constexpr explicit integer(value_type value) noexcept : value_(value) {
    }
    constexpr explicit integer(
        nonstd::span<const std::uint8_t> data,
        std::size_t* bytes_used = nullptr) {
        if(data.empty()) {
            throw std::length_error("not enough data to deserialize a varlen_integer");
        }

        value_ = 0;
        value_type multiplier = 1;
        value_type encoded_byte = 0;
        auto iter = data.begin();
        std::size_t byte_counter = 0;
        do {
            encoded_byte = *iter++;
            byte_counter++;
            value_ += (encoded_byte & 0x7f) * multiplier;
            if (multiplier > 128 * 128 * 128) {
                throw std::invalid_argument("Buffer contains bad variable integer data");
            }
            else if ((encoded_byte & 128) && iter == data.end()) {
                throw std::length_error("not enough data to deserialize a varlen_integer");
            }
            multiplier *= 128;
        } while (encoded_byte & 128);
        if(bytes_used) {
            *bytes_used = byte_counter;
        }
    }

    template <class Integer>
    constexpr integer(integer<Integer> value) noexcept : value_(value.value()) {
    }

    constexpr value_type value() const noexcept {
        return value_;
    }

    integer operator=(value_type value) {
        value_ = value;
        return *this;
    }

private:
    value_type value_ = 0;
};

#define MAKE_OPERATOR(X)                                                                           \
    template <class Backend, class Integer,                                                        \
              std::enable_if_t<std::is_integral_v<Integer>> * = nullptr>                           \
    bool operator X(integer<Backend> lhs, Integer rhs) {                                           \
        return lhs.value() X rhs;                                                                  \
    }                                                                                              \
    template <class Backend, class Integer,                                                        \
              std::enable_if_t<std::is_integral_v<Integer>> * = nullptr>                           \
    bool operator X(Integer lhs, integer<Backend> rhs) {                                           \
        return lhs X rhs.value();                                                                  \
    }                                                                                              \
    template <class B1, class B2>                                                                  \
    bool operator X(integer<B1> lhs, integer<B2> rhs) {                                            \
        return lhs.value() X rhs.value();                                                          \
    }

MAKE_OPERATOR(==)
MAKE_OPERATOR(!=)
MAKE_OPERATOR(<)
MAKE_OPERATOR(<=)
MAKE_OPERATOR(>)
MAKE_OPERATOR(>=)

#undef MAKE_OPERATOR

template <class BackType, class Iter,
          std::enable_if_t<!std::is_same_v<BackType, variable_length_tag>> * = nullptr>
[[nodiscard]] Iter serialize(integer<BackType> value, Iter out) {
    constexpr auto type_size = sizeof(BackType);
    auto shift_amount = 8 * (type_size - 1);
    for (std::size_t i = 0; i < type_size; i++) {
        *out = static_cast<std::uint8_t>(value.value() >> shift_amount) & 0x00FF;
        ++out;
        shift_amount -= 8;
    }
    return out;
}

template <class BackType, class Iter>
[[nodiscard]] Iter deserialize_into(integer<BackType> &value, Iter begin, Iter end) {
    detail::throw_if_empty(begin, end);
    nonstd::span<const std::uint8_t> data(&(*begin), end - begin);
    ::new (&value) integer<BackType>(data);
    return begin + sizeof(BackType);
}

template <class Iter>
[[nodiscard]] Iter serialize(integer<variable_length_tag> value, Iter out) {
    auto val = value.value();
    do {
        std::uint8_t byte = val % 128;
        val /= 128;
        if (val > 0) {
            byte |= 0x80;
        }
        *out = byte;
        ++out;
    } while (val > 0);
    return out;
}

template <class Iter>
[[nodiscard]] Iter deserialize_into(integer<variable_length_tag> &value, Iter begin, Iter end) {
    detail::throw_if_empty(begin, end);
    std::size_t bytes_used;
    nonstd::span<const std::uint8_t> data(&(*begin), end - begin);
    value = integer<variable_length_tag>(data, &bytes_used);
    return begin + bytes_used;
}

using integer16 = integer<std::uint16_t>;
using integer32 = integer<std::uint32_t>;
using varlen_integer = integer<variable_length_tag>;

} // namespace mqtt5