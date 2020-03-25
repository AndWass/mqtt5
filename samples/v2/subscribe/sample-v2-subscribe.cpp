#include <mqtt5_v2/connection.hpp>
#include <mqtt5_v2/protocol/control_packet.hpp>

#include <p0443_v2/asio/connect.hpp>
#include <p0443_v2/asio/resolve.hpp>
#include <p0443_v2/asio/write_all.hpp>
#include <p0443_v2/await_sender.hpp>
#include <p0443_v2/immediate_task.hpp>

#include <boost/asio.hpp>

#include <iostream>

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;

constexpr auto host = "test.mosquitto.org";
constexpr auto port = "1883";

p0443_v2::immediate_task mqtt_task(net::io_context &io) {
    mqtt5_v2::connection<tcp::socket> connection(io);

    {
        tcp::resolver resolver(io);
        auto resolve_result = co_await p0443_v2::await_sender(
            p0443_v2::asio::resolve(resolver, host, port));
        auto connected_ep = co_await p0443_v2::await_sender(
            p0443_v2::asio::connect_socket(connection.next_layer(), resolve_result));
    }

    using connect_packet = mqtt5_v2::protocol::connect;

    mqtt5_v2::protocol::connect connect;
    connect.flags = connect_packet::clean_start_flag;
    co_await p0443_v2::await_sender(connection.control_packet_writer(connect));
    auto read_packet = co_await p0443_v2::await_sender(connection.control_packet_reader());
    auto *connack = read_packet.body_as<mqtt5_v2::protocol::connack>();

    if (connack && connack->reason_code == 0) {
        std::cout << "Successfully connected to " << host << ":" << port << std::endl;
        mqtt5_v2::protocol::subscribe subscribe;
        subscribe.set_packet_identifier(10);
        subscribe.topic_filters().emplace_back("/mqtt5_v2/+", 0);
        co_await p0443_v2::await_sender(connection.control_packet_writer(subscribe));

        read_packet = co_await p0443_v2::await_sender(connection.control_packet_reader());
        auto *suback = read_packet.body_as<mqtt5_v2::protocol::suback>();
        if(suback && suback->packet_identifier_ == 10) {
            if(!suback->reason_codes_.empty() && suback->reason_codes_.front() == 0) {
                std::cout << "Waiting for published messages on topic '/mqtt5_v2/+'\n";
                while(true) {
                    read_packet = co_await p0443_v2::await_sender(connection.control_packet_reader());
                    if(auto *publish = read_packet.body_as<mqtt5_v2::protocol::publish>(); publish) {
                        std::cout << "Received publish\n";
                        std::string data(publish->get_payload_ref().begin(), publish->get_payload_ref().end());
                        std::cout << "  '" << data << "'\n";
                        std::cout << "  [Topic = " << publish->topic() << "]\n";
                    }
                }
            }
        }
    }
    else if (connack) {
        std::cout << "Received unsuccessful connack, reason code = " << (int)connack->reason_code
                  << "\n";
    }
    else {
        std::cout << "Received unexpected packet\n";
    }
    std::cout << std::endl;
}

int main() {
    net::io_context io;
    mqtt_task(io);

    io.run();
}
