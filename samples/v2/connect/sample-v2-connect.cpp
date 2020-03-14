#include <mqtt5_v2/protocol/control_packet.hpp>
#include <mqtt5_v2/protocol/connect.hpp>

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
    tcp::socket socket(io);
    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> read_buffer;

    mqtt5_v2::protocol::control_packet packet_buffer;
    mqtt5_v2::transport::data_fetcher<tcp::socket> fetcher(socket, read_buffer);

    {
    tcp::resolver resolver(io);
    auto resolve_result = co_await p0443_v2::await_sender(p0443_v2::asio::resolve(resolver, "test.mosquitto.org", "1883"));
    auto connected_ep = co_await p0443_v2::await_sender(p0443_v2::asio::connect_socket(socket, resolve_result));
    std::cout <<  "Connected to " << connected_ep.address().to_string() << ":" << connected_ep.port() << "\n";
    }
    packet_buffer = mqtt5_v2::protocol::connect{};
    std::vector<std::uint8_t> write_buffer;
    packet_buffer.serialize([&](auto b) {
        write_buffer.push_back(b);
    });
    co_await p0443_v2::await_sender(p0443_v2::asio::write_all(socket, net::buffer(write_buffer)));
    co_await p0443_v2::await_sender(packet_buffer.inplace_deserializer(fetcher));
    std::cout << "Received type " << int(packet_buffer.fixed_sized_header.type()) << " with " << packet_buffer.rest_of_data.size() << " bytes of data\n";
}

int main() {
    net::io_context io;
    mqtt_task(io);

    io.run();
}