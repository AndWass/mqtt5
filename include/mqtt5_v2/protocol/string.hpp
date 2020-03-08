
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include <utf8.h>

#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/error.hpp>

#include <p0443_v2/sequence.hpp>
#include <p0443_v2/then.hpp>

namespace mqtt5_v2
{
namespace protocol
{
struct string
{
    template <class SIter, class EIter>
    static bool is_valid(SIter begin, EIter end) {
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
        auto is_empty = begin == end;
        if (!utf8::is_valid(begin, end)) {
            return false;
        }
        if (std::find_if(begin, end, is_disallowed_code_point) != end) {
            return false;
        }
        return true;
    }

    string() = default;
    explicit string(const std::string& str): value_(str) {}

    const std::string &value() const {
        static std::string empty = "";
        if (value_.index() != 0) {
            return std::get<1>(value_);
        }
        return empty;
    }

    bool is_valid() const {
        if (value_.index() == 0) {
            return false;
        }

        auto &ref = std::get<1>(value_);
        return is_valid(ref.begin(), ref.end());
    }

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data) {
        value_ = fixed_int<std::uint16_t>{};
        auto &len_ref = std::get<0>(value_);
        // First read a 16 bit uint and then read the indicated amount of data
        return p0443_v2::then(
            len_ref.inplace_deserializer(data), [this](transport::data_fetcher<Stream> fetcher) {
                return p0443_v2::transform(
                    fetcher.get_data(std::get<0>(value_).value),
                    [this](transport::data_fetcher<Stream> data) {
                        std::size_t amount_to_get = std::get<0>(value_).value;
                        this->value_.emplace<1>(std::string(
                            reinterpret_cast<const char *>(data.cdata()), amount_to_get));
                        data.buffer->consume(amount_to_get);
                        return data;
                    });
            });
    }

    nonstd::span<const std::uint8_t> set_from_bytes(nonstd::span<const std::uint8_t> data) {
        fixed_int<std::uint16_t> length;
        data = length.set_from_bytes(data);
        if(data.size() < length.value) {
            throw protocol_error("not enough bytes to convert to string");
        }
        value_ = std::string(reinterpret_cast<const char*>(data.data()), std::size_t(length.value));
        return data.subspan(length.value);
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        fixed_int<std::uint16_t> len;
        auto &ref = value();
        len.value = ref.size();
        len.serialize(writer);
        for(auto& b: ref) {
            writer(b);
        }
    }

    template<class T, std::enable_if_t<std::is_constructible_v<std::string, T>>* = nullptr>
    string& operator=(T&& rhs) {
        value_.emplace<1>(std::forward<T>(rhs));
        return *this;
    }

private:
    std::variant<fixed_int<std::uint16_t>, std::string> value_;
};

struct key_value_pair
{
    const std::string& key() const {
        return key_.value();
    }
    const std::string& value() const {
        return value_.value();
    }
    bool is_valid() const {
        return key_.is_valid() && value_.is_valid();
    }

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data) {
        return p0443_v2::sequence(key_.inplace_deserializer(data), value_.inplace_deserializer(data));
    }

    nonstd::span<const std::uint8_t> set_from_bytes(nonstd::span<const std::uint8_t> data) {
        data = key_.set_from_bytes(data);
        return value_.set_from_bytes(data);
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        key_.serialize(writer);
        value_.serialize(writer);
    }
private:
    string key_;
    string value_;
};
} // namespace protocol
} // namespace mqtt5_v2