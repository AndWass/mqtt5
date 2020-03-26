
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

namespace mqtt5_v2
{
namespace protocol
{
struct binary: detail::unconstructible
{
    using type = std::vector<std::uint8_t>;

    template<class Stream>
    static std::vector<std::uint8_t> deserialize(transport::data_fetcher<Stream> fetcher)
    {
        auto bin_size = fixed_int<std::uint16_t>::deserialize(fetcher);
        auto data = fetcher.cspan();
        if(data.size() < bin_size) {
            throw protocol_error("not enough bytes to convert to binary");
        }
        std::vector<std::uint8_t> retval(data.begin(), data.begin() + bin_size);
        fetcher.consume(bin_size);
        return retval;
    }

    template <class Writer>
    static void serialize(const std::vector<std::uint8_t>& ref, Writer &&writer) {
        fixed_int<std::uint16_t>::serialize(static_cast<std::uint16_t>(ref.size()), writer);
        for (auto &b : ref) {
            writer(b);
        }
    }

private:
};
} // namespace protocol
} // namespace mqtt5_v2
