
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5_v2/protocol/header.hpp>
#include <mqtt5_v2/protocol/connect.hpp>

#include <p0443_v2/then.hpp>

#include <vector>

namespace mqtt5_v2::protocol
{
struct control_packet
{
    header fixed_sized_header;
    std::vector<std::uint8_t> rest_of_data;

    template<class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        return p0443_v2::then(fixed_sized_header.inplace_deserializer(data_fetcher), [this](transport::data_fetcher<Stream> fetcher) {
            return p0443_v2::transform(fetcher.get_data(fixed_sized_header.remaining_length()), [this](transport::data_fetcher<Stream> fetcher) {
                auto data_span = fetcher.cspan();
                std::size_t data_span_size = data_span.size();
                rest_of_data.clear();
                rest_of_data.reserve(data_span_size);
                std::copy(data_span.begin(), data_span.end(), std::back_inserter(rest_of_data));
            });
        });
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        fixed_sized_header.serialize(writer);
        for(const auto& d: rest_of_data) {
            writer(d);
        }
    }

    control_packet& operator=(const connect& c) {
        rest_of_data.clear();
        fixed_sized_header.set_type(1);
        fixed_sized_header.set_flags(0);
        c.serialize([this](auto b) {
            rest_of_data.push_back(b);
        });
        fixed_sized_header.set_remaining_length(rest_of_data.size());
        return *this;
    }
};
}
