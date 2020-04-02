
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5_v2/transport/data_fetcher.hpp>
#include <mqtt5_v2/protocol/deserialize.hpp>
#include <mqtt5_v2/protocol/error.hpp>

#include "detail/unconstructible.hpp"

namespace mqtt5_v2
{
namespace protocol
{
struct varlen_int: detail::unconstructible
{
    using type = std::uint32_t;

    static constexpr std::uint32_t max_value() {
        return 268'435'455;
    }

    [[nodiscard]] static bool can_deserialize(nonstd::span<const std::uint8_t> data) {
        if(data.size() >= 4) {
            return true;
        }
        for(auto b: data) {
            if((b & 128) == 0) {
                return true;
            }
        }
        return false;
    }

    template<class Stream>
    [[nodiscard]] static std::uint32_t deserialize(transport::data_fetcher<Stream> data)
    {
        std::uint32_t value;
        std::uint32_t bytes_used;

        value = deserialize(data, bytes_used);
        data.consume(bytes_used);

        return value;
    }

    template <class Writer>
    static void serialize(std::uint32_t value, Writer&& writer) {
        if(value > max_value()) {
            throw protocol_error("value exceeding maximum allowed for varlen int");
        }
        auto val = value;
        do {
            std::uint8_t byte = val % 128;
            val /= 128;
            if (val > 0) {
                byte |= 0x80;
            }
            writer(byte);
        } while (val > 0);
    }

private:
    template<class Stream>
    [[nodiscard]] static std::uint32_t deserialize(transport::data_fetcher<Stream> data, std::uint32_t &data_used) {
        auto data_span = data.cspan();
        std::uint32_t multiplier = 1;
        std::uint32_t value = 0;
        auto iter = data_span.begin();
        data_used = 0;

        std::uint32_t encoded_byte;
        do {
            encoded_byte = *iter++;
            data_used++;
            value += (encoded_byte & 127) * multiplier;
            if (multiplier > 128 * 128 * 128) {
                throw protocol_error("data overflow");
            }
            else if (iter == data_span.end() && (encoded_byte & 128) != 0) {
                throw std::runtime_error("not enough data");
            }
            multiplier *= 128;
        } while ((encoded_byte & 128) != 0);
        return value;
    }
};
} // namespace protocol
} // namespace mqtt5_v2
