#include "mqtt5/connect_options.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <experimental/coroutine>
#include <mqtt5/client.hpp>

#include <boost/asio.hpp>

#include <mqtt5/coroutine.hpp>
#include <p0443_v2/immediate_task.hpp>

namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

p0443_v2::immediate_task client_task(net::io_context& io)
{
    mqtt5::connect_options opts;
    opts.hostname = "mqtt.eclipse.org";
    opts.port = "1883";
    opts.keep_alive = std::chrono::seconds{10};
    opts.last_will.emplace();
    opts.last_will->topic = "mqtt5/last_will";
    opts.last_will->set_payload("{}");
    opts.last_will->delay_interval = std::chrono::seconds{10};
    opts.last_will->quality_of_service = 1;
    opts.last_will->content_type = "application/json";

    mqtt5::client<tcp::socket> client(io.get_executor());
    co_await client.connector(opts);
    if(client.is_connected()) {
        std::cout  << "Connected!\n";
    }
    auto result = co_await client.publish("mqtt5/test_publish", "hello world!", 1);
    std::cout << "Message published with code " << (int)result << "\n";
    co_await p0443_v2::stdcoro::suspend_always{};
}

int main() {
    net::io_context io;
    client_task(io);
    io.run();
}