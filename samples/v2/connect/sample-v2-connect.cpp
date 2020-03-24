#include <mqtt5_v2/protocol/control_packet.hpp>
#include <mqtt5_v2/connection.hpp>

#include <p0443_v2/await_sender.hpp>
#include <p0443_v2/immediate_task.hpp>
#include <p0443_v2/asio/resolve.hpp>
#include <p0443_v2/asio/connect.hpp>
#include <p0443_v2/asio/write_all.hpp>

#include <boost/asio.hpp>

#include <iostream>

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;

p0443_v2::immediate_task mqtt_task(net::io_context& io) {
    mqtt5_v2::connection<tcp::socket> connection(io);

    {
        tcp::resolver resolver(io);
        auto resolve_result = co_await p0443_v2::await_sender(p0443_v2::asio::resolve(resolver, "test.mosquitto.org", "1883"));
        auto connected_ep = co_await p0443_v2::await_sender(p0443_v2::asio::connect_socket(connection.next_layer(), resolve_result));
        std::cout <<  "Connected to " << connected_ep.address().to_string() << ":" << connected_ep.port() << "\n";
    }
    
    mqtt5_v2::protocol::connect connect;
    connect.flags = 12; // Will QoS1 and Will flag
    connect.will_topic = "/mqtt5_v2/lastwill";
    connect.will_payload.emplace(std::in_place, "This is my last will and testament");
    connect.will_properties.emplace();
    std::cout << co_await p0443_v2::await_sender(connection.control_packet_writer(connect)) << " bytes written\n";


    auto read_packet = co_await p0443_v2::await_sender(connection.control_packet_reader());
    auto *connack = read_packet.body_as<mqtt5_v2::protocol::connack>();

    if(connack && connack->reason_code == 0) {
        std::cout << "Received successful connack!\n";

        mqtt5_v2::protocol::publish publish;
        publish.set_topic("/mqtt5_v2/some_topic");
        publish.set_payload("This is published from mqtt5_v2 using c++ coroutines!");
        std::cout << "Published " << co_await p0443_v2::await_sender(connection.control_packet_writer(publish)) << " bytes\n";
    }
    else if(connack) {
        std::cout << "Received unsuccessful connack, reason code = " << (int)connack->reason_code << "\n";
    }
    else {
        std::cout << "Received unexpected packet\n";
    }
}

int main() {
    net::io_context io;
    mqtt_task(io);

    io.run();
}
