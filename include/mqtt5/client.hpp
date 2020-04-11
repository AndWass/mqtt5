
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "connection.hpp"
#include "mqtt5/connect_options.hpp"
#include "mqtt5/protocol/connect.hpp"
#include "mqtt5/protocol/ping.hpp"
#include "mqtt5/protocol/publish.hpp"
#include "mqtt5/puback_reason_code.hpp"
#include "protocol/control_packet.hpp"

#include <boost/asio/executor.hpp>
#include <boost/sml.hpp>
#include <chrono>
#include <exception>
#include <memory>
#include <p0443_v2/asio/connect.hpp>
#include <p0443_v2/asio/resolve.hpp>
#include <p0443_v2/asio/timer.hpp>
#include <p0443_v2/submit.hpp>

#include <boost/asio/steady_timer.hpp>

#include <iostream>
#include <p0443_v2/type_traits.hpp>

namespace mqtt5
{
namespace helper
{
std::ostream &steady_now(std::ostream &os) {
    os << std::chrono::duration_cast<std::chrono::seconds>(
              std::chrono::steady_clock::now().time_since_epoch())
              .count()
       << " ";
    return os;
}
} // namespace helper
namespace net = boost::asio;

template <class Stream>
class client
{
private:
    struct packet_identifier_generator_t
    {
        std::uint16_t next_ = 1;
        std::uint16_t operator()() {
            auto retval = next_;
            next_++;
            if (next_ == 0) {
                next_ = 1;
            }
            return retval;
        }
    } next_packet_identifier;
    struct connect_sender;
    struct subscriber_token;

    struct publish_receiver_base
    {
        virtual void set_value(mqtt5::puback_reason_code code) = 0;
        virtual void set_done() = 0;
        virtual void set_error(std::exception_ptr e) = 0;

        virtual ~publish_receiver_base() = default;
    };
    struct in_flight_publish
    {
        protocol::publish message_;
        std::unique_ptr<publish_receiver_base> receiver_;
    };

    template <class Modifier>
    struct publisher
    {
        template <template <class...> class Tuple, template <class...> class Variant>
        using value_types = Variant<Tuple<mqtt5::puback_reason_code>>;

        template <template <class...> class Variant>
        using error_types = Variant<std::exception_ptr>;

        static constexpr bool sends_done = true;

        std::vector<std::uint8_t> payload;
        std::string topic_;
        Modifier modifying_function_;
        client *client_;
        std::uint8_t qos_ = 0;

        publisher(Modifier modifier) : modifying_function_(std::move(modifier)) {
        }

        template <class Receiver>
        struct operation
        {
            Receiver receiver_;

            std::string topic_;
            std::vector<std::uint8_t> payload;
            Modifier modifying_function_;
            client *client_;
            std::uint8_t qos_;

            struct publish_receiver : publish_receiver_base
            {
                Receiver next_;

                publish_receiver(Receiver &&next) : next_(std::move(next)) {
                }

                void set_value(mqtt5::puback_reason_code code) override {
                    p0443_v2::set_value(std::move(next_), code);
                }

                void set_done() override {
                    p0443_v2::set_done(std::move(next_));
                }

                void set_error(std::exception_ptr ex) override {
                    p0443_v2::set_error(std::move(next_), std::move(ex));
                }
            };

            void start() {
                protocol::publish publish;
                publish.topic = std::move(topic_);
                publish.payload = std::move(payload);
                publish.set_quality_of_service(qos_);
                modifying_function_(publish);

                if (publish.quality_of_service() > 0) {
                    publish.packet_identifier = client_->next_packet_identifier();
                }

                client_->send_message(publish);

                if (publish.quality_of_service() > 0) {
                    // Store message for further processing
                    in_flight_publish stored{std::move(publish), std::make_unique<publish_receiver>(
                                                                     std::move(receiver_))};

                    client_->published_messages_.emplace_back(std::move(stored));
                }
                else {
                    p0443_v2::set_value(std::move(receiver_), puback_reason_code::success);
                }
            }
        };

        template <class Receiver>
        auto connect(Receiver &&receiver) {
            return operation<p0443_v2::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver),
                                                                 topic_,
                                                                 payload,
                                                                 modifying_function_,
                                                                 client_,
                                                                 qos_};
        }
    };

    struct receiver_base;

    template <class... Events>
    struct event_emitting_receiver;

    net::executor executor_;
    std::vector<std::unique_ptr<typename connect_sender::receiver_base>> connect_receivers_;
    connection<Stream> connection_;
    net::steady_timer connect_and_ping_timer_;
    net::steady_timer keep_alive_timer_;

    mqtt5::connect_options connect_opts_;

    std::chrono::duration<std::uint16_t> keep_alive_used_{0};
    std::string client_id_;
    std::vector<in_flight_publish> published_messages_;

    struct connection_sm_t;

    std::unique_ptr<boost::sml::sm<connection_sm_t>> connection_sm_;

    void start_connect_timer();
    void start_ping_timer();
    void start_keep_alive_timer();

    void resolve_and_connect();

    void send_connect() {
        protocol::connect connect;
        if (connect_opts_.last_will) {
            connect.flags |= protocol::connect::will_flag;
            if (connect_opts_.last_will->retain) {
                connect.flags |= protocol::connect::will_retain_flag;
            }
            if (connect_opts_.last_will->quality_of_service == 1) {
                connect.flags |= protocol::connect::will_qos_1;
            }
            else if (connect_opts_.last_will->quality_of_service == 2) {
                connect.flags |= protocol::connect::will_qos_2;
            }
            connect.will_topic = connect_opts_.last_will->topic;
            connect.will_payload = connect_opts_.last_will->payload;

            connect.will_properties.content_type = connect_opts_.last_will->content_type;
            connect.will_properties.response_topic = connect_opts_.last_will->response_topic;
            connect.will_properties.correlation_data = connect_opts_.last_will->correlation_data;

            connect.will_properties.delay_interval = connect_opts_.last_will->delay_interval;
            connect.will_properties.message_expiry_interval =
                connect_opts_.last_will->message_expiry_interval;

            connect.will_properties.payload_format_indicator =
                connect_opts_.last_will->payload_format_indicator;
        }
        keep_alive_used_ = connect_opts_.keep_alive;
        client_id_ = connect_opts_.client_id;
        connect.keep_alive = connect_opts_.keep_alive;
        connect.client_id = connect_opts_.client_id;

        connect.flags |= connect_opts_.clean_start ? 0 : protocol::connect::clean_start_flag;

        if (!connect_opts_.username.empty()) {
            connect.username = connect_opts_.username;
            connect.flags |= protocol::connect::username_flag;
        }
        if (!connect_opts_.password.empty()) {
            connect.password = connect_opts_.password;
            connect.flags |= protocol::connect::password_flag;
        }

        send_message(std::move(connect));
    }

    void send_ping() {
        std::cout << helper::steady_now << "Sending ping\n";
        send_message(protocol::pingreq{});
    }

    template <class T>
    void send_message(T &&msg);

    void receive_one_message() {
        struct receiver : receiver_base
        {
            void set_value(protocol::control_packet &&packet) {
                this->client_->handle_message(std::move(packet));
            }

            void set_done() {
                this->client_->connection_sm_->process_event(
                    typename connection_sm_t::disconnect_evt{});
            }
        };
        p0443_v2::submit(connection_.control_packet_reader(), receiver{this});
    }

    void handle_message(protocol::control_packet &&packet) {
        std::cout << "Received message " << static_cast<int>(packet.packet_type()) << "\n";
        connection_sm_->process_event(typename connection_sm_t::packet_received_evt{&packet});
    }

    void handle_connack(protocol::connack &connack);
    void handle_puback(protocol::puback &puback);

    void close_underlying_connection() {
        connection_.next_layer().close();
    }

    void notify_connector_receivers(const bool success) {
        auto recvs = std::move(connect_receivers_);
        connect_receivers_.clear();
        for (auto &&recv : recvs) {
            if (success) {
                recv->set_value();
            }
            else {
                recv->set_done();
            }
        }
    }

public:
    template <class... Args>
    client(const net::executor &executor, Args &&... args);

    auto connector(connect_options opts) {
        connect_opts_ = std::move(opts);
        // By using sequence with just we "move" our sender into the p0443 world
        // which gives us optional coroutine support for "free"
        return connect_sender{this};
    }

    template <class Payload, class Modifier>
    auto publish(std::string topic, Payload &&payload, std::uint8_t qos, Modifier &&modifier) {
        using modifier_t = p0443_v2::remove_cvref_t<Modifier>;
        using payload_t = p0443_v2::remove_cvref_t<Payload>;
        publisher<modifier_t> pub(std::forward<Modifier>(modifier));
        pub.client_ = this;
        pub.topic_ = std::move(topic);
        if constexpr (std::is_same_v<payload_t, std::vector<std::uint8_t>>) {
            pub.payload = std::forward<Payload>(payload);
        }
        else {
            std::copy(payload.begin(), payload.end(), std::back_inserter(pub.payload));
        }
        pub.qos_ = qos;
        return pub;
    }

    auto publish(const std::string &topic, const std::string &message, std::uint8_t qos = 0) {
        return publish(topic, message, qos, [](auto &) {});
    }

    bool is_connected();

    bool is_handshaking();

    /**
     * @brief Gets the actual keep alive value used.
     *
     * The server is allowed to specify a different keep alive in the connack than
     * the client requested value. This will always reflect what is used by both
     * server and client, not what is requested by the client.
     */
    std::chrono::seconds get_actual_keep_alive() const {
        return keep_alive_used_;
    }

    /**
     * @brief The client id used by the client.
     *
     * This can be assigned by the server after the fact.
     */
    std::string client_id() const {
        return client_id_;
    }
};

template <class Stream>
struct client<Stream>::connection_sm_t
{
    struct connect_evt
    {
    };

    struct net_connect_evt
    {
    };

    struct disconnect_evt
    {
    };

    struct connection_established_evt
    {
    };

    struct packet_received_evt
    {
        protocol::control_packet *packet;
    };

    struct packet_written_evt
    {
    };

    struct ping_timeout_evt
    {
    };

    struct keep_alive_timeout_evt
    {
    };

    static constexpr auto idle = boost::sml::state<struct idle>;
    static constexpr auto start_connecting = boost::sml::state<struct start_connecting>;
    static constexpr auto connecting = boost::sml::state<struct connecting>;
    static constexpr auto handshaking = boost::sml::state<struct handshaking>;
    static constexpr auto connected = boost::sml::state<struct connected>;
    static constexpr auto disconnected = boost::sml::state<struct disconnected>;

    static constexpr auto rx_idle = boost::sml::state<struct rx_idle>;
    static constexpr auto rx_receiving = boost::sml::state<struct rx_receiving>;

    static constexpr auto ping_idle = boost::sml::state<struct ping_idle>;
    static constexpr auto ping_waiting = boost::sml::state<struct ping_waiting>;

    static constexpr auto keep_alive_idle = boost::sml::state<struct keep_alive_idle>;
    static constexpr auto keep_alive_waiting = boost::sml::state<struct keep_alive_waiting>;

    client *client_;
    auto operator()() {
        namespace sml = boost::sml;

        auto ac_start_connecting = [this] {
            client_->resolve_and_connect();
            client_->start_connect_timer();
        };
        auto start_handshake = [this] { client_->send_connect(); };
        auto close_socket = [this] { client_->close_underlying_connection(); };

        auto start_receiving = [this] { client_->receive_one_message(); };

        auto is_connack = [](packet_received_evt evt) {
            return evt.packet->template is<protocol::connack>();
        };

        auto is_puback = [](packet_received_evt evt) {
            return evt.packet->template is<protocol::puback>();
        };

        auto connection_established = [this](packet_received_evt connack) {
            std::cout << "Connection established...\n";
            client_->handle_connack(*connack.packet->template body_as<protocol::connack>());
        };

        auto handle_puback = [this](packet_received_evt puback) {
            client_->handle_puback(*puback.packet->template body_as<protocol::puback>());
        };

        auto start_ping_timer = [this] { client_->start_ping_timer(); };

        auto start_keep_alive_timer = [this] { client_->start_keep_alive_timer(); };

        auto send_ping = [this] {
            client_->send_ping();
            client_->start_ping_timer();
        };

        return sml::make_transition_table(
            *idle + sml::event<connect_evt> = start_connecting,
            start_connecting / ac_start_connecting = connecting,
            connecting + sml::event<net_connect_evt> / start_handshake = handshaking,
            connecting + sml::event<disconnect_evt> / close_socket = disconnected,

            handshaking + sml::event<packet_received_evt>[is_connack] / connection_established =
                connected,

            connected + sml::event<packet_received_evt>[is_puback] / handle_puback = connected,

            *rx_idle + sml::event<net_connect_evt> / start_receiving = rx_receiving,
            rx_receiving + sml::event<packet_received_evt> / start_receiving = rx_receiving,
            rx_receiving + sml::event<disconnect_evt> = rx_idle,

            *ping_idle + sml::event<connection_established_evt> / start_ping_timer = ping_waiting,
            ping_waiting + sml::event<disconnect_evt> = ping_idle,
            ping_waiting + sml::event<ping_timeout_evt> / send_ping = ping_waiting,

            *keep_alive_idle + sml::event<connection_established_evt> / start_keep_alive_timer =
                keep_alive_waiting,
            keep_alive_waiting + sml::event<packet_received_evt> / start_keep_alive_timer =
                keep_alive_waiting,
            keep_alive_waiting + sml::event<keep_alive_timeout_evt> / close_socket =
                keep_alive_idle,
            keep_alive_waiting + sml::event<disconnect_evt> = keep_alive_idle);
    }
};

template <class Stream>
template <class... Args>
client<Stream>::client(const net::executor &executor, Args &&... args)
    : executor_(executor), connection_(executor, std::forward<Args>(args)...),
      connect_and_ping_timer_(executor), keep_alive_timer_(executor),
      connection_sm_(new boost::sml::sm<connection_sm_t>(connection_sm_t{this})) {
    std::cout << sizeof(*connection_sm_) << "\n";
}

template <class Stream>
void client<Stream>::resolve_and_connect() {
    auto resolve_connector = p0443_v2::transform(
        p0443_v2::then(
            p0443_v2::asio::resolve(executor_, connect_opts_.hostname, connect_opts_.port),
            [this](auto results) {
                return p0443_v2::asio::connect_socket(connection_.next_layer(), results);
            }),
        [](auto...) {});
    p0443_v2::submit(std::move(resolve_connector),
                     event_emitting_receiver<typename connection_sm_t::net_connect_evt,
                                             typename connection_sm_t::disconnect_evt>{this});
}

template <class Stream>
template <class T>
void client<Stream>::send_message(T &&message) {
    std::cout << "Writing msg " << (int)message.type_value << "\n";
    p0443_v2::submit(connection_.control_packet_writer(std::forward<T>(message)),
                     event_emitting_receiver<typename connection_sm_t::packet_written_evt,
                                             typename connection_sm_t::disconnect_evt>{this});
}

template <class Stream>
void client<Stream>::handle_connack(protocol::connack &connack) {
    if (connack.properties.server_keep_alive.count() != 0) {
        keep_alive_used_ = connack.properties.server_keep_alive;
    }
    if (!connack.properties.assigned_client_id.empty()) {
        client_id_ = connack.properties.assigned_client_id;
    }

    connection_sm_->process_event(typename connection_sm_t::connection_established_evt{});
    notify_connector_receivers(true);
}

template <class Stream>
void client<Stream>::handle_puback(protocol::puback &puback) {
    auto iter =
        std::find_if(published_messages_.begin(), published_messages_.end(), [&](auto &msg) {
            return puback.packet_identifier == msg.message_.packet_identifier;
        });
    if (iter != published_messages_.end()) {
        in_flight_publish to_finish = std::move(*iter);
        published_messages_.erase(iter);
        to_finish.receiver_->set_value(puback.reason_code);
    }
}

template <class Stream>
void client<Stream>::start_connect_timer() {
    struct receiver : receiver_base
    {
        void set_value() {
            this->client_->connection_sm_->process_event(
                typename connection_sm_t::disconnect_evt{});
        }
    };
    p0443_v2::submit(
        p0443_v2::asio::timer::wait_for(connect_and_ping_timer_, std::chrono::seconds{5}),
        event_emitting_receiver<typename connection_sm_t::disconnect_evt>{this});
}

template <class Stream>
void client<Stream>::start_ping_timer() {
    if (keep_alive_used_.count() > 0) {
        p0443_v2::submit(
            p0443_v2::asio::timer::wait_for(connect_and_ping_timer_, keep_alive_used_ / 2),
            event_emitting_receiver<typename connection_sm_t::ping_timeout_evt>{this});
    }
}

template <class Stream>
void client<Stream>::start_keep_alive_timer() {
    if (keep_alive_used_.count() > 0) {
        p0443_v2::submit(
            p0443_v2::asio::timer::wait_for(keep_alive_timer_, keep_alive_used_),
            event_emitting_receiver<typename connection_sm_t::keep_alive_timeout_evt>{this});
    }
}

template <class Stream>
bool client<Stream>::is_connected() {
    return connection_sm_->is(connection_sm_t::connected);
}

template <class Stream>
bool client<Stream>::is_handshaking() {
    return connection_sm_->is(connection_sm_t::handshaking);
}
} // namespace mqtt5

//#include "detail/client/connection_state_machine.ipp"
#include "impl/client.hpp"