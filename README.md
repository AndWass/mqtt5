# MQTT5
This is a playground project to try out stuff from the upcoming executors proposal with a real-world usecase.

The main repository is found on [GitLab](https://gitlab.com/AndWass/mqtt5) but a [GitHub](https://github.com/AndWass/mqtt5) repository is also available.

This project depends on [p0443](https://gitlab.com/AndWass/p0443) which is my extremely non-conforming and partial implementation of the [P0443R13](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r13.html) proposal with extra algorithms added.

To implement networking the p0443 project contains a few `p0443_v2::asio` senders that can be used with both regular TCP sockets and with `boost::beast::websocket::stream` streams.

At the moment only some basic MQTT5 frame parsing and serializing is available, but even that allows for some quick prototyping and playing around. A higher-level client implementation is currently in progress.

## Client coroutine sample code

```cpp
// immediate_task is a coroutine "task"-like that always
// returns void, but will not be suspended on initial call
p0443_v2::immediate_task run_tcp_client(mqtt5::client<tcp::socket>& client)
{
    mqtt5::connect_options opts;
    boost::string_view hostname = "mqtt.eclipse.org";
    boost::string_view port = "1883";
    opts.keep_alive = std::chrono::minutes{1};
    opts.last_will.emplace();
    opts.last_will->topic = "mqtt5/last_will";
    opts.last_will->set_payload("last will from TCP client");
    opts.last_will->delay_interval = std::chrono::seconds{10};
    opts.last_will->quality_of_service = 1_qos;

    co_await client.connect_socket(hostname, port);
    std::cout << "Socket connected...\n";
    co_await client.handshake(opts);
    std::cout << "Handshake complete...\n";

    // A normal publisher can only be used once, either by co_await
    // or by p0443_v2::connect and start, or submit.
    co_await client.publisher("mqtt5/hello_world", "Hello world!");

    int packet_number = 1;
    // A reusable_publisher can be used multiple times.
    // The callable only modifies the currently to be sent
    // message. The "base" message stays the same between usages.
    auto publisher = client.reusable_publisher(
        "mqtt5/tcp_client", "hello world from TCP client! ", 1_qos,
        [&packet_number](mqtt5::protocol::publish &pub) mutable {
            pub.properties.topic_alias = 1;
            if (packet_number > 1) {
                pub.topic.clear();
            }
            auto packet_nr_string = std::to_string(packet_number);
            packet_number++;
            pub.payload.insert(pub.payload.end(), packet_nr_string.begin(), packet_nr_string.end());
        });

    // Publish
    // "hello world from TCP client! 1"
    // "hello world from TCP client! 2"
    // etc...
    for (int i = 0; i < 5; i++) {
        co_await publisher;
    }
    std::cout << "All messages published!\n";
}
```

## Low layer coroutine sample code

The code below is taken from the complete [subscribe sample](https://gitlab.com/AndWass/mqtt5/-/blob/master/samples/subscribe/sample-subscribe.cpp).

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

Only a few lines have to be modified to support a WebSocket stream using `boost::beast`. Subscribe examples for [TCP](https://gitlab.com/AndWass/mqtt5/-/blob/master/samples/subscribe/sample-subscribe.cpp)
and [WebSockets](https://gitlab.com/AndWass/mqtt5/-/blob/master/samples/subscribe/sample-subscribe-ws.cpp) are available.

The connection setup code for TCP socket is

```cpp
// The MQTT5 connection object to work with
mqtt5::connection<tcp::socket> connection(io);
// Resolve host and port
auto resolve_result =
    co_await p0443_v2::await_sender(p0443_v2::asio::resolve(io, opt.host, opt.port));
// Connect the TCP socket
co_await p0443_v2::await_sender(
    p0443_v2::asio::connect_socket(connection.next_layer(), resolve_result));
// Use
namespace prot = mqtt5::protocol;
```

With websockets this becomes

```cpp
mqtt5::connection<ws::stream<boost::beast::tcp_stream>> connection(io);
// Need to set binary stream
connection.next_layer().binary(true);
// The Client MUST include "mqtt" in the list of WebSocket Sub Protocols it offers
auto handshake_decorator = [](ws::request_type& req) {
    req.insert(http::field::sec_websocket_protocol, "mqtt");
};
connection.next_layer().set_option(ws::stream_base::decorator(handshake_decorator));

// Resolve just like before
auto resolve_result =
    co_await p0443_v2::await_sender(p0443_v2::asio::resolve(io, opt.host, opt.port));
// Connect and handshake in a sequence
co_await p0443_v2::await_sender(
    p0443_v2::sequence(
        p0443_v2::asio::connect_socket(connection.next_layer().next_layer(), resolve_result),
        p0443_v2::asio::handshake(connection.next_layer(), opt.host, opt.url)
));
// From here on the code is exactly the same
namespace prot = mqtt5::protocol;
```

Everything else stays exactly the same.