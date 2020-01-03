#pragma once

#include <boost/asio/io_context.hpp>
#include <type_traits>
#include <string_view>
#include <optional>
#include <string>

#include "message/connect.hpp"
#include "message/connack.hpp"

#include "publish.hpp"

#include "write.hpp"
#include "read.hpp"

namespace mqtt5
{
template<typename SockType>
class connection
{
public:
    using next_layer_type = std::remove_reference_t<SockType>;
    using lowest_layer_type = typename next_layer_type::lowest_layer_type;
    using executor_type = typename lowest_layer_type::executor_type;

    std::optional<std::string> username,password;
    bool clean_start = true;
    std::optional<publish> will;
    std::chrono::seconds will_delay;

    connection(boost::asio::io_context &io): io_(&io), socket_(io) {}

    lowest_layer_type& lowest_layer() {
        return socket_.lowest_layer();
    }
    const lowest_layer_type& lowest_layer() const {
        return socket_.lowest_layer();
    }

    next_layer_type& next_layer() {
        return socket_;
    }
    const next_layer_type& next_layer() const {
        return socket_;
    }

    void handshake() {
        handshake(message::connect::generate_client_id().to_string());
    }
    void handshake(std::string_view client_id) {
        message::connect con;
        con.client_id = type::string(client_id);
        if(will) {
            con.will = *will;
            con.will.properties.delay_interval = will_delay;
            con.flags.will_flag = true;
            con.flags.will_qos = will->qos;
            con.flags.will_retain = will->retain;
        }

        if(username) {
            con.username = *username;
            con.flags.username = true;
        }
        
        if(password) {
            con.password = *password;
            con.flags.password = true;
        }
        con.flags.clean_start = clean_start;
        
        mqtt5::write(socket_, con);
        auto maybe_connack = mqtt5::read(socket_);
        if(maybe_connack.type == packet_type::connack) {
            message::connack connack = maybe_connack.template convert_to<message::connack>();
        }
    }
private:
    boost::asio::io_context *io_;
    SockType socket_;
};
}