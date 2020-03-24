#include <mqtt5_v2/protocol/control_packet.hpp>
#include <mqtt5_v2/protocol/connect.hpp>
#include <mqtt5_v2/protocol/connack.hpp>
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

template<class T>
const char* signature() {
    return __PRETTY_FUNCTION__;
}

void test(mqtt5_v2::protocol::control_packet& p) {
    auto ack = std::get<mqtt5_v2::protocol::connack>(p.body());
    std::cout << ack.properties.properties_ref().size() << std::endl;
}

p0443_v2::immediate_task mqtt_task(net::io_context& io) {
    mqtt5_v2::connection<tcp::socket> connection(io);

    {
        tcp::resolver resolver(io);
        auto resolve_result = co_await p0443_v2::await_sender(p0443_v2::asio::resolve(resolver, "test.mosquitto.org", "1883"));
        auto connected_ep = co_await p0443_v2::await_sender(p0443_v2::asio::connect_socket(connection.next_layer(), resolve_result));
        std::cout <<  "Connected to " << connected_ep.address().to_string() << ":" << connected_ep.port() << "\n";
    }
    std::cout << co_await p0443_v2::await_sender(connection.control_packet_writer(mqtt5_v2::protocol::connect{})) << " bytes written\n";
    auto read_packet = co_await p0443_v2::await_sender(connection.control_packet_reader());
    auto &body = read_packet.body();
    test(read_packet);
    std::cout << "Packet read with index " << read_packet.body().index() << std::endl;
    //std::cout << "Received type " << int(read_packet.fixed_sized_header.type()) << " with " << read_packet.rest_of_data.size() << " bytes of data\n";
}

int main() {
    net::io_context io;
    mqtt_task(io);

    io.run();
}
