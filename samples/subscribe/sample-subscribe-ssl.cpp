
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <boost/asio/ssl/rfc2818_verification.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/_experimental/test/stream.hpp>
#include <mqtt5/connection.hpp>
#include <mqtt5/protocol/control_packet.hpp>

#include <p0443_v2/asio/connect.hpp>
#include <p0443_v2/asio/handshake.hpp>
#include <p0443_v2/asio/resolve.hpp>
#include <p0443_v2/await_sender.hpp>
#include <p0443_v2/immediate_task.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/program_options.hpp>

#include <iostream>

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;

using namespace mqtt5::literals;

struct options
{
    std::string host;
    std::string port;
    std::string ca_file;
    std::vector<std::string> topics;

    bool valid() const {
        return !host.empty() && !port.empty() && !topics.empty();
    }
};

p0443_v2::immediate_task mqtt_task(net::io_context &io, options opt) {
    namespace ssl = boost::asio::ssl;
    ssl::context ctx(ssl::context::sslv23);
    ctx.set_default_verify_paths();
    if (!opt.ca_file.empty()) {
        ctx.load_verify_file(opt.ca_file);
    }
    mqtt5::connection<ssl::stream<tcp::socket>> connection(io, ctx);
    connection.next_layer().set_verify_callback(ssl::rfc2818_verification(opt.host));
    connection.next_layer().set_verify_mode(ssl::verify_peer);

    auto resolve_result =
        co_await p0443_v2::await_sender(p0443_v2::asio::resolve(io, opt.host, opt.port));
    co_await p0443_v2::await_sender(p0443_v2::sequence(
        p0443_v2::asio::connect_socket(connection.next_layer().next_layer(), resolve_result),
        p0443_v2::asio::handshake(connection.next_layer(), ssl::stream<tcp::socket>::client)));

    namespace prot = mqtt5::protocol;

    prot::connect connect;
    connect.flags = prot::connect::clean_start_flag;
    co_await p0443_v2::await_sender(connection.control_packet_writer(connect));
    auto connack = (co_await p0443_v2::await_sender(connection.control_packet_reader()))
                       .body_as<prot::connack>();

    if (connack && connack->reason_code == 0) {
        std::cout << "Successfully connected to " << opt.host << ":" << opt.port << std::endl;
        std::cout << "Assigned client identifier = " << connack->properties.assigned_client_id
                  << "\n";
        prot::subscribe subscribe;
        subscribe.packet_identifier = 10;
        for (const auto &topic : opt.topics) {
            subscribe.topics.emplace_back(topic, 1);
        }
        co_await p0443_v2::await_sender(connection.control_packet_writer(subscribe));

        auto suback = co_await p0443_v2::await_sender(connection.packet_reader<prot::suback>());

        if (suback && suback->packet_identifier == 10) {
            bool valid_codes = std::all_of(suback->reason_codes.begin(), suback->reason_codes.end(),
                                           [](auto c) { return c <= 1; });
            if (!suback->reason_codes.empty() && valid_codes) {
                std::cout << "Waiting for published messages'\n";
                while (true) {
                    if (auto publish = co_await p0443_v2::await_sender(
                            connection.packet_reader<prot::publish>());
                        publish) {

                        std::cout << "Received publish\n";
                        std::string data(publish->payload.begin(), publish->payload.end());
                        std::cout << "  '" << data << "'\n";
                        std::cout << "  [Topic = " << publish->topic
                                  << ", QoS = " << (int)publish->quality_of_service() << "]\n";

                        if (publish->quality_of_service() == 1_qos) {
                            prot::puback ack;
                            ack.packet_identifier = publish->packet_identifier;
                            ack.reason_code = mqtt5::puback_reason_code::success;

                            std::cout << "  [PUBACK Packet identifier = " << ack.packet_identifier
                                      << "]\n";
                            co_await p0443_v2::await_sender(connection.control_packet_writer(ack));
                        }
                        else if (publish->quality_of_service() == 2_qos) {
                            std::cout << "  !!Unsupported quality of service\n";
                        }
                    }
                }
            }
        }
    }
    else if (connack) {
        std::cout << "Received unsuccessful connack, reason code = " << (int)connack->reason_code
                  << "\n";
    }
    else {
        std::cout << "Received unexpected packet\n";
    }
}

options parse_options(int argc, char **argv) {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()("help,h", "Print help")(
        "host", po::value<std::string>()->default_value("mqtt.eclipse.org"),
        "Broker hostname")("port", po::value<std::string>()->default_value("8883"), "Broker port")(
        "topic", po::value<std::vector<std::string>>(), "Topics to subscribe to")(
        "cafile", po::value<std::string>(), "PEM file with certificates to verify against");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return {};
    }

    options retval;
    if (vm.count("host")) {
        retval.host = vm["host"].as<std::string>();
    }
    if (vm.count("port")) {
        retval.port = vm["port"].as<std::string>();
    }
    if (vm.count("topic")) {
        retval.topics = vm["topic"].as<std::vector<std::string>>();
    }
    if (vm.count("cafile")) {
        retval.ca_file = vm["cafile"].as<std::string>();
    }
    if (!retval.valid()) {
        std::cout << desc << "\n";
        return {};
    }

    return retval;
}

int main(int argc, char **argv) {
    auto options = parse_options(argc, argv);
    if (!options.valid()) {
        return 1;
    }
    net::io_context io;
    mqtt_task(io, options);

    io.run();
}
