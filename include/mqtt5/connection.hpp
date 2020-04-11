
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <p0443_v2/asio/write_all.hpp>
#include <p0443_v2/just.hpp>
#include <p0443_v2/type_traits.hpp>
#include <p0443_v2/with.hpp>

#include <mqtt5/protocol/control_packet.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/type_traits/is_detected.hpp>

#include <vector>

namespace mqtt5
{
template <class AsyncStream>
class connection
{
private:
    AsyncStream stream_;
    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> read_buffer_;
public:
    using next_layer_type = typename std::remove_reference_t<AsyncStream>;
    using executor_type = typename next_layer_type::executor_type;

    template <class... Args>
    connection(Args &&... args) : stream_(std::forward<Args>(args)...) {
    }

    next_layer_type &next_layer() {
        return stream_;
    }
    const next_layer_type &next_layer() const {
        return stream_;
    }

    executor_type get_executor() {
        return stream_.get_executor();
    }

    auto control_packet_reader() {
        return p0443_v2::with(
            [this](protocol::control_packet &packet) {
                return p0443_v2::transform(
                    packet.inplace_deserializer(
                        transport::data_fetcher<AsyncStream>(stream_, read_buffer_)),
                    [&packet]() -> protocol::control_packet { return std::move(packet); });
            },
            protocol::control_packet{});
    }

    template <class T>
    auto packet_reader() {
        return p0443_v2::transform(control_packet_reader(), [](auto &&p) -> std::optional<T> {
            return std::move(p).template body_as<T>();
        });
    }

    auto control_packet_writer(protocol::control_packet packet) {
        std::vector<std::uint8_t> buffer;
        packet.serialize([&](auto b) { buffer.push_back(b); });

        return p0443_v2::with(
            [this](const auto &buffer) {
                return p0443_v2::asio::write_all(stream_, boost::asio::buffer(buffer));
            },
            std::move(buffer));
    }
};
} // namespace mqtt5
