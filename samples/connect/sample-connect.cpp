
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <mqtt5/connection.hpp>
#include <mqtt5/protocol/control_packet.hpp>

#include <p0443_v2/asio/connect.hpp>
#include <p0443_v2/asio/resolve.hpp>
#include <p0443_v2/asio/write_all.hpp>
#include <p0443_v2/await_sender.hpp>
#include <p0443_v2/immediate_task.hpp>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include <iostream>

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;

struct options
{
    std::string host;
    std::string port = "1883";

    bool valid() const {
        return !host.empty() && !port.empty();
    }
};

p0443_v2::immediate_task mqtt_task(net::io_context &io, options opt) {
    mqtt5::connection<tcp::socket> connection(io);

    auto resolve_result = co_await p0443_v2::await_sender(
        p0443_v2::asio::resolve(io, opt.host, opt.port));
    auto connected_ep = co_await p0443_v2::await_sender(
        p0443_v2::asio::connect_socket(connection.next_layer(), resolve_result));

    using connect_packet = mqtt5::protocol::connect;

    mqtt5::protocol::connect connect;
    connect.flags = connect_packet::clean_start_flag | connect_packet::will_flag |
                    connect_packet::will_qos_1;

    connect.will_topic = "/mqtt5/lastwill";
    connect.set_will_payload("This is my last will and testament");
    connect.will_properties.emplace();

    co_await p0443_v2::await_sender(connection.control_packet_writer(connect));

    auto read_packet = co_await p0443_v2::await_sender(connection.control_packet_reader());
    auto *connack = read_packet.body_as<mqtt5::protocol::connack>();

    if (connack && connack->reason_code == 0) {
        std::cout << "Successfully connected to " << opt.host << ":" << opt.port << std::endl;
        std::cout << "Assigned client id = " << connack->properties.assigned_client_id << std::endl;
        mqtt5::protocol::publish publish;
        publish.set_quality_of_service(1);
        publish.packet_identifier = 1;
        publish.topic = "/mqtt5/some_topic";
        publish.set_payload("This is published from mqtt5 using c++ coroutines!");

        std::cout << "Published "
                  << co_await p0443_v2::await_sender(connection.control_packet_writer(publish))
                  << " bytes\n";
        read_packet = co_await p0443_v2::await_sender(connection.control_packet_reader());
        auto *maybe_ack = read_packet.body_as<mqtt5::protocol::puback>();
        if (maybe_ack) {
            std::cout << "Received ACK(" << (int)maybe_ack->reason_code << ") for publish packet "
                      << maybe_ack->packet_identifier << "\n";
        }
        else {
            std::cout << "Did not receive an ACK!\n";
        }
    }
    else if (connack) {
        std::cout << "Received unsuccessful connack, reason code = " << (int)connack->reason_code
                  << "\n";
    }
    else {
        std::cout << "Received unexpected packet\n";
    }
    std::cout << std::endl;
}

options parse_options(int argc, char**argv) {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()("help,h", "Print help")
    ("host", po::value<std::string>()->default_value("mqtt.eclipse.org"), "Broker hostname")
    ("port", po::value<std::string>()->default_value("1883"), "Broker port");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("help")) {
        std::cout << desc << "\n";
        return {};
    }

    options retval;
    if(vm.count("host")) {
        retval.host = vm["host"].as<std::string>();
    }
    if(vm.count("port")) {
        retval.port = vm["port"].as<std::string>();
    }
    if(!retval.valid()) {
        std::cout << desc << "\n";
        return {};
    }

    return retval;
}

int main(int argc, char** argv) {
    auto options = parse_options(argc, argv);
    if(!options.valid()) {
        return 1;
    }
    net::io_context io;
    mqtt_task(io, options);

    io.run();
}
