
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5/protocol/unsubscribe.hpp>

#include <p0443_v2/type_traits.hpp>

#include <memory>

namespace mqtt5::detail
{
struct unsubscribe_receiver_base
{
    virtual void set_value(std::vector<std::uint8_t> codes) = 0;
    virtual void set_done() = 0;
    virtual void set_error(std::exception_ptr e) = 0;

    virtual ~unsubscribe_receiver_base() = default;
};

struct in_flight_unsubscribe
{
    mqtt5::protocol::unsubscribe message_;
    std::unique_ptr<unsubscribe_receiver_base> receiver_;
};

template<class Client>
struct unsubscribe_sender
{
    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<std::vector<std::uint8_t>>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    Client* client_;
    mqtt5::protocol::unsubscribe unsub;

    template<class Receiver>
    struct operation
    {
        mqtt5::protocol::unsubscribe unsub;
        Client* client_;
        Receiver receiver_;

        struct receiver: unsubscribe_receiver_base
        {
            Receiver next_;
            receiver(Receiver &&next) : next_(std::move(next)) {
            }
            void set_value(std::vector<std::uint8_t> results) override {
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
            in_flight_unsubscribe in_flight;
            in_flight.message_ = std::move(unsub);
            in_flight.receiver_ = std::make_unique<receiver>(std::move(receiver_));
            in_flight.message_.packet_identifier = client_->next_packet_identifier();
            client_->send_message(in_flight.message_);
            client_->unsubscribe_messages_.emplace_back(std::move(in_flight));
        }
    };

    template<class Receiver>
    auto connect(Receiver &&receiver) {
        using receiver_t = p0443_v2::remove_cvref_t<Receiver>;
        return operation<receiver_t>{
            std::move(unsub),
            client_,
            std::forward<Receiver>(receiver)
        };
    }
};
}