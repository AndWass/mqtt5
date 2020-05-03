
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <exception>

namespace mqtt5::detail
{
template<class Client>
struct event_emitting_receiver_base
{
    Client *client_;
    event_emitting_receiver_base(Client *client): client_(client) {}
    event_emitting_receiver_base(Client &client): client_(&client) {}

    void set_done() {
    }
    template <class E>
    void set_error(E &&e) {
        // Do this more graceful!
        std::terminate();
    }
};

template<class Client, class...Events>
struct event_emitting_receiver;

template <class Client, class ValueEvent>
struct event_emitting_receiver<Client, ValueEvent> : event_emitting_receiver_base<Client>, ValueEvent
{
    using event_emitting_receiver_base<Client>::event_emitting_receiver_base;

    template <class... Values>
    void set_value(Values &&...) {
        this->client_->connection_sm_->process_event(static_cast<ValueEvent &>(*this));
    }
};

template <class Client, class ValueEvent, class DoneEvent>
struct event_emitting_receiver<Client, ValueEvent, DoneEvent>
    : event_emitting_receiver_base<Client>, ValueEvent, DoneEvent
{
    template <class... Values>
    void set_value(Values &&...) {
        this->client_->connection_sm_->process_event(static_cast<ValueEvent &>(*this));
    }

    void set_done() {
        this->client_->connection_sm_->process_event(static_cast<DoneEvent &>(*this));
    }
};
}