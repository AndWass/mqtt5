
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "mqtt5/quality_of_service.hpp"
#include <mqtt5/protocol/publish.hpp>

#include "message_receiver_base.hpp"

namespace mqtt5::detail
{
struct in_flight_publish
{
    enum class state_type
    {
        waiting_puback,
        waiting_pubrec,
        waitiing_pubcomp
    };
    protocol::publish message_;
    std::unique_ptr<detail::message_receiver_base<mqtt5::publish_result>> receiver_;
    state_type state_ = state_type::waiting_puback;
};

struct publish_op_starter_base
{
    virtual ~publish_op_starter_base() = default;
    virtual void start() = 0;
};

template <class Fn>
struct publish_op_starter : publish_op_starter_base
{
    Fn function_;

    publish_op_starter(Fn &&fn) : function_(std::move(fn)) {
    }

    void start() override {
        function_();
    }
};

template <class Client, class Modifier>
struct publish_sender
{
    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<mqtt5::publish_result>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    protocol::publish message_;
    Modifier modifying_function_;
    Client *client_;

    publish_sender(Client *client, Modifier modifier)
        : modifying_function_(std::move(modifier)), client_(client) {
    }

    template <class Receiver>
    struct operation
    {
        Receiver receiver_;

        protocol::publish message_;
        Modifier modifying_function_;
        Client *client_;

        struct publish_receiver : detail::message_receiver_base<mqtt5::publish_result>
        {
            Receiver next_;

            publish_receiver(Receiver &&next) : next_(std::move(next)) {
            }

            void set_value(mqtt5::publish_result code) override {
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
            modifying_function_(message_);

            if (message_.quality_of_service() == 0_qos) {
                client_->send_message(std::move(message_));
                p0443_v2::set_value(std::move(receiver_), publish_result::success);
            }
            else {
                message_.packet_identifier = client_->next_packet_identifier();
                auto start_fn = [client_ = client_, message_ = std::move(message_),
                                 receiver_ = std::move(receiver_)]() mutable {
                    --client_->server_send_quota_;
                    client_->send_message(message_);
                    auto start_state = in_flight_publish::state_type::waiting_puback;
                    if(message_.quality_of_service() == 2_qos)
                    {
                        start_state = in_flight_publish::state_type::waiting_pubrec;
                    }
                    in_flight_publish stored{
                        std::move(message_),
                        std::make_unique<publish_receiver>(std::move(receiver_)),
                        start_state
                        };

                    client_->published_messages_.emplace_back(std::move(stored));
                };
                if (client_->server_send_quota_ > 0) {
                    start_fn();
                }
                else {
                    auto publish_starter = std::make_unique<publish_op_starter<decltype(start_fn)>>(
                        std::move(start_fn));
                    client_->queued_publishes_.push_back(std::move(publish_starter));
                }
            }
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        return operation<p0443_v2::remove_cvref_t<Receiver>>{
            std::forward<Receiver>(receiver), std::move(message_), modifying_function_, client_};
    }
};

template <class Client, class Modifier>
struct reusable_publish_sender
{
    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<mqtt5::publish_result>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    protocol::publish message_;
    Modifier modifying_function_;
    Client *client_;

    reusable_publish_sender(Client *client, Modifier modifier)
        : modifying_function_(std::move(modifier)), client_(client) {
    }

    reusable_publish_sender(const reusable_publish_sender &) = default;
    reusable_publish_sender(reusable_publish_sender &&rhs) : reusable_publish_sender(rhs) {
    }

    reusable_publish_sender &operator=(const reusable_publish_sender &) = default;
    reusable_publish_sender &operator=(reusable_publish_sender &&rhs) {
        if (this != &rhs) {
            *this = rhs;
        }
        return *this;
    }
    ~reusable_publish_sender() = default;

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        return typename publish_sender<Client, Modifier>::template operation<
            p0443_v2::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver), message_,
                                                modifying_function_, client_};
    }
};
} // namespace mqtt5::detail