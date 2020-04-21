
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "message_receiver_base.hpp"
#include <exception>
#include <mqtt5/protocol/publish.hpp>
#include <mqtt5/topic_filter.hpp>

#include <algorithm>
#include <p0443_v2/type_traits.hpp>
#include <vector>

namespace mqtt5::detail
{
struct filtered_subscription
{
    using receiver_type = detail::message_receiver_base<protocol::publish>;
    topic_filter filter_;
    std::vector<std::unique_ptr<receiver_type>> receivers_;
};

template <class Client>
struct filter_subscribe_sender
{
    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<protocol::publish>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    Client *client_;
    topic_filter filter_;

    template <class Receiver>
    struct operation
    {
        Client *client_;
        topic_filter filter_;
        Receiver receiver_;

        struct receiver: filtered_subscription::receiver_type
        {
            Receiver receiver_;

            receiver(Receiver recv): receiver_(std::move(recv)) {}

            void set_value(protocol::publish pub) override {
                p0443_v2::set_value(std::move(receiver_), std::move(pub));
            }
            void set_done() override {
                p0443_v2::set_done(std::move(receiver_));
            }
            void set_error(std::exception_ptr ex) override {
                p0443_v2::set_error(std::move(receiver_), std::move(ex));
            }
        };

        void start() {
            auto existing_item = std::find_if(
                client_->publish_waiters_.begin(), client_->publish_waiters_.end(),
                [this](const filtered_subscription &item) { return item.filter_ == filter_; });
            if(existing_item != client_->publish_waiters_.end()) {
                existing_item->receivers_.emplace_back(std::make_unique<receiver>(std::move(receiver_)));
            }
            else {
                filtered_subscription new_item;
                new_item.filter_ = std::move(filter_);
                new_item.receivers_.emplace_back(std::make_unique<receiver>(std::move(receiver_)));
                client_->publish_waiters_.emplace_back(std::move(new_item));
            }
        }
    };

    template<class Receiver>
    auto connect(Receiver&& rx) {
        using receiver_t = p0443_v2::remove_cvref_t<Receiver>;
        return operation<receiver_t>{client_, std::move(filter_), std::forward<Receiver>(rx)};
    }
};
template<class T>
filter_subscribe_sender(T*, mqtt5::topic_filter) -> filter_subscribe_sender<T>;
} // namespace mqtt5::detail