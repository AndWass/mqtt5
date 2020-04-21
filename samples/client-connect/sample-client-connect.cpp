#include "mqtt5/connect_options.hpp"
#include "mqtt5/protocol/publish.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream_base.hpp>
#include <chrono>
#include <mqtt5/client.hpp>
#include <mqtt5/publish_options.hpp>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <p0443_v2/asio/handshake.hpp>
#include <p0443_v2/await_sender.hpp>
#include <p0443_v2/immediate_task.hpp>

#include <iostream>

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace ws = boost::beast::websocket;

using namespace mqtt5::literals;

p0443_v2::immediate_task run_tcp_client(mqtt5::client<tcp::socket> &client) {
    net::steady_timer wait_timer(client.get_executor());
    co_await p0443_v2::asio::timer::wait_for(wait_timer, std::chrono::seconds{5});
    mqtt5::connect_options opts;
    boost::string_view hostname = "mqtt.eclipse.org";
    boost::string_view port = "1883";
    opts.keep_alive = std::chrono::seconds{43};
    opts.last_will.emplace();
    opts.last_will->topic = "mqtt5/last_will";
    opts.last_will->set_payload("last will from TCP client");
    opts.last_will->delay_interval = std::chrono::seconds{10};
    opts.last_will->quality_of_service = 1_qos;
    opts.last_will->content_type = "application/json";

    co_await client.socket_connector(hostname, port);
    co_await client.handshaker(opts);

    namespace pubopt = mqtt5::publish_options;
    // A normal publisher can only be used once, either by co_await
    // or by p0443_v2::connect and start, or submit.
    co_await client.publisher("log/debug", "Hello world!", 1_qos,
                              mqtt5::payload_format_indicator::utf8);

    int packet_number = 1;
    // A reusable_publisher can be used multiple times.
    auto publisher = client.reusable_publisher(
        "mqtt5/telemetry/hello_world", "hello world from TCP client! ", 1_qos, pubopt::topic_alias(1),
        [&packet_number](mqtt5::protocol::publish &pub) mutable {
            if (packet_number > 1) {
                pub.topic.clear();
            }
            auto packet_nr_string = std::to_string(packet_number);
            packet_number++;
            pub.payload.insert(pub.payload.end(), packet_nr_string.begin(), packet_nr_string.end());
        });

    for (int i = 0; i < 5; i++) {
        co_await publisher;
    }

    co_await client.publisher("mqtt5/messages", "TCP client done");
    client.close();
}

void print_publish(const mqtt5::protocol::publish &publish) {
    std::string payload(publish.payload.begin(), publish.payload.end());
    std::cout << payload << "\n";
    std::cout << "  [ Topic = " << publish.topic << "\n";
    std::cout << "    QoS = " << static_cast<int>(publish.quality_of_service())
              << " Packet ID = " << publish.packet_identifier << " ]\n";
}

p0443_v2::immediate_task
read_telemetry(mqtt5::client<ws::stream<boost::beast::tcp_stream>> &client) {
    while(true) {
        auto publish = co_await client.filtered_subscriber("mqtt5/telemetry/+");
        std::cout << "Telemetry: \n";
        print_publish(publish);
    }
}

p0443_v2::immediate_task
read_messages(mqtt5::client<ws::stream<boost::beast::tcp_stream>> &client) {
    while(true) {
        auto publish = co_await client.filtered_subscriber("mqtt5/messages");
        std::cout << "Message: \n";
        print_publish(publish);
    }
}

p0443_v2::immediate_task
read_debug(mqtt5::client<ws::stream<boost::beast::tcp_stream>> &client) {
    while(true) {
        auto publish = co_await client.filtered_subscriber("+/debug");
        std::cout << "Debug: \n";
        print_publish(publish);
    }
}


p0443_v2::immediate_task
run_websocket_client(mqtt5::client<ws::stream<boost::beast::tcp_stream>> &client) {
    mqtt5::connect_options opts;
    boost::string_view hostname = "mqtt.eclipse.org";
    boost::string_view port = "80";
    opts.keep_alive = std::chrono::seconds{60};
    opts.last_will.emplace();
    opts.last_will->topic = "mqtt5/last_will";
    opts.last_will->set_payload("last will from WebSocket client");
    opts.last_will->delay_interval = std::chrono::seconds{10};
    opts.last_will->quality_of_service = 1_qos;
    opts.last_will->content_type = "application/json";

    client.get_nth_layer<1>().binary(true);
    client.get_nth_layer<1>().set_option(ws::stream_base::decorator([](ws::request_type &request) {
        request.set(boost::beast::http::field::sec_websocket_protocol, "mqtt");
    }));

    co_await client.socket_connector(hostname, port);
    co_await p0443_v2::asio::handshake(client.get_nth_layer<1>(), hostname, "/mqtt");
    co_await client.handshaker(opts);

    namespace pubopt = mqtt5::publish_options;

    co_await client.subscriber("mqtt5/#", 1_qos);
    co_await client.subscriber("log/#", 1_qos);
    read_telemetry(client);
    read_messages(client);
    read_debug(client);
}

int main() {
    net::io_context io;
    mqtt5::client<tcp::socket> tcp_client(io.get_executor());
    mqtt5::client<ws::stream<boost::beast::tcp_stream>> ws_client(io.get_executor());
    run_tcp_client(tcp_client);
    run_websocket_client(ws_client);
    io.run();
}