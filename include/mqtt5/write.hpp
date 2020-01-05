#pragma once

#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/post.hpp>
#include <boost/system/error_code.hpp>

#include <cstdint>
#include <memory>
#include <vector>

#include "message/raw.hpp"

namespace mqtt5
{
template <typename Stream, typename Receiver>
void write(Stream &stream, const message::raw &msg, Receiver &&receiver) {
    auto bytes = std::make_shared<std::vector<std::uint8_t>>();
    auto rx = std::make_shared<std::remove_reference_t<Receiver>>(std::forward<Receiver>(receiver));
    try {
        (void)mqtt5::message::serialize(msg, std::back_inserter(*bytes));
    }
    catch (const boost::system::error_code &ec) {
        net::post(stream.get_executor(), [ec, rx]() { rx->set_error(ec); });
        return;
    }

    net::async_write(stream, boost::asio::buffer(*bytes),
                     [bytes, rx](const auto &ec, auto ignored) {
                         (void)ignored;
                         if (!ec) {
                             rx->set_done();
                         }
                         else {
                             rx->set_error(ec);
                         }
                     });
}
} // namespace mqtt5