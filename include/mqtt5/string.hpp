#pragma once

#include <boost/stl_interfaces/iterator_interface.hpp>
#include <initializer_list>
#include <string>
#include <type_traits>

#include <nonstd/span.hpp>
#include <utf8.h>

#include "integer.hpp"

namespace mqtt5
{
class string
{
public:
    template <class Integral, std::enable_if_t<std::is_integral_v<Integral>> * = nullptr>
    explicit string(std::initializer_list<Integral> init) : string(init.begin(), init.end()) {
    }

    explicit string(std::string str) {
        storage_ = std::move(str);
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
        constexpr iterator() noexcept = default;
        iterator(const string &str) noexcept : iterator(str.storage_.begin(), str.storage_.end()) {
        }
        std::uint32_t operator*() const {
            try {
                return utf8::peek_next(pos_, end_);
            }
            catch (utf8::not_enough_room &err) {
                throw std::runtime_error(err.what());
            }
        }
        iterator &operator+=(std::ptrdiff_t i) {
            try {
                utf8::advance(pos_, i, end_);
                return *this;
            }
            catch (utf8::not_enough_room &err) {
                throw std::runtime_error(err.what());
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
                throw std::runtime_error(e.what());
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

    explicit operator std::string() noexcept {
        return storage_;
    }

private:
    void throw_bad_string() const {
        throw std::invalid_argument("Bad UTF-8 formatted string");
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
        if (!utf8::is_valid(storage_)) {
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

template<class Iter>
Iter serialize(const string &str, Iter out)
{
    integer16 size(str.byte_size());
    out = serialize(size, out);
    return std::copy(str.byte_begin(), str.byte_end(), out);
}
} // namespace mqtt5