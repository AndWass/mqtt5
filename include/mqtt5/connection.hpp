#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <memory>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "message/connack.hpp"
#include "message/connect.hpp"
#include "message/raw.hpp"

#include "publish.hpp"
#include "operation.hpp"
#include "read.hpp"
#include "write.hpp"

namespace mqtt5
{
namespace net = boost::asio;

template <typename SockType>
class connection
{
public:
    using next_layer_type = std::remove_reference_t<SockType>;
    using lowest_layer_type = typename next_layer_type::lowest_layer_type;
    using executor_type = typename lowest_layer_type::executor_type;

    std::string client_id;
    std::optional<std::string> username, password;
    bool clean_start = true;
    std::optional<publish> will;
    std::chrono::seconds will_delay;

    connection(boost::asio::io_context &io) : io_(io), socket_(io_) {
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

    template<typename Receiver>
    void read(Receiver &&receiver) {
        mqtt5::read(socket_, std::forward<Receiver>(receiver));
    }

    template<typename Receiver>
    void write(message::raw packet, Receiver &&receiver) {
        mqtt5::write(socket_, std::move(packet), std::forward<Receiver>(receiver));
    }

    template <typename Receiver>
    void handshake(Receiver &&receiver) {
        auto shared_rx = std::make_shared<std::remove_reference_t<Receiver>>(std::forward<Receiver>(receiver));
        if (client_id.empty()) {
            client_id = generate_client_id();
        }
        message::connect con;
        con.client_id = type::string(client_id);
        if (will) {
            con.will = *will;
            con.will.properties.delay_interval = will_delay;
            con.flags.will_flag = true;
            con.flags.will_qos = will->qos;
            con.flags.will_retain = will->retain;
        }

        if (username) {
            con.username = *username;
            con.flags.username = true;
        }

        if (password) {
            con.password = *password;
            con.flags.password = true;
        }
        con.flags.clean_start = clean_start;

        net::dispatch(io_, [rx = shared_rx, con_ = std::move(con),
                             this]() {
            auto on_error = [rx](const auto &ec) { rx->set_error(ec); };
            auto read_receiver = mqtt5::on_value_or_error(
                [rx](const auto &packet) {
                    if (packet.type == packet_type::connack) {
                        message::connack connack = packet.template convert_to<message::connack>();
                        rx->set_done();
                    }
                    else {
                        rx->set_error(boost::system::errc::make_error_code(
                            boost::system::errc::protocol_error));
                    }
                },
                on_error);

            auto write_receiver = mqtt5::on_done_or_error(
                [rx = std::move(read_receiver), this, on_error] { mqtt5::read(socket_, rx); },
                on_error);
            mqtt5::write(socket_, con_, write_receiver);
        });
    } // namespace mqtt5

    static std::string generate_client_id() noexcept {
        static const std::string_view string_characters =
            "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        auto uuid1 = boost::uuids::random_generator()();
        auto uuid2 = boost::uuids::random_generator()();
        std::string retval;

        auto add_uuid = [&retval](const auto &uuid) {
            for (auto b : uuid) {
                if (retval.size() == 23) {
                    break;
                }
                retval += string_characters[b % string_characters.size()];
            }
        };

        add_uuid(uuid1);
        add_uuid(uuid2);

        return retval;
    }

private:
    boost::asio::io_context::strand io_;
    SockType socket_;
};
} // namespace mqtt5