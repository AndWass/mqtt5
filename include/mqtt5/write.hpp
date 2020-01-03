#pragma once

#include <boost/system/error_code.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>

#include <vector>
#include <cstdint>

#include "message/raw.hpp"

namespace mqtt5
{
namespace net = boost::asio;
template<class Stream>
std::size_t write(Stream &stream, const message::raw &msg, boost::system::error_code &ec)
{
    std::vector<std::uint8_t> bytes;
    (void)mqtt5::message::serialize(msg, std::back_inserter(bytes));
    return net::write(stream, boost::asio::buffer(bytes), ec);
}

template<class Stream>
std::size_t write(Stream &stream, const message::raw &msg)
{
    std::vector<std::uint8_t> bytes;
    (void)mqtt5::message::serialize(msg, std::back_inserter(bytes));
    return net::write(stream, boost::asio::buffer(bytes));
}
}