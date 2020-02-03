#include <iostream>

#include <mqtt5/connection.hpp>
#include <mqtt5/stream.hpp>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>

#include <thread>

namespace asio = boost::asio;

int main() {
    asio::io_context io;
    
    asio::ip::tcp::resolver resolver(io);
    asio::deadline_timer ping_timer(io);
    auto resolved = resolver.resolve("test.mosquitto.org", "1883");

    using connection_type = mqtt5::connection<asio::ip::tcp::socket>;
    using stream_type = mqtt5::stream<asio::ip::tcp::socket>;

    stream_type stream(io);
    stream.lowest_layer().connect(*resolved);
    mqtt5::message::connect connect;
    mqtt5::message::raw response;
    stream.write(connect);
    stream.read(response);
    std::cout << (int)response.type << std::endl;
    /*connection_type connection(io);
    connection.will = mqtt5::publish{};
    connection.will->topic_name = "/andreastest";
    connection.will->set_payload("This is a last will");
    connection.lowest_layer().connect(*resolved);

    struct ping_handler {
        asio::deadline_timer *timer;
        connection_type *conn;
        void operator()(const boost::system::error_code &ec) {
            if(!ec) {
                std::cout << "PING\n";
                mqtt5::message::raw pingreq;
                pingreq.type = mqtt5::packet_type::pingreq;
                conn->write(std::move(pingreq), ping_handler{timer, conn});
                //conn->ping_request(ping_handler{timer, conn});
            }
            else {
                std::cout << "Timer error: " << ec.message() << "\n";
            }
        }

        void set_done() {
            timer->expires_from_now(boost::posix_time::seconds{15});
            timer->async_wait(ping_handler{timer, conn});
        }
        void set_error(const boost::system::error_code &ec) {
            std::cout << "PING ERROR " << ec.message() << "\n";
        }
    };

    struct read_receiver_type {
        connection_type *conn;
        asio::deadline_timer *timer;
        void set_value(const mqtt5::message::raw &packet) {
            std::cout << "Received packet " << (int)packet.type << "\n";
            conn->read(read_receiver_type{conn, timer});
        }
        void set_error(const boost::system::error_code &ec) {
            std::cout << "READ ERROR: " << ec.message() << "\n";
        }
    };

    auto handshake_receiver = mqtt5::on_done_or_error([&]() {
        std::cout << "Handshake done\n";
        connection.read(read_receiver_type{&connection, &ping_timer});
        ping_handler pinger{&ping_timer, &connection};
        pinger.set_done();
    },
    [](const auto &ec){
        std::cout << ec.message() << "\n";
    });

    connection.handshake(handshake_receiver);*/

    io.run();
}