#pragma once

#include <memory>
#include <type_traits>

#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/system/error_code.hpp>

#include "message/raw.hpp"
#include "type/integer.hpp"

namespace mqtt5
{
namespace net = boost::asio;
template <typename Stream, typename Receiver>
void read(Stream &stream, Receiver &&receiver) {

    struct read_op : boost::enable_shared_from_this<read_op>
    {
        using receiver_t = std::remove_reference_t<Receiver>;

        int state_ = 0;
        std::vector<std::uint8_t> data;

        Stream *stream_;
        receiver_t rx_;

        read_op(Stream &stream, Receiver rx)
            : stream_(std::addressof(stream)), rx_(std::forward<Receiver>(rx)) {
        }

        void resume() {
            if (state_ == 2) {
                message::raw retval;
                (void)message::deserialize_into(retval, data.begin(), data.end());
                rx_.set_value(std::move(retval));
            }
            else {
                try {
                    auto completion = [me = this->shared_from_this()](const auto &ec, auto sz) {
                        me->resume(ec);
                    };
                    switch (state_) {
                    case 0:
                        data.resize(2);
                        ++state_;
                        net::async_read(*stream_, net::buffer(data), completion);
                        break;
                    case 1:
                        if (data[data.size() - 1] & 0x80) {
                            data.resize(data.size() + 1);
                            net::async_read(*stream_,
                                            net::buffer(data.data() + (data.size() - 1), 1),
                                            completion);
                        }
                        else {
                            type::varlen_integer remaining_size(data.begin() + 1, data.end());
                            auto header_size = data.size();
                            data.resize(header_size + remaining_size.value());
                            net::async_read(
                                *stream_,
                                net::buffer(data.data() + header_size, remaining_size.value()),
                                completion);
                            ++state_;
                        }
                    }
                }
                catch (boost::system::error_code &ec) {
                    rx_.set_error(ec);
                }
            }
        }

        void resume(const boost::system::error_code &ec) {
            if (ec) {
                rx_.set_error(ec);
            }
            else {
                resume();
            }
        }
    };
    auto op = boost::make_shared<read_op>(stream, std::forward<Receiver>(receiver));
    op->resume();
}

} // namespace mqtt5