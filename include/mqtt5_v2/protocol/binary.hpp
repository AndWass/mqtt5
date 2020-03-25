
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include <utf8.h>

#include <mqtt5_v2/protocol/error.hpp>
#include <mqtt5_v2/protocol/fixed_int.hpp>

#include <p0443_v2/then.hpp>

namespace mqtt5_v2
{
namespace protocol
{
using std::begin;
using std::end;
struct binary
{
    binary() = default;
    explicit binary(const std::vector<std::uint8_t> &str) : value_(str) {
    }

    template <class IterB, class IterE>
    explicit binary(IterB start, IterE end) : value_(std::in_place_index<1>, start, end) {
    }

    template <class Container, class = decltype(begin(std::declval<Container>())),
              class = decltype(end(std::declval<Container>()))>
    binary(std::in_place_t, Container &&c) : binary(begin(c), end(c)) {
    }

    const std::vector<std::uint8_t> &value() const {
        static std::vector<std::uint8_t> empty;
        if (value_.index() != 0) {
            return std::get<1>(value_);
        }
        return empty;
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
                        auto *data_begin = data.cdata();
                        auto *data_end = data_begin + amount_to_get;
                        this->value_.emplace<1>(std::vector<std::uint8_t>(data_begin, data_end));
                        data.consume(amount_to_get);
                        return data;
                    });
            });
    }

    nonstd::span<const std::uint8_t> set_from_bytes(nonstd::span<const std::uint8_t> data) {
        fixed_int<std::uint16_t> length;
        data = length.set_from_bytes(data);
        if (data.size() < length.value) {
            throw protocol_error("not enough bytes to convert to binary");
        }
        value_ = std::vector<std::uint8_t>(data.begin(), data.begin() + length.value);
        return data.subspan(length.value);
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        fixed_int<std::uint16_t> len;
        auto &ref = value();
        len.value = static_cast<std::uint16_t>(ref.size());
        len.serialize(writer);
        for (auto &b : ref) {
            writer(b);
        }
    }

    binary &operator=(const nonstd::span<const std::uint8_t> data) {
        value_.emplace<1>(data.begin(), data.end());
        return *this;
    }

private:
    std::variant<fixed_int<std::uint16_t>, std::vector<std::uint8_t>> value_;
};
} // namespace protocol
} // namespace mqtt5_v2
