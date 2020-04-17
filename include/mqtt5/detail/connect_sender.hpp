
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <p0443_v2/set_done.hpp>
#include <p0443_v2/set_error.hpp>
#include <p0443_v2/set_value.hpp>

#include "message_receiver_base.hpp"

namespace mqtt5::detail
{
template <class Client>
struct connect_sender
{
    Client *client_;

    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    struct operation
    {
        Client *client_;
        void start() {
            if (client_->is_connected()) {
                client_->notify_connector_receivers(true);
            }
            else {
                client_->connection_sm_->process_event(
                    typename Client::connection_sm_t::handshake_evt{});
            }
        }
    };

    template <class Receiver>
    struct receiver_impl : detail::message_receiver_base<>
    {
        p0443_v2::remove_cvref_t<Receiver> next_;

        ~receiver_impl() = default;
        receiver_impl(Receiver recv) : next_(std::move(recv)) {
        }
        void set_value() override {
            p0443_v2::set_value(std::move(next_));
        }
        void set_done() override {
            p0443_v2::set_done(std::move(next_));
        }
        void set_error(std::exception_ptr e) override {
            p0443_v2::set_error(std::move(next_), std::move(e));
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        client_->connect_receivers_.emplace_back(new receiver_impl<Receiver>{std::move(receiver)});
        return operation{client_};
    }
};

template<class T>
connect_sender(T*) -> connect_sender<T>;
} // namespace mqtt5::detail