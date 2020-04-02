
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include <mqtt5/protocol/error.hpp>
#include <mqtt5/protocol/fixed_int.hpp>

#include <p0443_v2/lazy.hpp>
#include <p0443_v2/sequence.hpp>
#include <p0443_v2/then.hpp>

namespace mqtt5
{
namespace protocol
{
struct string: detail::unconstructible
{
    template<class Stream>
    [[nodiscard]] static std::string deserialize(transport::data_fetcher<Stream> fetcher)
    {
        auto string_size = fixed_int<std::uint16_t>::deserialize(fetcher);
        auto rest_of_data = fetcher.cspan(string_size);
        std::string retval(reinterpret_cast<const char *>(rest_of_data.data()), string_size);
        fetcher.consume(string_size);
        return retval;
    }

    template <class Writer>
    static void serialize(const std::string &data, Writer &&writer) {
        fixed_int<std::uint16_t>::serialize(static_cast<std::uint16_t>(data.size()), writer);
        for (auto &b : data) {
            writer(b);
        }
    }
};

struct key_value_pair
{
    struct deserialize_result;

    template<class Stream>
    [[nodiscard]] static key_value_pair deserialize(transport::data_fetcher<Stream> fetcher)
    {
        key_value_pair retval;
        retval.key = string::deserialize(fetcher);
        retval.value = string::deserialize(fetcher);
        return retval;
    }

    template <class Writer>
    static void serialize(const key_value_pair& data, Writer &&writer) {
        string::serialize(data.key, writer);
        string::serialize(data.value, writer);
    }

    std::string key;
    std::string value;
};
} // namespace protocol
} // namespace mqtt5
