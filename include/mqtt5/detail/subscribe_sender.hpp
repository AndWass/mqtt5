
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "mqtt5/protocol/subscribe.hpp"
#include "mqtt5/quality_of_service.hpp"
#include "mqtt5/topic_filter.hpp"

#include <memory>
#include <p0443_v2/type_traits.hpp>
#include <vector>

#include "message_receiver_base.hpp"

namespace mqtt5
{
enum class subscription_retain_handling {
    send_retain_on_subscribe,
    send_retain_on_subscribe_if_nonexistant,
    do_not_send_retained_on_subscribe
};
struct single_subscription
{
    topic_filter topic;
    mqtt5::quality_of_service quality_of_service = 1_qos;
    bool no_local = false;
    bool retain_as_published = false;
    mqtt5::subscription_retain_handling subscription_retain_handling =
        mqtt5::subscription_retain_handling::send_retain_on_subscribe;
};

struct subscribe_result
{
    enum class result_code : std::uint8_t {
        granted_qos0,
        granted_qos1,
        granted_qos2,
        unspecified_error = 0x80,
        implementation_specific_error = 0x83,
        unauthorized = 0x87,
        invalid_topic_filter = 0x8f,
        packet_identifier_in_use = 0x91,
        quota_exceeded = 0x97,
        shared_subscriptions_not_supported = 0x9e,
        subscription_identifiers_not_supported = 0xa1,
        wildcard_subscriptions_not_supported = 0xa2
    };
    std::vector<result_code> codes;
};
namespace detail
{
struct in_flight_subscribe
{
    protocol::subscribe message_;
    std::unique_ptr<message_receiver_base<subscribe_result>> receiver_;
};

template <class Client, class Modifier>
struct subscribe_sender
{
    Client *client_;
    Modifier modifier_;
    std::vector<single_subscription> subscriptions_;

    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<subscribe_result>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    template <class Receiver>
    struct operation
    {
        Receiver receiver_;
        Client *client_;
        Modifier modifier_;
        std::vector<single_subscription> subscriptions_;

        struct receiver : message_receiver_base<subscribe_result>
        {
            Receiver next_;
            receiver(Receiver &&next) : next_(std::move(next)) {
            }
            void set_value(subscribe_result results) override {
                p0443_v2::set_value(std::move(next_), std::move(results));
            }
            void set_done() override {
                p0443_v2::set_done(std::move(next_));
            }
            void set_error(std::exception_ptr e) override {
                p0443_v2::set_error(std::move(next_), std::move(e));
            }
        };

        void start() {
            in_flight_subscribe in_flight;
            in_flight.receiver_ = std::make_unique<receiver>(std::move(receiver_));

            for (auto &s : subscriptions_) {
                std::uint8_t flags = flags = static_cast<std::uint8_t>(s.quality_of_service);
                if (s.no_local) {
                    flags += 0x04;
                }
                if (s.retain_as_published) {
                    flags += 0x08;
                }
                flags += static_cast<std::uint8_t>(s.subscription_retain_handling) << 4;
                in_flight.message_.topics.emplace_back(s.topic.to_string(), flags);
            }
            modifier_(in_flight.message_);
            in_flight.message_.packet_identifier = client_->next_packet_identifier();
            client_->send_message(in_flight.message_);
            client_->subscribe_messages_.emplace_back(std::move(in_flight));
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        using receiver_t = p0443_v2::remove_cvref_t<Receiver>;
        return operation<receiver_t>{std::forward<Receiver>(receiver), client_,
                                     std::move(modifier_), std::move(subscriptions_)};
    }
};
template<class C, class M>
subscribe_sender(C*, M&&) -> subscribe_sender<C, p0443_v2::remove_cvref_t<M>>;
} // namespace detail
} // namespace mqtt5