#pragma once

#include <array>
#include <cstdint>
#include <exception>
#include <limits>
#include <nonstd/span.hpp>
#include <type_traits>

#include <boost/asio/read.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include "mqtt5/type/serialize.hpp"

namespace mqtt5
{
namespace type
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
            boost::throw_exception(boost::system::system_error(
                boost::system::errc::make_error_code(boost::system::errc::protocol_error)));
        }
        for (std::size_t i = 0; i < sizeof(value_type); i++) {
            if constexpr(sizeof(value_type) > 1) {
                value_ <<= 8;
            }
            value_ += data[i];
        }
    }

    template <class Iter>
    integer(Iter begin, Iter end)
        : integer(nonstd::span<const std::uint8_t>(&(*begin), end - begin)) {
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
                                     std::is_same<BackingType, std::uint32_t>,
                                     std::is_same<BackingType, std::uint8_t>>);
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
    constexpr explicit integer(nonstd::span<const std::uint8_t> data,
                               std::size_t *bytes_used = nullptr) {
        if (data.empty()) {
            boost::throw_exception(boost::system::system_error(
                boost::system::errc::make_error_code(boost::system::errc::protocol_error)));
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
                boost::throw_exception(boost::system::system_error(
                    boost::system::errc::make_error_code(boost::system::errc::protocol_error)));
            }
            else if ((encoded_byte & 128) && iter == data.end()) {
                boost::throw_exception(boost::system::system_error(
                    boost::system::errc::make_error_code(boost::system::errc::protocol_error)));
            }
            multiplier *= 128;
        } while (encoded_byte & 128);
        if (bytes_used) {
            *bytes_used = byte_counter;
        }
    }

    template <class Iter>
    integer(Iter begin, Iter end, std::size_t *bytes_used = nullptr)
        : integer(nonstd::span<const std::uint8_t>(&(*begin), end - begin), bytes_used) {
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

using integer8 = integer<std::uint8_t>;
using integer16 = integer<std::uint16_t>;
using integer32 = integer<std::uint32_t>;
using varlen_integer = integer<variable_length_tag>;

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

namespace detail {
    template <typename Integer>
    struct integer_read_from_stream_impl
    {
        template <typename Stream, typename Receiver>
        static void read_from_stream(Stream &stream, Receiver &&rx) {
            struct read_state
            {
                using receiver_type = std::remove_reference_t<Receiver>;
                std::array<std::uint8_t, sizeof(typename Integer::value_type)> storage;
                receiver_type rx;
                read_state(Receiver rx) : rx(std::forward<Receiver>(rx)) {
                }
            };
            auto state = std::make_shared<read_state>(std::forward<Receiver>(rx));
            boost::asio::async_read(stream, boost::asio::buffer(state->storage),
                                    [state](const auto &ec, const auto sz) {
                                        if (ec) {
                                            state->rx.set_error(ec);
                                        }
                                        else {
                                            try {
                                                Integer value(state->storage.begin(),
                                                              state->storage.end());
                                                state->rx.set_value(value);
                                            }
                                            catch (boost::system::system_error &ec2) {
                                                state->rx.set_error(ec2.code());
                                            }
                                        }
                                    });
        }
    };

    template <>
    struct integer_read_from_stream_impl<varlen_integer>
    {
        template <typename Stream, typename Receiver>
        static void read_from_stream(Stream &stream, Receiver &&rx) {
            struct read_state : std::enable_shared_from_this<read_state>
            {
                using receiver_type = std::remove_reference_t<Receiver>;
                std::array<std::uint8_t, 4> storage;
                receiver_type rx;
                std::uint8_t bytes_read = 0;
                Stream *stream;
                read_state(Receiver rx, Stream &stream)
                    : rx(std::forward<Receiver>(rx)), stream(std::addressof(stream)) {
                }

                void read_one() {
                    boost::asio::async_read(*stream, boost::asio::buffer(&storage[bytes_read], 1),
                                            [me = this->shared_from_this()](
                                                const auto &ec, auto sz) { me->handle_read(ec); });
                }
                void handle_read(const boost::system::error_code &ec) {
                    if (ec) {
                        rx.set_error(ec);
                    }
                    else {
                        if ((storage[bytes_read] & 0x80) == 0) {
                            try {
                                varlen_integer value(storage.begin(),
                                                     (storage.begin() + bytes_read + 1));
                                rx.set_value(value);
                            }
                            catch (boost::system::system_error &err) {
                                rx.set_error(err.code());
                            }
                        }
                        else {
                            bytes_read++;
                            if (bytes_read == 4) {
                                rx.set_error(boost::system::errc::make_error_code(
                                    boost::system::errc::protocol_error));
                            }
                            else {
                                read_one();
                            }
                        }
                    }
                }
            };
            auto state = std::make_shared<read_state>(std::forward<Receiver>(rx), stream);
            state->read_one();
        }
    };
}

template <typename Integer, typename Stream, typename Receiver>
void integer_from_stream(Stream &stream, Receiver &&rx) {
    detail::integer_read_from_stream_impl<Integer>::template read_from_stream(
        stream, std::forward<Receiver>(rx));
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
} // namespace type
} // namespace mqtt5