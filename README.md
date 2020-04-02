# MQTT5
This is a playground project to try out stuff from the upcoming executors proposal with a real-world usecase.

This project depends on [p0443](https://gitlab.com/AndWass/p0443) which is my extremely non-conforming and partial implementation of the [P0443R13](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r13.html) proposal with extra algorithms added.

To implement networking the p0443 project contains a few `p0443_v2::asio` senders that can be used with both regular TCP sockets and with `boost::beast::websocket::stream` streams.

At the moment only some basic MQTT5 frame parsing and serializing is available, but even that allows for some quick prototyping and playing around.

## TCP and coroutine sample code

```cpp
// The MQTT5 connection object to work with
mqtt5::connection<tcp::socket> connection(io);

// Resolve the host and port
auto resolve_result =
    co_await p0443_v2::await_sender(p0443_v2::asio::resolve(io, opt.host, opt.port));
// Connect a socket using the resolve results
auto connected_ep = co_await p0443_v2::await_sender(
    p0443_v2::asio::connect_socket(connection.next_layer(), resolve_result));

namespace prot = mqtt5::protocol;

// Create a connect packet and send it
prot::connect connect;
connect.flags = prot::connect::clean_start_flag;
co_await p0443_v2::await_sender(connection.control_packet_writer(connect));

// Wait for CONNACK and read it.
// body_as returns a std::optional<prot::connack>;
auto connack = (co_await p0443_v2::await_sender(connection.control_packet_reader()))
    .body_as<prot::connack>();

if (connack && connack->reason_code == 0) {
    std::cout << "Successfully connected to " << opt.host << ":" << opt.port << std::endl;
    // We didn't provide a client id, so we will be assigned one
    std::cout << "Assigned client identifier = " <<
        connack->properties.assigned_client_id << "\n";
    
    // Subscribe to a list of topics, maximum QoS=1
    prot::subscribe subscribe;
    subscribe.packet_identifier = 10;
    for (const auto &topic : opt.topics) {
        subscribe.topics.emplace_back(topic, 1);
    }
    // Send the control packet
    co_await p0443_v2::await_sender(connection.control_packet_writer(subscribe));
    // Expected response is a suback
    auto suback =
        co_await p0443_v2::await_sender(connection.packet_reader<prot::suback>());

    if (suback && suback->packet_identifier == 10) {
        // Check that the response codes are all valid.
        bool valid_codes = std::all_of(suback->reason_codes.begin(), suback->reason_codes.end(),
                                        [](auto c) { return c <= 1; });
        if (!suback->reason_codes.empty() && valid_codes) {
            std::cout << "Waiting for published messages'\n";
            // Read published messages and print it, ACK if necessary
            while (true) {
                if (auto publish = co_await p0443_v2::await_sender(
                        connection.packet_reader<prot::publish>());
                    publish) {

                    std::cout << "Received publish\n";
                    std::string data(publish->payload.begin(), publish->payload.end());
                    std::cout << "  '" << data << "'\n";
                    std::cout << "  [Topic = " << publish->topic
                                << ", QoS = " << (int)publish->quality_of_service() << "]\n";

                    if (publish->quality_of_service() == 1) {
                        prot::puback ack;
                        ack.packet_identifier = publish->packet_identifier;
                        ack.reason_code = 0;

                        std::cout << "  [PUBACK Packet identifier = " << ack.packet_identifier
                                    << "]\n";
                        co_await p0443_v2::await_sender(connection.control_packet_writer(ack));
                    }
                    else if (publish->quality_of_service() == 2) {
                        std::cout << "  !!Unsupported quality of service\n";
                    }
                }
            }
        }
    }
}
```

### WebSocket

Only a few lines have to be modified to support a WebSocket stream using `boost::beast`:

```cpp
// The MQTT5 connection object to work with
mqtt5::connection<tcp::socket> connection(io);

// Resolve the host and port
auto resolve_result =
    co_await p0443_v2::await_sender(p0443_v2::asio::resolve(io, opt.host, opt.port));
// Connect a socket using the resolve results
auto connected_ep = co_await p0443_v2::await_sender(
    p0443_v2::asio::connect_socket(connection.next_layer(), resolve_result));

// Code from here on will not be modified
namespace prot = mqtt5::protocol;

// Create a connect packet and send it
prot::connect connect;
connect.flags = prot::connect::clean_start_flag;
co_await p0443_v2::await_sender(connection.control_packet_writer(connect));
```

Becomes

```cpp
// Use a beast websocket stream instead
mqtt5::connection<ws::stream<boost::beast::tcp_stream>> connection(io);
// Resolve as before
auto resolve_result =
    co_await p0443_v2::await_sender(p0443_v2::asio::resolve(io, opt.host, opt.port));
// Connect the underlying socket of the WebSocket stream
auto connected_ep = co_await p0443_v2::await_sender(
    p0443_v2::asio::connect_socket(connection.next_layer().next_layer(), resolve_result));

// MQTT over WebSocket requires binary streams
connection.next_layer().binary(true);
// The client must include "mqtt" in the list of WebSocket Sub Protocols it offers
auto handshake_decorator = [](ws::request_type& req) {
    req.insert(http::field::sec_websocket_protocol, "mqtt");
};
connection.next_layer().set_option(ws::stream_base::decorator(handshake_decorator));
// Perform the WebSocket handshake
co_await p0443_v2::await_sender(p0443_v2::asio::handshake(connection.next_layer(), opt.host, opt.url));

std::cout << "WebSocket handshake complete\n";

// All remaining code is the same as before.
namespace prot = mqtt5::protocol;

// Create a connect packet and send it
prot::connect connect;
connect.flags = prot::connect::clean_start_flag;
co_await p0443_v2::await_sender(connection.control_packet_writer(connect));
```