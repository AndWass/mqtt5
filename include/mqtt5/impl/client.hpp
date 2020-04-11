
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5/client.hpp>

#include "client_macro_begin.hpp"

namespace mqtt5
{

#include "connect_sender.hpp"
#include "subscriber_token.hpp"

CLIENT_TEMPLATE
struct CLIENT::receiver_base
{
    client *client_;
    void set_done() {
    }
    template <class E>
    void set_error(E &&e) {
        // Do this more graceful!
        std::terminate();
    }
};

CLIENT_TEMPLATE
template <class ValueEvent>
struct CLIENT::event_emitting_receiver<ValueEvent> : receiver_base, ValueEvent
{
    template <class... Values>
    void set_value(Values &&...) {
        this->client_->connection_sm_->process_event(static_cast<ValueEvent &>(*this));
    }
};

CLIENT_TEMPLATE
template <class ValueEvent, class DoneEvent>
struct CLIENT::event_emitting_receiver<ValueEvent, DoneEvent>
    : receiver_base, ValueEvent, DoneEvent
{
    template <class... Values>
    void set_value(Values &&...) {
        this->client_->connection_sm_->process_event(static_cast<ValueEvent &>(*this));
    }

    void set_done() {
        this->client_->connection_sm_->process_event(static_cast<DoneEvent &>(*this));
    }
};
} // namespace mqtt5

#include "client_macro_end.hpp"