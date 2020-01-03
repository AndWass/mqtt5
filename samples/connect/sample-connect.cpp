#include <iostream>

#include <mqtt5/connection.hpp>
#include <boost/asio.hpp>

#include <thread>

namespace asio = boost::asio;

int main() {
    asio::io_context io;
    asio::ip::tcp::resolver resolver(io);
    auto resolved = resolver.resolve("test.mosquitto.org", "1883");

    mqtt5::connection<asio::ip::tcp::socket> connection(io);
    connection.will = mqtt5::publish{};
    connection.will->topic_name = "/andreastest";
    connection.will->set_payload("This is a last will");
    connection.will_delay = std::chrono::seconds{30};
    connection.lowest_layer().connect(*resolved);
    connection.handshake();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    /*mqtt5::connect connect_packet;
    connect_packet.client_id = connect_packet.generate_client_id();
    mqtt5::binary_packet bp;
    bp = connect_packet;

    auto bytes = bp.to_bytes();

    std::cout << "Sending connect packet with client id " << connect_packet.client_id.to_string() << std::endl;

    
    asio::ip::tcp::socket socket(io);
    //asio::ip::tcp::endpoint ep()

    socket.connect(*resolved);
    asio::write(socket, asio::buffer(bytes));
    bytes.clear();
    bytes.resize(10);
    auto amount_read = socket.read_some(asio::buffer(bytes));
    bytes.resize(amount_read);
    //auto amount_read = asio::read(socket, asio::buffer(bytes));
    std::cout << "Read " << amount_read << " bytes" << std::endl;*/
}