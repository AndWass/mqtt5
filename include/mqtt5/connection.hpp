
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
namespace detail
{
template <class T>
using lowest_layer_detector = decltype(std::declval<T>().lowest_layer());

template <class T>
using next_layer_detector = decltype(std::declval<T>().next_layer());

template<class T>
auto& get_lowest_layer(T& t) {
    if constexpr(boost::is_detected_v<next_layer_detector, T>)
    {
        using next_layer_t = std::remove_reference_t<decltype(t.next_layer())>;
        if constexpr(std::is_same_v<next_layer_t, T>)
        {
            return t;
        }
        else {
            return mqtt5::detail::get_lowest_layer(t.next_layer());
        }
    }
    else {
        return t;
    }
}
} // namespace detail
/**
 * @brief An MQTT5 connection object.
 *
 * The connection holds an internal object of type AsyncStream
 * which is used to read and write control messages.
 */
template <class AsyncStream>
class connection
{
private:
    AsyncStream stream_;
    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> read_buffer_;

public:
    using next_layer_type = typename std::remove_reference_t<AsyncStream>;
    using executor_type = typename next_layer_type::executor_type;

    /**
     * @brief Construct a connection.
     *
     * All arguments are forwarded to the underlying stream.
     */
    template <class... Args>
    connection(Args &&... args) : stream_(std::forward<Args>(args)...) {
    }

    /**
     * @brief Get a reference to the next stream layer
     */
    next_layer_type &next_layer() {
        return stream_;
    }
    const next_layer_type &next_layer() const {
        return stream_;
    }

    /**
     * @brief Get a reference to the lowest layer socket object.
     *
     * This is usually a TCP/IP socket, but can be wrapped up in
     * various number of layers.
     */
    auto& lowest_layer() {
        return detail::get_lowest_layer(stream_);
    }

    const auto& lowest_layer() const {
        return detail::get_lowest_layer(stream_);
    }

    executor_type get_executor() {
        return stream_.get_executor();
    }

    /**
     * @brief Create a reader for a complete control packet.
     *
     * When the reader is started it will read an entire packet from
     * a stream.
     *
     * Sender value: mqtt5::protocol::control_packet
     * Sender error: std::exception_ptr
     * Sender sets done: yes
     */
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

    /**
     * @brief Create a reader for a complete specific control packet.
     *
     * When the reader is started it will read an entire packet from
     * the connection and attemt to transform it into the specified packet.
     *
     * Sender value: std::optional<T>
     * Sender error: std::exception_ptr
     * Sender sets done: yes
     */
    template <class T>
    auto packet_reader() {
        return p0443_v2::transform(control_packet_reader(), [](auto &&p) -> std::optional<T> {
            return std::move(p).template body_as<T>();
        });
    }

    /**
     * @brief Create a writer for a control packet.
     *
     * When the writer is started it will write the control packet to the
     * stream.
     *
     * Sender value: std::size_t
     * Sender error: std::exception_ptr
     * Sender sets done: yes
     */
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
