//#include <mqtt5_v2/protocol/publish.hpp>
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
    connect.flags = connect_packet::clean_start_flag | connect_packet::will_flag |
                    connect_packet::will_qos_1;

    connect.will_topic = "/mqtt5_v2/lastwill";
    connect.set_will_payload("This is my last will and testament");
    //connect.will_payload.emplace("This is my last will and testament");
    connect.will_properties.emplace();

    co_await p0443_v2::await_sender(connection.control_packet_writer(connect));

    auto read_packet = co_await p0443_v2::await_sender(connection.control_packet_reader());
    auto *connack = read_packet.body_as<mqtt5_v2::protocol::connack>();

    if (connack && connack->reason_code == 0) {
        std::cout << "Successfully connected to " << host << ":" << port << std::endl;
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
