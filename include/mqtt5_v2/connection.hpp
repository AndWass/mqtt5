
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <p0443_v2/with.hpp>
#include <mqtt5_v2/protocol/control_packet.hpp>

namespace mqtt5_v2
{
template<class AsyncStream>
class connection
{
private:
    AsyncStream stream_;
    boost::beast::basic_flat_buffer<std::uint8_t> read_buffer_;
public:
    using next_layer_type = std::remove_reference_t<AsyncStream>;
    using lowest_layer_type = typename next_layer_type::lowest_layer_type;
    using executor_type = typename lowest_layer_type::executor_type;
};
}