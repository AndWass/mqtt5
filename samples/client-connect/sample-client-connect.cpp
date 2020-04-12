#define BOOST_BEAST_USE_STD_STRING_VIEW 1

#include "mqtt5/connect_options.hpp"
#include "mqtt5/protocol/publish.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream_base.hpp>
#include <chrono>
#include <experimental/coroutine>
#include <mqtt5/client.hpp>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include <p0443_v2/await_sender.hpp>
#include <p0443_v2/asio/handshake.hpp>
#include <p0443_v2/immediate_task.hpp>

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

template<class T>
void inspect(const T& t) {
    std::cout << "Inspect!\n";
}

p0443_v2::immediate_task tcp_client(net::io_context& io)
{
    mqtt5::connect_options opts;
    std::string_view hostname = "mqtt.eclipse.org";
    std::string_view port = "1883";
    opts.keep_alive = std::chrono::seconds{43};
    opts.last_will.emplace();
    opts.last_will->topic = "mqtt5/last_will";
    opts.last_will->set_payload("last will from TCP client");
    opts.last_will->delay_interval = std::chrono::seconds{10};
    opts.last_will->quality_of_service = 1;
    opts.last_will->content_type = "application/json";

    mqtt5::client<tcp::socket> client(io.get_executor());
    co_await client.connect_socket(hostname, port);
    std::cout << "Socket connected...\n";
    co_await client.handshake(opts);
    if(client.is_connected()) {
        std::cout  << "Connected!\n";
    }

    int packet_number = 1;
    auto publisher = client.reusable_publisher("mqtt5/tcp_client", "hello world from TCP client! ", 1, [&packet_number](mqtt5::protocol::publish& pub) {
        pub.properties.topic_alias = 1;
        if(packet_number > 1) {
            pub.topic.clear();
        }
        auto packet_nr_string = std::to_string(packet_number);
        pub.payload.insert(pub.payload.end(), packet_nr_string.begin(), packet_nr_string.end());
    });

    for(int i=0; i<5; i++)
    {
        co_await publisher;
        packet_number++;
    }

    co_await p0443_v2::stdcoro::suspend_always{};
}

p0443_v2::immediate_task websocket_client(net::io_context& io)
{
    mqtt5::connect_options opts;
    std::string_view hostname = "mqtt.eclipse.org";
    std::string_view port = "80";
    opts.keep_alive = std::chrono::seconds{60};
    opts.last_will.emplace();
    opts.last_will->topic = "mqtt5/last_will";
    opts.last_will->set_payload("last will from WebSocket client");
    opts.last_will->delay_interval = std::chrono::seconds{10};
    opts.last_will->quality_of_service = 1;
    opts.last_will->content_type = "application/json";

    namespace ws = boost::beast::websocket;

    mqtt5::client<ws::stream<boost::beast::tcp_stream>> client(io.get_executor());
    client.get_nth_layer<1>().binary(true);
    client.get_nth_layer<1>().set_option(ws::stream_base::decorator([](ws::request_type& request){
        request.set(boost::beast::http::field::sec_websocket_protocol, "mqtt");
    }));

    co_await client.connect_socket(hostname, port);
    co_await p0443_v2::asio::handshake(client.get_nth_layer<1>(), hostname, "/mqtt");
    co_await client.handshake(opts);

    if(client.is_connected()) {
        std::cout  << "Connected!\n";
    }
    auto result = co_await client.publisher("mqtt5/websocket_client", "hello world from WebSocket client!", 1, [](mqtt5::protocol::publish& pub) {
        pub.properties.topic_alias = 1;
    });
    std::cout << "Message published with code " << (int)result << "\n";
    result = co_await client.publisher("", "Published using topic alias from WebSocket client!", 1, [](mqtt5::protocol::publish& pub) {
        pub.properties.topic_alias = 1;
    });

    co_await p0443_v2::stdcoro::suspend_always{};
}

int main() {
    net::io_context io;
    tcp_client(io);
    websocket_client(io);
    io.run();
}