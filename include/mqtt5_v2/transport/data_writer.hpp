
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <cstdint>
#include <type_traits>

#include <nonstd/span.hpp>

#include <boost/beast/core/flat_buffer.hpp>

namespace mqtt5_v2::transport
{
template <class Stream>
struct data_writer
{
    using buffer_type = boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>>;

    struct writer_handle
    {
        void push_back(std::uint8_t byte) {
            auto buf = buffer_->prepare(1);
            auto *ptr = static_cast<std::uint8_t *>(buf.data());
            ptr[0] = byte;
            buffer_->commit(1);
        }

    private:
        buffer_type *buffer_;
    };

    template <class WriteFunction>
    auto write_data(WriteFunction &&pred) {
    }
private:
    Stream *stream;
    buffer_type *buffer;
};
} // namespace mqtt5_v2::transport