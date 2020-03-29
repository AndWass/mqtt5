#include <mqtt5_v2/connection.hpp>
#include <mqtt5_v2/protocol/control_packet.hpp>

#include <p0443_v2/asio/connect.hpp>
#include <p0443_v2/asio/resolve.hpp>
#include <p0443_v2/asio/write_all.hpp>
#include <p0443_v2/await_sender.hpp>
#include <p0443_v2/immediate_task.hpp>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <iostream>

namespace net = boost::asio;
namespace ip = net::ip;
using tcp = ip::tcp;

struct options
{
    std::string host;
    std::string port;
    std::string topic;
    std::string message;
    std::uint8_t quality_of_service;

    bool valid() const {
        return !host.empty() && !port.empty() && !topic.empty() && !message.empty() &&
               quality_of_service < 3;
    }
};

p0443_v2::immediate_task mqtt_task(net::io_context &io, options opt) {
    mqtt5_v2::connection<tcp::socket> connection(io);

    auto resolve_result =
        co_await p0443_v2::await_sender(p0443_v2::asio::resolve(io, opt.host, opt.port));
    auto connected_ep = co_await p0443_v2::await_sender(
        p0443_v2::asio::connect_socket(connection.next_layer(), resolve_result));

    using connect_packet = mqtt5_v2::protocol::connect;

    mqtt5_v2::protocol::connect connect;
    connect.flags = connect_packet::clean_start_flag;
    co_await p0443_v2::await_sender(connection.control_packet_writer(connect));
    auto connack = (co_await p0443_v2::await_sender(connection.control_packet_reader()))
                       .body_as<mqtt5_v2::protocol::connack>();

    if (connack && connack->reason_code == 0) {
        std::cout << "Successfully connected to " << opt.host << ":" << opt.port << std::endl;
        mqtt5_v2::protocol::publish publish;
        publish.set_quality_of_service(opt.quality_of_service);
        if (opt.quality_of_service > 0) {
            publish.packet_identifier = 1;
        }
        publish.topic = opt.topic;
        publish.set_payload(opt.message);
        co_await p0443_v2::await_sender(connection.control_packet_writer(publish));

        std::cout << "Message sent to " << opt.topic
                  << " with QoS = " << (int)opt.quality_of_service << "\n";

        if (opt.quality_of_service > 0) {
            auto puback = co_await p0443_v2::await_sender(
                connection.packet_reader<mqtt5_v2::protocol::puback>());

            using puback_t = mqtt5_v2::protocol::puback;
            if (puback) {
                if (puback->packet_identifier == 1) {
                    if (puback->reason_code == puback_t::success ||
                        puback->reason_code == puback_t::no_matching_subscribers) {
                        std::cout << "Message delivered\n";
                    }
                    else {
                        std::cout << "Error delivering message. Reason code = "
                                  << (int)puback->reason_code << "\n";
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
        "Broker hostname")("port", po::value<std::string>()->default_value("1883"),
                           "Broker port")("topic", po::value<std::string>(), "Topic to publish to")(
        "message", po::value<std::string>(),
        "Message to publish")("qos", po::value<int>()->default_value(0), "Quality of service");

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
        retval.topic = vm["topic"].as<std::string>();
    }
    if (vm.count("message")) {
        retval.message = vm["message"].as<std::string>();
    }
    if (vm.count("qos")) {
        retval.quality_of_service = vm["qos"].as<int>();
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
