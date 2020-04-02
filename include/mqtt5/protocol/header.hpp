
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5/protocol/fixed_int.hpp>
#include <mqtt5/protocol/varlen_int.hpp>

#include <mqtt5/transport/data_fetcher.hpp>

#include <p0443_v2/transform.hpp>

namespace mqtt5::protocol
{
struct header
{
    header() = default;

    template<class P>
    header(std::uint8_t type_, std::uint8_t flags_, const P& packet) {
        set_type(type_);
        set_flags(flags_);

        std::uint32_t body_len = 0;
        packet.serialize_body([&](auto) {
            body_len++;
        });
        set_remaining_length(body_len);
    }

    void set_type(std::uint8_t t) {
        type_flags_ = flags() + (t << 4);
    }
    
    std::uint8_t type() {
        return type_flags_ >> 4;
    }

    void set_flags(std::uint8_t f) {
        type_flags_ = (type_flags_ & 0xf0) | (f & 0x0f);
    }
    std::uint8_t flags() {
        return type_flags_ & 0x0F;
    }

    void set_remaining_length(std::uint32_t len) {
        remaining_length_ = len;
    }
    std::uint32_t remaining_length() {
        return remaining_length_;
    }

    template<class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        auto fetcher_predicate = [](auto fetcher) -> std::uint32_t {
            if(fetcher.size() < 2) {
                return static_cast<std::uint32_t>(2-fetcher.size());
            }
            auto data = fetcher.cspan().subspan(1);
            if(varlen_int::can_deserialize(data))
            {
                return 0;
            }
            return 1;
        };
        auto transformer = [this](auto fetcher) {
            auto data = fetcher.cspan();
            type_flags_ = data[0];
            fetcher.consume(1);

            remaining_length_ = varlen_int::deserialize(fetcher);

            return fetcher;
        };
        return p0443_v2::transform(data_fetcher.get_data_until(fetcher_predicate), transformer);
    }

    template<class Writer>
    void serialize(Writer&& writer) const {
        fixed_int<std::uint8_t>::serialize(type_flags_, writer);
        varlen_int::serialize(remaining_length_, writer);
    }
private:
    std::uint8_t type_flags_;
    varlen_int::type remaining_length_;
};
}