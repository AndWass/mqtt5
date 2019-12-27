#include <iostream>

#include <mqtt5/connect.hpp>
#include <mqtt5/binary_packet.hpp>

#include <boost/asio.hpp>
namespace asio = boost::asio;

int main() {
    mqtt5::connect connect_packet;
    connect_packet.client_id = connect_packet.generate_client_id();
    mqtt5::binary_packet bp;
    bp = connect_packet;

    auto bytes = bp.to_bytes();

    std::cout << "Sending connect packet with client id " << connect_packet.client_id.to_string() << std::endl;

    asio::io_context io;
    asio::ip::tcp::resolver resolver(io);
    auto resolved = resolver.resolve("test.mosquitto.org", "1883");
    asio::ip::tcp::socket socket(io);
    //asio::ip::tcp::endpoint ep()
    socket.connect(*resolved);
    asio::write(socket, asio::buffer(bytes));
    bytes.clear();
    bytes.resize(10);
    auto amount_read = socket.read_some(asio::buffer(bytes));
    bytes.resize(amount_read);
    //auto amount_read = asio::read(socket, asio::buffer(bytes));
    std::cout << "Read " << amount_read << " bytes" << std::endl;
}