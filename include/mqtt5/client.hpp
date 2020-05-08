
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "connection.hpp"
#include "detail/connect_sender.hpp"
#include "detail/event_emitting_receiver.hpp"
#include "detail/filter_subscribe_sender.hpp"
#include "detail/publish_sender.hpp"
#include "detail/subscribe_sender.hpp"
#include "detail/unsubscribe_sender.hpp"

#include "mqtt5/connect_options.hpp"
#include "mqtt5/disconnect_reason.hpp"
#include "mqtt5/protocol/connect.hpp"
#include "mqtt5/protocol/disconnect.hpp"
#include "mqtt5/protocol/ping.hpp"
#include "mqtt5/protocol/publish.hpp"
#include "mqtt5/puback_reason_code.hpp"
#include "mqtt5/publish_options.hpp"
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
#include <p0443_v2/sink_receiver.hpp>
#include <p0443_v2/submit.hpp>

#include <boost/asio/steady_timer.hpp>
#include <p0443_v2/type_traits.hpp>

#include <boost/mp11/tuple.hpp>

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

    struct received_qos2_state
    {
        enum class state_type { pubrec_sent };
        protocol::publish publish_;
        state_type current_state_;
    };

    template <class Client, class... Events>
    friend struct detail::event_emitting_receiver;

    template <class, class>
    friend struct detail::publish_sender;
    template <class, class>
    friend struct detail::subscribe_sender;
    template <class>
    friend struct detail::filter_subscribe_sender;
    template <class>
    friend struct detail::unsubscribe_sender;

    template <class, class>
    friend struct detail::reusable_publish_sender;

    template <class>
    friend struct detail::connect_sender;

    net::executor executor_;
    std::vector<std::unique_ptr<detail::message_receiver_base<>>> connect_receivers_;
    connection<Stream> connection_;
    net::steady_timer connect_and_ping_timer_;
    net::steady_timer keep_alive_timer_;

    mqtt5::connect_options connect_opts_;

    std::chrono::duration<std::uint16_t> keep_alive_used_{0};
    std::string client_id_;
    std::vector<detail::in_flight_publish> published_messages_;
    std::vector<detail::in_flight_subscribe> subscribe_messages_;
    std::vector<detail::in_flight_unsubscribe> unsubscribe_messages_;

    std::vector<detail::filtered_subscription> publish_waiters_;
    void deliver_to_publish_waiters(const protocol::publish &publish) {
        using filtered_sub_container_t = decltype(publish_waiters_.front().receivers_);

        std::vector<filtered_sub_container_t> all_receivers;
        for (auto iter = publish_waiters_.begin(); iter != publish_waiters_.end();) {
            if (iter->filter_.matches(publish.topic)) {
                all_receivers.emplace_back(std::move(iter->receivers_));
                iter = publish_waiters_.erase(iter);
            }
            else {
                iter++;
            }
        }

        for (auto &rv : all_receivers) {
            for (auto &rx : rv) {
                rx->set_value(publish);
            }
        }
    }

    std::vector<received_qos2_state> received_qos2_states_;

    std::vector<std::unique_ptr<detail::publish_op_starter_base>> queued_publishes_;
    std::uint16_t server_max_send_quota_{65535};
    std::uint16_t server_send_quota_{65535};

    std::uint16_t client_receive_quota_{65535};

    void maybe_send_queued_publish() {
        if (server_send_quota_ < server_max_send_quota_) {
            server_send_quota_++;

            if (!queued_publishes_.empty()) {
                auto next = std::move(queued_publishes_.front());
                queued_publishes_.erase(queued_publishes_.begin());
                next->start();
            }
        }
    }

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
    void handle_packet(protocol::connack &connack);
    void handle_packet(protocol::puback &puback);
    void handle_packet(protocol::suback &suback);
    void handle_packet(protocol::unsuback &unsuback);
    void handle_packet(protocol::publish &publish);
    void handle_packet(protocol::pubrec &pubrec);
    void handle_packet(protocol::pubrel &pubrel);
    void handle_packet(protocol::pubcomp &pubcomp);
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

    [[nodiscard]] auto get_executor() {
        return connection_.get_executor();
    }

    [[nodiscard]] auto socket_connector(boost::string_view host, boost::string_view port);

    template <int N>
    [[nodiscard]] auto &get_nth_layer() {
        return get_nth_layer_impl<N>(connection_);
    }

    [[nodiscard]] auto handshaker(connect_options opts) {
        connect_opts_ = std::move(opts);
        return detail::connect_sender{this};
    }

    template <class Payload, class... Opts>
    [[nodiscard]] auto publisher(std::string topic, Payload &&payload, Opts &&... opts) {
        auto opts_and_modifiers =
            publish_options::detail::separate_options_modifiers(std::forward<Opts>(opts)...);
        auto late_modifier =
            [late_mods = std::move(opts_and_modifiers.modifiers)](protocol::publish &pub) mutable {
                boost::mp11::tuple_for_each(late_mods, [&](auto &mod) { mod(pub); });
            };
        detail::publish_sender pub(this, std::move(late_modifier));
        pub.message_.topic = std::move(topic);
        pub.message_.set_payload(std::forward<Payload>(payload));
        boost::mp11::tuple_for_each(opts_and_modifiers.options,
                                    [&](auto &opt) { opt(pub.message_); });
        return pub;
    }

    template <class Payload, class... Opts>
    [[nodiscard]] auto reusable_publisher(std::string topic, Payload &&payload, Opts &&... opts) {
        auto opts_and_modifiers =
            publish_options::detail::separate_options_modifiers(std::forward<Opts>(opts)...);
        auto late_modifier =
            [late_mods = std::move(opts_and_modifiers.modifiers)](protocol::publish &pub) mutable {
                boost::mp11::tuple_for_each(late_mods, [&](auto &mod) { mod(pub); });
            };
        detail::reusable_publish_sender pub(this, std::move(late_modifier));
        pub.message_.topic = std::move(topic);
        pub.message_.set_payload(std::forward<Payload>(payload));
        boost::mp11::tuple_for_each(opts_and_modifiers.options,
                                    [&](auto &opt) { opt(pub.message_); });
        return pub;
    }

    template <class Modifier>
    [[nodiscard]] auto subscriber(std::vector<mqtt5::single_subscription> subs,
                                  Modifier &&modifier) {
        using modifier_t = p0443_v2::remove_cvref_t<Modifier>;
        auto retval = detail::subscribe_sender{this, std::forward<Modifier>(modifier)};
        retval.subscriptions_ = std::move(subs);
        return retval;
    }

    [[nodiscard]] auto subscriber(std::vector<mqtt5::single_subscription> subs) {
        return subscriber(std::move(subs), [](auto &) {});
    }

    [[nodiscard]] auto subscriber(topic_filter topic, mqtt5::quality_of_service qos) {
        single_subscription single_sub;
        single_sub.topic = topic;
        single_sub.quality_of_service = qos;
        return subscriber({single_sub}, [](auto &) {});
    }

    [[nodiscard]] auto filtered_subscriber(topic_filter topic) {
        return detail::filter_subscribe_sender{this, std::move(topic)};
    }

    [[nodiscard]] auto unsubscriber(std::vector<std::string> topics) {
        auto retval = detail::unsubscribe_sender<client>{this};
        retval.unsub.topics = std::move(topics);
        return retval;
    }

    [[nodiscard]] auto
    disconnector(mqtt5::disconnect_reason reason = mqtt5::disconnect_reason::normal) {
        return p0443_v2::transform(
            connection_.control_packet_writer(mqtt5::protocol::disconnect(reason)),
            [this](auto...) { this->close(); });
    }

    [[nodiscard]] bool is_connected();
    [[nodiscard]] bool is_handshaking();

    /**
     * @brief Gets the actual keep alive value used.
     *
     * The server is allowed to specify a different keep alive in the connack than
     * the client requested value. This will always reflect what is used by both
     * server and client, not what is requested by the client.
     */
    [[nodiscard]] std::chrono::seconds get_actual_keep_alive() const {
        return keep_alive_used_;
    }

    /**
     * @brief The client id used by the client.
     *
     * This can be assigned by the server after the fact.
     */
    [[nodiscard]] std::string client_id() const {
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

    struct puback_sent_evt
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

    template <class PacketT>
    auto is_packet() {
        return [](packet_received_evt evt) { return evt.packet->template is<PacketT>(); };
    }

    template <class PacketT>
    auto packet_handler() {
        return [c = this->client_](packet_received_evt evt) {
            c->handle_packet(*evt.packet->template body_as<PacketT>());
        };
    }

    client *client_;
    auto operator()() {
        namespace sml = boost::sml;

        auto ac_start_handshake = [this] {
            client_->start_connect_timer();
            client_->send_connect();
        };
        auto close_socket = [this] { client_->close(); };

        auto start_receiving = [this] { client_->receive_one_message(); };

        auto start_ping_timer = [this] { client_->start_ping_timer(); };

        auto start_keep_alive_timer = [this] { client_->start_keep_alive_timer(); };

        auto send_ping = [this] {
            client_->send_ping();
            client_->start_ping_timer();
        };

        auto stop_ping_timer = [this] { client_->stop_ping_timer(); };

        auto stop_keep_alive_timer = [this] { client_->stop_keep_alive_timer(); };

        auto puback_sent_handler = [this] {
            if (client_->client_receive_quota_ < client_->connect_opts_.receive_maximum) {
                client_->client_receive_quota_++;
            }
        };

        return sml::make_transition_table(
            *idle + sml::event<handshake_evt> = start_handshake,
            start_handshake / ac_start_handshake = handshaking,

            handshaking + sml::event<packet_received_evt>[is_packet<protocol::connack>()] /
                              packet_handler<protocol::connack>() = connected,
            handshaking + sml::event<disconnect_evt> / close_socket = idle,

            connected + sml::event<packet_received_evt>[is_packet<protocol::puback>()] /
                            packet_handler<protocol::puback>() = connected,
            connected + sml::event<packet_received_evt>[is_packet<protocol::suback>()] /
                            packet_handler<protocol::suback>() = connected,
            connected + sml::event<packet_received_evt>[is_packet<protocol::unsuback>()] /
                            packet_handler<protocol::unsuback>() = connected,
            connected + sml::event<packet_received_evt>[is_packet<protocol::publish>()] /
                            packet_handler<protocol::publish>() = connected,
            connected + sml::event<packet_received_evt>[is_packet<protocol::pubrec>()] /
                            packet_handler<protocol::pubrec>() = connected,
            connected + sml::event<packet_received_evt>[is_packet<protocol::pubrel>()] /
                            packet_handler<protocol::pubrel>() = connected,
            connected + sml::event<packet_received_evt>[is_packet<protocol::pubcomp>()] /
                            packet_handler<protocol::pubcomp>() = connected,
            connected + sml::event<puback_sent_evt> / puback_sent_handler = connected,

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

template <class Stream>
void client<Stream>::close() {
    try {
        connection_.lowest_layer().cancel();
    }
    catch (std::exception &) {
    }
    try {
        connection_.lowest_layer().close();
    }
    catch (std::exception &) {
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
void client<Stream>::handle_packet(protocol::connack &connack) {
    if (connack.properties.server_keep_alive.count() != 0) {
        keep_alive_used_ = connack.properties.server_keep_alive;
    }
    if (!connack.properties.assigned_client_id.empty()) {
        client_id_ = connack.properties.assigned_client_id;
    }
    server_max_send_quota_ = connack.properties.receive_maximum;
    server_send_quota_ = connack.properties.receive_maximum;

    client_receive_quota_ = connect_opts_.receive_maximum;

    connection_sm_->process_event(typename connection_sm_t::handshake_done_evt{});
    notify_connector_receivers(true);
}

template <class Stream>
void client<Stream>::handle_packet(protocol::puback &puback) {
    auto iter =
        std::find_if(published_messages_.begin(), published_messages_.end(), [&](auto &msg) {
            return puback.packet_identifier == msg.message_.packet_identifier;
        });
    if (iter != published_messages_.end()) {
        detail::in_flight_publish to_finish = std::move(*iter);
        published_messages_.erase(iter);
        if (to_finish.state_ == detail::in_flight_publish::state_type::waiting_puback) {
            to_finish.receiver_->set_value(static_cast<publish_result>(puback.reason_code));
        }
        else {
            to_finish.receiver_->set_done();
            close();
        }
    }

    maybe_send_queued_publish();
}

template <class Stream>
void client<Stream>::handle_packet(protocol::suback &suback) {
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
void client<Stream>::handle_packet(protocol::unsuback &unsuback) {
    auto iter =
        std::find_if(unsubscribe_messages_.begin(), unsubscribe_messages_.end(), [&](auto &msg) {
            return unsuback.packet_identifier == msg.message_.packet_identifier;
        });
    if (iter != unsubscribe_messages_.end()) {
        detail::in_flight_unsubscribe to_finish = std::move(*iter);
        unsubscribe_messages_.erase(iter);
        to_finish.receiver_->set_value(std::move(unsuback.reason_codes));
    }
}

template <class Stream>
void client<Stream>::handle_packet(protocol::publish &publish) {
    if (publish.quality_of_service() != 0_qos) {
        if (client_receive_quota_ > 0) {
            client_receive_quota_--;
        }
        else {
            p0443_v2::submit(disconnector(mqtt5::disconnect_reason::quota_exceeded),
                             p0443_v2::sink_receiver{});
        }
    }

    bool deliver_to_receivers = true;

    if (publish.quality_of_service() == 1_qos) {
        protocol::puback ack;
        ack.packet_identifier = publish.packet_identifier;
        p0443_v2::submit(
            connection_.control_packet_writer(ack),
            detail::event_emitting_receiver<client<Stream>,
                                            typename connection_sm_t::puback_sent_evt>{this});
    }
    else if (publish.quality_of_service() == 2_qos) {
        auto iter =
            std::find_if(received_qos2_states_.begin(), received_qos2_states_.end(),
                         [&](const received_qos2_state &state) {
                             return state.publish_.packet_identifier == publish.packet_identifier;
                         });
        deliver_to_receivers = false;
        protocol::pubrec rec;
        rec.packet_identifier = publish.packet_identifier;

        if (iter == received_qos2_states_.end()) {
            received_qos2_state new_state;
            new_state.current_state_ = received_qos2_state::state_type::pubrec_sent;
            new_state.publish_ = std::move(publish);
            received_qos2_states_.emplace_back(std::move(new_state));
        }
        send_message(rec);
    }
    if (deliver_to_receivers) {
        deliver_to_publish_waiters(publish);
    }
}

template <class Stream>
void client<Stream>::handle_packet(protocol::pubrec &pubrec) {
    auto iter =
        std::find_if(published_messages_.begin(), published_messages_.end(), [&](const auto &msg) {
            return msg.message_.packet_identifier == pubrec.packet_identifier;
        });
    protocol::pubrel response;
    response.packet_identifier = pubrec.packet_identifier;
    bool send_response = true;

    if (iter == published_messages_.end()) {
        response.reason_code = pubrel_reason_code::packet_identifier_not_found;
    }
    else if (pubrec.reason_code > pubrec_reason_code::no_matching_subscribers) {
        detail::in_flight_publish to_finish = std::move(*iter);
        published_messages_.erase(iter);
        to_finish.receiver_->set_value(static_cast<publish_result>(pubrec.reason_code));
        send_response = false;
    }
    else if (iter->state_ == detail::in_flight_publish::state_type::waiting_puback) {
        detail::in_flight_publish to_finish = std::move(*iter);
        published_messages_.erase(iter);
        to_finish.receiver_->set_done();
        close();
        send_response = false;
    }
    else {
        iter->state_ = detail::in_flight_publish::state_type::waitiing_pubcomp;
    }
    if (send_response) {
        send_message(response);
    }
}

template <class Stream>
void client<Stream>::handle_packet(protocol::pubrel &pubrel) {
    protocol::pubcomp response;
    response.packet_identifier = pubrel.packet_identifier;
    auto iter =
        std::find_if(received_qos2_states_.begin(), received_qos2_states_.end(),
                     [&](const received_qos2_state &state) {
                         return state.publish_.packet_identifier == pubrel.packet_identifier;
                     });
    std::optional<protocol::publish> publish;
    if (iter == received_qos2_states_.end()) {
        response.reason_code = pubcomp_reason_code::packet_identifier_not_found;
    }
    else {
        publish = std::move(iter->publish_);
        received_qos2_states_.erase(iter);
    }
    send_message(response);
    if (publish) {
        deliver_to_publish_waiters(*publish);
    };
}

template <class Stream>
void client<Stream>::handle_packet(protocol::pubcomp &pubcomp) {
    auto iter =
        std::find_if(published_messages_.begin(), published_messages_.end(),
                     [&](const detail::in_flight_publish &state) {
                         return state.message_.packet_identifier == pubcomp.packet_identifier;
                     });
    if (iter == published_messages_.end()) {
        // Do nothing
    }
    else {
        detail::in_flight_publish to_finish = std::move(*iter);
        published_messages_.erase(iter);
        to_finish.receiver_->set_value(static_cast<publish_result>(pubcomp.reason_code));
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

    connect.connect_properties.receive_maximum = connect_opts_.receive_maximum;

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
