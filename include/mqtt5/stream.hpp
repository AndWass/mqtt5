#pragma once

#include "message/raw.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/flat_buffer.hpp>

namespace mqtt5
{
template <class SockType>
class stream
{
public:
    using next_layer_type = std::remove_reference_t<SockType>;
    using lowest_layer_type = typename next_layer_type::lowest_layer_type;
    using executor_type = typename lowest_layer_type::executor_type;

    template <class... Args>
    stream(Args &&... args) : socket_(std::forward<Args>(args)...) {
    }

    lowest_layer_type &lowest_layer() {
        return socket_.lowest_layer();
    }
    const lowest_layer_type &lowest_layer() const {
        return socket_.lowest_layer();
    }

    next_layer_type &next_layer() {
        return socket_;
    }
    const next_layer_type &next_layer() const {
        return socket_;
    }

    auto write(const mqtt5::message::raw &msg) {
        auto buffer = msg.to_bytes();
        return boost::asio::write(socket_, boost::asio::buffer(buffer));
    }

    void read(mqtt5::message::raw &msg) {
        if (buffered_data_.size() < 2) {
            read_some(16);
            read(msg);
        }
        else {
            auto data = buffered_data_.data();
            // Try to get the total size of data
            try {
                std::size_t bytes_used = 0;
                auto *data_begin = static_cast<std::uint8_t*>(data.data());

                mqtt5::type::varlen_integer remaining_packet_length(
                    data_begin + 1, data_begin + data.size(), &bytes_used);
                auto total_packet_length = 1 + bytes_used + remaining_packet_length.value();
                if (buffered_data_.size() >= total_packet_length)
                {
                    (void)mqtt5::message::deserialize_into(msg, data_begin, data_begin+data.size());
                    buffered_data_.consume(total_packet_length);
                    return;
                }
                auto amount_left_to_read = total_packet_length - buffered_data_.size();
                read_some(amount_left_to_read);
                read(msg);
            }
            catch (...) {
                if (data.size() > 4) {
                    // Bad total size!
                    std::rethrow_exception(std::current_exception());
                }
                read_some(16);
                read(msg);
            }
        }
    }

private:
    void read_some(std::size_t size) {
        auto amount_read = socket_.read_some(buffered_data_.prepare(size));
        buffered_data_.commit(amount_read);
    }
    next_layer_type socket_;
    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> buffered_data_;
};
} // namespace mqtt5