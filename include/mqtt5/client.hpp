
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "connection.hpp"
#include "detail/connect_sender.hpp"
#include "detail/event_emitting_receiver.hpp"
#include "detail/publish_sender.hpp"
#include "detail/subscribe_sender.hpp"

#include "mqtt5/connect_options.hpp"
#include "mqtt5/protocol/connect.hpp"
#include "mqtt5/protocol/ping.hpp"
#include "mqtt5/protocol/publish.hpp"
#include "mqtt5/puback_reason_code.hpp"
#include "mqtt5/quality_of_service.hpp"
#include "mqtt5/topic_filter.hpp"
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

    template <class Client, class... Events>
    friend struct detail::event_emitting_receiver;

    template <class, class>
    friend struct detail::publish_sender;
    template <class, class>
    friend struct detail::subscribe_sender;

    template <class, class>
    friend struct detail::reusable_publish_sender;

    template <class>
    friend struct detail::connect_sender;

    net::executor executor_;
    std::vector<std::unique_ptr<detail::connect_sender_receiver_base>> connect_receivers_;
    connection<Stream> connection_;
    net::steady_timer connect_and_ping_timer_;
    net::steady_timer keep_alive_timer_;

    mqtt5::connect_options connect_opts_;

    std::chrono::duration<std::uint16_t> keep_alive_used_{0};
    std::string client_id_;
    std::vector<detail::in_flight_publish> published_messages_;
    std::vector<detail::in_flight_subscribe> subscribe_messages_;

    struct connection_sm_t;

    std::unique_ptr<boost::sml::sm<connection_sm_t>> connection_sm_;

    void start_connect_timer();
    void start_ping_timer();
    void stop_ping_timer();
    void start_keep_alive_timer();
    void stop_keep_alive_timer();
    void send_connect();
    void send_ping();

    template <class T>
    void send_message(T &&msg);
    void receive_one_message();
    void handle_connack(protocol::connack &connack);
    void handle_puback(protocol::puback &puback);
    void handle_suback(protocol::suback &suback);
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

    template <int N, class T>
    auto &get_nth_layer_impl(T &current) {
        if constexpr (N == 0) {
            return current;
        }
        else {
            return get_nth_layer_impl<N - 1>(current.next_layer());
        }
    }

public:
    template <class... Args>
    client(const net::executor &executor, Args &&... args);

    void close();

    auto get_executor() {
        return connection_.get_executor();
    }

    auto socket_connector(boost::string_view host, boost::string_view port);

    template <int N>
    auto &get_nth_layer() {
        return get_nth_layer_impl<N>(connection_);
    }

    auto handshaker(connect_options opts) {
        connect_opts_ = std::move(opts);
        return detail::connect_sender{this};
    }

    template <class Payload, class Modifier>
    auto publisher(std::string topic, Payload &&payload, quality_of_service qos,
                   Modifier &&modifier) {
        using modifier_t = p0443_v2::remove_cvref_t<Modifier>;
        using payload_t = p0443_v2::remove_cvref_t<Payload>;
        detail::publish_sender pub(this, std::forward<Modifier>(modifier));
        pub.topic_ = std::move(topic);
        if constexpr (std::is_same_v<payload_t, std::vector<std::uint8_t>>) {
            pub.payload = std::forward<Payload>(payload);
        }
        else {
            using std::begin;
            using std::end;
            std::copy(begin(payload), end(payload), std::back_inserter(pub.payload));
        }
        pub.qos_ = qos;
        return pub;
    }

    auto publisher(const std::string &topic, const std::string &message,
                   quality_of_service qos = 0_qos) {
        return publisher(topic, message, qos, [](auto &) {});
    }

    template <class Payload, class Modifier>
    auto reusable_publisher(std::string topic, Payload &&payload, quality_of_service qos,
                            Modifier &&modifier) {
        using modifier_t = p0443_v2::remove_cvref_t<Modifier>;
        using payload_t = p0443_v2::remove_cvref_t<Payload>;
        detail::reusable_publish_sender pub(this, std::forward<Modifier>(modifier));
        pub.topic_ = std::move(topic);
        if constexpr (std::is_same_v<payload_t, std::vector<std::uint8_t>>) {
            pub.payload = std::forward<Payload>(payload);
        }
        else {
            using std::begin;
            using std::end;
            std::copy(begin(payload), end(payload), std::back_inserter(pub.payload));
        }
        pub.qos_ = qos;
        return pub;
    }

    auto reusable_publisher(const std::string &topic, const std::string &message,
                            quality_of_service qos = 0_qos) {
        return reusable_publisher(topic, message, qos, [](auto &) {});
    }

    template <class Modifier>
    auto subscriber(std::vector<mqtt5::single_subscription> subs, Modifier &&modifier) {
        using modifier_t = p0443_v2::remove_cvref_t<Modifier>;
        auto retval = detail::subscribe_sender{this, std::forward<Modifier>(modifier)};
        retval.subscriptions_ = std::move(subs);
        return retval;
    }
    auto subscriber(topic_filter topic, mqtt5::quality_of_service qos) {
        single_subscription single_sub;
        single_sub.topic = topic;
        single_sub.quality_of_service = qos;
        return subscriber({single_sub}, [](auto &) {});
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
    struct handshake_evt
    {
    };
    struct disconnect_evt
    {
    };

    struct handshake_done_evt
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
    static constexpr auto start_handshake = boost::sml::state<struct start_handshake>;
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

        auto ac_start_handshake = [this] {
            client_->start_connect_timer();
            client_->send_connect();
        };
        auto close_socket = [this] {
            client_->close();
        };

        auto start_receiving = [this] {
            client_->receive_one_message();
        };

        auto is_connack = [](packet_received_evt evt) {
            return evt.packet->template is<protocol::connack>();
        };

        auto is_puback = [](packet_received_evt evt) {
            return evt.packet->template is<protocol::puback>();
        };

        auto is_suback = [](packet_received_evt evt) {
            return evt.packet->template is<protocol::suback>();
        };

        auto connection_established = [this](packet_received_evt connack) {
            client_->handle_connack(*connack.packet->template body_as<protocol::connack>());
        };

        auto handle_puback = [this](packet_received_evt puback) {
            client_->handle_puback(*puback.packet->template body_as<protocol::puback>());
        };

        auto handle_suback = [this](packet_received_evt suback) {
            client_->handle_suback(*suback.packet->template body_as<protocol::suback>());
        };

        auto start_ping_timer = [this] { client_->start_ping_timer(); };

        auto start_keep_alive_timer = [this] { client_->start_keep_alive_timer(); };

        auto send_ping = [this] {
            client_->send_ping();
            client_->start_ping_timer();
        };

        auto stop_ping_timer = [this] { client_->stop_ping_timer(); };

        auto stop_keep_alive_timer = [this] { client_->stop_keep_alive_timer(); };

        return sml::make_transition_table(
            *idle + sml::event<handshake_evt> = start_handshake,
            start_handshake / ac_start_handshake = handshaking,

            handshaking + sml::event<packet_received_evt>[is_connack] / connection_established =
                connected,
            handshaking + sml::event<disconnect_evt> / close_socket = idle,

            connected + sml::event<packet_received_evt>[is_puback] / handle_puback = connected,
            connected + sml::event<packet_received_evt>[is_suback] / handle_suback = connected,

            *rx_idle + sml::event<handshake_evt> / start_receiving = rx_receiving,
            rx_receiving + sml::event<packet_received_evt> / start_receiving = rx_receiving,
            rx_receiving + sml::event<disconnect_evt> / close_socket = rx_idle,

            *ping_idle + sml::event<handshake_done_evt> / start_ping_timer = ping_waiting,
            ping_waiting + sml::event<disconnect_evt> / stop_ping_timer = ping_idle,
            ping_waiting + sml::event<ping_timeout_evt> / send_ping = ping_waiting,

            *keep_alive_idle + sml::event<handshake_done_evt> / start_keep_alive_timer =
                keep_alive_waiting,
            keep_alive_waiting + sml::event<packet_received_evt> / start_keep_alive_timer =
                keep_alive_waiting,
            keep_alive_waiting + sml::event<keep_alive_timeout_evt> / close_socket =
                keep_alive_idle,
            keep_alive_waiting + sml::event<disconnect_evt> / stop_keep_alive_timer =
                keep_alive_idle);
    }
};

template <class Stream>
template <class... Args>
client<Stream>::client(const net::executor &executor, Args &&... args)
    : executor_(executor), connection_(executor, std::forward<Args>(args)...),
      connect_and_ping_timer_(executor), keep_alive_timer_(executor),
      connection_sm_(new boost::sml::sm<connection_sm_t>(connection_sm_t{this})) {
}

template<class Stream>
void client<Stream>::close()
{
    try
    {
        connection_.lowest_layer().cancel();
    }
    catch(std::exception&) {
    }
    try
    {
        connection_.lowest_layer().close();
    }
    catch(std::exception&) {
    }
}

template <class Stream>
auto client<Stream>::socket_connector(boost::string_view host, boost::string_view port) {
    return p0443_v2::transform(
        p0443_v2::then(p0443_v2::asio::resolve(executor_, host.to_string(), port.to_string()),
                       [this](auto results) {
                           return p0443_v2::asio::connect_socket(connection_.lowest_layer(),
                                                                 results);
                       }),
        [](auto...) {});
}

template <class Stream>
template <class T>
void client<Stream>::send_message(T &&message) {
    p0443_v2::submit(
        connection_.control_packet_writer(std::forward<T>(message)),
        detail::event_emitting_receiver<client<Stream>,
                                        typename connection_sm_t::packet_written_evt,
                                        typename connection_sm_t::disconnect_evt>{this});
}

template <class Stream>
void client<Stream>::receive_one_message() {
    struct receiver : detail::event_emitting_receiver_base<client<Stream>>
    {
        void set_value(protocol::control_packet &&packet) {
            this->client_->connection_sm_->process_event(
                typename connection_sm_t::packet_received_evt{&packet});
        }

        void set_done() {
            this->client_->connection_sm_->process_event(
                typename connection_sm_t::disconnect_evt{});
        }
    };
    p0443_v2::submit(connection_.control_packet_reader(), receiver{this});
}

template <class Stream>
void client<Stream>::handle_connack(protocol::connack &connack) {
    if (connack.properties.server_keep_alive.count() != 0) {
        keep_alive_used_ = connack.properties.server_keep_alive;
    }
    if (!connack.properties.assigned_client_id.empty()) {
        client_id_ = connack.properties.assigned_client_id;
    }

    connection_sm_->process_event(typename connection_sm_t::handshake_done_evt{});
    notify_connector_receivers(true);
}

template <class Stream>
void client<Stream>::handle_puback(protocol::puback &puback) {
    auto iter =
        std::find_if(published_messages_.begin(), published_messages_.end(), [&](auto &msg) {
            return puback.packet_identifier == msg.message_.packet_identifier;
        });
    if (iter != published_messages_.end()) {
        detail::in_flight_publish to_finish = std::move(*iter);
        published_messages_.erase(iter);
        to_finish.receiver_->set_value(puback.reason_code);
    }
}

template <class Stream>
void client<Stream>::handle_suback(protocol::suback &suback) {
    auto iter =
        std::find_if(subscribe_messages_.begin(), subscribe_messages_.end(), [&](auto &msg) {
            return suback.packet_identifier == msg.message_.packet_identifier;
        });
    if (iter != subscribe_messages_.end()) {
        detail::in_flight_subscribe to_finish = std::move(*iter);
        subscribe_messages_.erase(iter);
        mqtt5::subscribe_result result;
        result.codes.reserve(suback.reason_codes.size());
        for (auto &c : suback.reason_codes) {
            result.codes.push_back(static_cast<subscribe_result::result_code>(c));
        }
        to_finish.receiver_->set_value(std::move(result));
    }
}

template <class Stream>
void client<Stream>::start_connect_timer() {
    p0443_v2::submit(
        p0443_v2::asio::timer::wait_for(connect_and_ping_timer_, std::chrono::seconds{5}),
        detail::event_emitting_receiver<client<Stream>, typename connection_sm_t::disconnect_evt>{
            this});
}

template <class Stream>
void client<Stream>::start_ping_timer() {
    if (keep_alive_used_.count() > 0) {
        p0443_v2::submit(
            p0443_v2::asio::timer::wait_for(connect_and_ping_timer_, keep_alive_used_ / 2),
            detail::event_emitting_receiver<client<Stream>,
                                            typename connection_sm_t::ping_timeout_evt>{this});
    }
}

template <class Stream>
void client<Stream>::stop_ping_timer() {
    connect_and_ping_timer_.cancel();
}

template <class Stream>
void client<Stream>::start_keep_alive_timer() {
    if (keep_alive_used_.count() > 0) {
        p0443_v2::submit(
            p0443_v2::asio::timer::wait_for(keep_alive_timer_, keep_alive_used_),
            detail::event_emitting_receiver<client<Stream>,
                                            typename connection_sm_t::keep_alive_timeout_evt>{
                this});
    }
}

template <class Stream>
void client<Stream>::stop_keep_alive_timer() {
    keep_alive_timer_.cancel();
}

template <class Stream>
void client<Stream>::send_connect() {
    protocol::connect connect;
    if (connect_opts_.last_will) {
        connect.flags |= protocol::connect::will_flag;
        if (connect_opts_.last_will->retain) {
            connect.flags |= protocol::connect::will_retain_flag;
        }
        if (connect_opts_.last_will->quality_of_service == 1_qos) {
            connect.flags |= protocol::connect::will_qos_1;
        }
        else if (connect_opts_.last_will->quality_of_service == 2_qos) {
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

template <class Stream>
void client<Stream>::send_ping() {
    send_message(protocol::pingreq{});
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