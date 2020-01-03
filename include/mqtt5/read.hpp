#pragma once

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/system/error_code.hpp>

#include "message/raw.hpp"
#include "type/integer.hpp"

namespace mqtt5
{
namespace net = boost::asio;
template<class Stream>
message::raw read(Stream &stream)
{
    std::vector<std::uint8_t> data(2);
    net::read(stream, net::buffer(data));
    while((data[data.size()-1] & 0x80))
    {
        std::uint8_t b[1];
        net::read(stream, net::buffer(b));
        data.push_back(b[0]);
    }
    type::varlen_integer remaining_size(data.begin()+1, data.end());
    auto header_size = data.size();
    data.resize(header_size+remaining_size.value());
    net::read(stream, net::buffer(data.data()+header_size, remaining_size.value()));

    message::raw retval;
    (void)message::deserialize_into(retval, data.begin(), data.end());
    return retval;
}

}