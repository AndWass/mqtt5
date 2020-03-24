#pragma once

#include <boost/stl_interfaces/iterator_interface.hpp>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <memory>

#include <boost/asio/read.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#include <nonstd/span.hpp>
#include <utf8.h>

#include "integer.hpp"

namespace mqtt5
{
namespace type
{
class string
{
public:
    string() = default;

    template <class Integral, std::enable_if_t<std::is_integral_v<Integral>> * = nullptr>
    explicit string(std::initializer_list<Integral> init) : string(init.begin(), init.end()) {
    }

    explicit string(std::string str) {
        storage_ = std::move(str);
        validate();
    }
    explicit string(std::string_view str) : storage_(str) {
        validate();
    }
    explicit string(const char *str) : storage_(str) {
        validate();
    }

    explicit string(nonstd::span<const std::uint8_t> data) : string(data.begin(), data.end()) {
    }

    template <class Iter>
    string(Iter begin, Iter end) {
        storage_ = std::string(begin, end);
        validate();
    }

    class iterator : public boost::stl_interfaces::iterator_interface<
                         iterator, std::random_access_iterator_tag, std::uint32_t, std::uint32_t>
    {
    public:
        iterator() = default;
        iterator(const string &str) noexcept : iterator(str.storage_.begin(), str.storage_.end()) {
        }
        std::uint32_t operator*() const {
            try {
                return utf8::peek_next(pos_, end_);
            }
            catch (utf8::not_enough_room &err) {
                boost::throw_exception(
                    boost::system::system_error(boost::system::errc::make_error_code(
                        boost::system::errc::argument_out_of_domain)));
            }
        }
        iterator &operator+=(std::ptrdiff_t i) {
            try {
                utf8::advance(pos_, i, end_);
                return *this;
            }
            catch (utf8::not_enough_room &err) {
                boost::throw_exception(boost::system::system_error(
                    boost::system::errc::make_error_code(boost::system::errc::no_buffer_space)));
            }
        }

        auto operator-(iterator rhs) {
            try {
                auto char_dist = pos_ - rhs.pos_;
                if (char_dist > 0) {
                    return utf8::distance(rhs.pos_, pos_);
                }
                return 0 - utf8::distance(pos_, rhs.pos_);
            }
            catch (utf8::exception &e) {
                boost::throw_exception(boost::system::system_error(
                boost::system::errc::make_error_code(boost::system::errc::io_error)));
            }
        }

    private:
        iterator(std::string::const_iterator pos, std::string::const_iterator end) noexcept
            : pos_(pos), end_(end) {
        }
        friend class string;
        std::string::const_iterator pos_;
        std::string::const_iterator end_;
    };

    iterator begin() const noexcept {
        return iterator(storage_.begin(), storage_.end());
    }
    iterator end() const noexcept {
        return iterator(storage_.end(), storage_.end());
    }

    auto byte_begin() const noexcept {
        return storage_.begin();
    }
    auto byte_end() const noexcept {
        return storage_.end();
    }
    auto byte_size() const noexcept {
        return storage_.size();
    }

    bool empty() const noexcept {
        return storage_.empty();
    }

    template <class T, std::enable_if_t<std::is_assignable_v<std::string, T>> * = nullptr>
    string &operator=(T &&val) {
        storage_ = std::forward<T>(val);
        return *this;
    }

    template <class T, std::enable_if_t<std::is_void_v<std::void_t<decltype(
                           std::declval<std::string &>() == std::declval<T>())>>> * = nullptr>
    bool operator==(const T &rhs) const {
        return storage_ == rhs;
    }

    explicit operator std::string() noexcept {
        return storage_;
    }

    std::string to_string() const noexcept {
        return storage_;
    }

private:
    void throw_bad_string() const {
        boost::throw_exception(boost::system::system_error(
                boost::system::errc::make_error_code(boost::system::errc::argument_out_of_domain)));
    }

    void validate() const {
        auto is_in_range = [](std::uint32_t val, std::uint32_t min, std::uint32_t max) {
            return val >= min && val <= max;
        };
        auto is_reserved_ascii = [&](std::uint32_t elem) {
            return elem <= 0x1f || is_in_range(elem, 0x7f, 0x9f);
        };
        auto is_unicode_noncharacter = [&](std::uint32_t elem) {
            for (std::uint32_t x = 0x1FFFE; x <= 0x10FFFE; x += 0x10000) {
                if (is_in_range(elem, x, x + 1)) {
                    return true;
                }
            }
            return is_in_range(elem, 0xFDD0, 0xFDEF) || is_in_range(elem, 0xFFFE, 0xFFFF);
        };
        auto is_disallowed_code_point = [&](std::uint32_t elem) {
            return is_reserved_ascii(elem) || is_unicode_noncharacter(elem);
        };
        auto is_empty = storage_.begin() == storage_.end();
        if (!utf8::is_valid(storage_.begin(), storage_.end())) {
            throw_bad_string();
        }
        if (std::find_if(this->begin(), this->end(), is_disallowed_code_point) != this->end()) {
            throw_bad_string();
        }
    }
    std::string storage_;
};

inline auto begin(string &s) {
    return s.begin();
}
inline auto end(string &s) {
    return s.end();
}

template <class Iter>
[[nodiscard]] Iter serialize(const string &str, Iter out) {
    integer16 size(static_cast<std::uint16_t>(str.byte_size()));
    out = type::serialize(size, out);
    return std::copy(str.byte_begin(), str.byte_end(), out);
}

template <class Iter>
[[nodiscard]] Iter deserialize_into(string &str, Iter begin, Iter end) {
    integer16 size;
    begin = type::deserialize_into(size, begin, end);
    auto data_left = end - begin;
    if (data_left < size.value()) {
        boost::throw_exception(boost::system::system_error(
                boost::system::errc::make_error_code(boost::system::errc::protocol_error)));
    }
    str = string(begin, begin + size.value());
    return begin + size.value();
}

template <typename Stream, typename Receiver>
void string_from_stream(Stream &stream, Receiver &&receiver) {
    struct read_receiver {
        Stream *stream;
        std::remove_reference_t<Receiver> rx;

        void set_value(integer16 i16) {
            auto str = std::make_shared<std::vector<std::uint8_t>>();
            str->resize(i16.value());
            boost::asio::async_read(*stream, boost::asio::buffer(*str), [str, rx = std::forward<Receiver>(this->rx)](const auto &ec, auto sz) mutable {
                if(ec) {
                    rx.set_error(ec);
                }
                else {
                    try {
                        string validated(*str);
                        rx.set_value(validated);
                    }
                    catch(boost::system::system_error &err) {
                        rx.set_error(err.code());
                    }
                }
            });
        }
        void set_error(const boost::system::error_code &ec) {
            rx.set_error(ec);
        }
    };

    integer_from_stream<integer16>(stream, read_receiver{std::addressof(stream), std::forward<Receiver>(receiver)});
}

struct key_value_pair
{
    key_value_pair() = default;
    template <typename KeyString, typename ValueString>
    key_value_pair(KeyString &&k, ValueString &&v)
        : key(std::forward<KeyString>(k)), value(std::forward<ValueString>(v)) {
    }
    string key;
    string value;
};

template <class Iter>
[[nodiscard]] Iter serialize(const key_value_pair &kv, Iter out) {
    out = type::serialize(kv.key, out);
    return type::serialize(kv.value, out);
}

template <class Iter>
[[nodiscard]] Iter deserialize_into(key_value_pair &kv, Iter begin, Iter end) {
    begin = type::deserialize_into(kv.key, begin, end);
    return type::deserialize_into(kv.value, begin, end);
}
} // namespace type
} // namespace mqtt5