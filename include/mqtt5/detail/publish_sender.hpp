
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "mqtt5/quality_of_service.hpp"
#include <mqtt5/protocol/publish.hpp>

namespace mqtt5::detail
{
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

template <class Client, class Modifier>
struct publish_sender
{
    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<mqtt5::puback_reason_code>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    std::vector<std::uint8_t> payload;
    std::string topic_;
    Modifier modifying_function_;
    Client *client_;
    quality_of_service qos_ = 0_qos;

    publish_sender(Client *client, Modifier modifier)
        : modifying_function_(std::move(modifier)), client_(client) {
    }

    template <class Receiver>
    struct operation
    {
        Receiver receiver_;

        std::string topic_;
        std::vector<std::uint8_t> payload;
        Modifier modifying_function_;
        Client *client_;
        quality_of_service qos_;

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

            if (publish.quality_of_service() != 0_qos) {
                publish.packet_identifier = client_->next_packet_identifier();
            }

            client_->send_message(publish);

            if (publish.quality_of_service() != 0_qos) {
                // Store message for further processing
                in_flight_publish stored{std::move(publish),
                                         std::make_unique<publish_receiver>(std::move(receiver_))};

                client_->published_messages_.emplace_back(std::move(stored));
            }
            else {
                p0443_v2::set_value(std::move(receiver_), puback_reason_code::success);
            }
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        return operation<p0443_v2::remove_cvref_t<Receiver>>{
            std::forward<Receiver>(receiver), topic_, payload, modifying_function_, client_, qos_};
    }
};

template <class Client, class Modifier>
struct reusable_publish_sender
{
    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<mqtt5::puback_reason_code>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    std::vector<std::uint8_t> payload;
    std::string topic_;
    Modifier modifying_function_;
    Client *client_;
    quality_of_service qos_ = 0_qos;

    reusable_publish_sender(Client *client, Modifier modifier)
        : modifying_function_(std::move(modifier)), client_(client) {
    }

    reusable_publish_sender(const reusable_publish_sender&) = default;
    reusable_publish_sender(reusable_publish_sender&& rhs): reusable_publish_sender(rhs) {}

    reusable_publish_sender& operator=(const reusable_publish_sender&) =default;
    reusable_publish_sender& operator=(reusable_publish_sender&& rhs) {
        if(this != &rhs) {
            *this = rhs;
        }
        return *this;
    }
    ~reusable_publish_sender() = default;

    template <class Receiver>
    struct operation
    {
        Receiver receiver_;

        std::string topic_;
        std::vector<std::uint8_t> payload;
        Modifier modifying_function_;
        Client *client_;
        quality_of_service qos_;

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

            if (publish.quality_of_service() != 0_qos) {
                publish.packet_identifier = client_->next_packet_identifier();
            }

            client_->send_message(publish);

            if (publish.quality_of_service() != 0_qos) {
                // Store message for further processing
                in_flight_publish stored{std::move(publish),
                                         std::make_unique<publish_receiver>(std::move(receiver_))};

                client_->published_messages_.emplace_back(std::move(stored));
            }
            else {
                p0443_v2::set_value(std::move(receiver_), puback_reason_code::success);
            }
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        return operation<p0443_v2::remove_cvref_t<Receiver>>{
            std::forward<Receiver>(receiver), topic_, payload, modifying_function_, client_, qos_};
    }
};
} // namespace mqtt5::detail