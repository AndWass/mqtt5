
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

#include <mqtt5_v2/transport/data_fetcher.hpp>

namespace mqtt5_v2
{
namespace protocol
{
struct varlen_int
{
    std::uint32_t value;

    template<class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        return data_fetcher.get_data_until(
            [this](transport::data_fetcher<Stream> fetcher) {
                if(fetcher.size() == 0) {
                    return 1;
                }
                auto available_data = fetcher.cspan();
                std::uint32_t data_used = 0;
                if(this->try_set_from_data(available_data, data_used)) {
                    fetcher.buffer->consume(data_used);
                    return 0;
                }
                return 1;
            }
        );
    }

    nonstd::span<const std::uint8_t> set_from_bytes(nonstd::span<const std::uint8_t> data) {
        std::uint32_t data_used = 0;
        if(!try_set_from_data(data, data_used)) {
            throw std::runtime_error("bytes not convertible to varlen_int");
        }
        return data.subspan(data_used);
    }

private:
    bool try_set_from_data(nonstd::span<const std::uint8_t> data, std::uint32_t &data_used) {
        std::uint32_t multiplier = 1;
        value = 0;
        auto iter = data.begin();
        data_used = 0;
        std::uint32_t encoded_byte;
        do
        {
            encoded_byte = *iter++;
            data_used++;
            value += (encoded_byte & 127) * multiplier;
            if (multiplier > 128*128*128) {
                throw std::runtime_error("data overflow");
            }
            else if(iter == data.end() && (encoded_byte & 128) != 0)
            {
                return false;
            }
            multiplier *= 128;
        }
        while ((encoded_byte & 128) != 0);
        return true;

    }
};
}
}