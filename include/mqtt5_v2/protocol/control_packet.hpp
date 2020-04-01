
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/mp11/algorithm.hpp>

#include <mqtt5_v2/protocol/connect.hpp>
#include <mqtt5_v2/protocol/ping.hpp>
#include <mqtt5_v2/protocol/header.hpp>
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>
#include <mqtt5_v2/protocol/publish.hpp>
#include <mqtt5_v2/protocol/subscribe.hpp>
#include <mqtt5_v2/protocol/unsubscribe.hpp>

#include <p0443_v2/just.hpp>
#include <p0443_v2/then.hpp>
#include <p0443_v2/type_traits.hpp>
#include <type_traits>
#include <vector>

#include <boost/mp11/list.hpp>
#include <p0443_v2/connect.hpp>
#include <variant>

namespace p0443_v2
{
template <class... SenderTypes>
struct one_of_sender
{
    std::variant<SenderTypes...> senders_;

    template <class T>
    one_of_sender(T &&t) : senders_(std::forward<T>(t)) {
    }

    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = boost::mp11::mp_unique<boost::mp11::mp_append<
        typename p0443_v2::sender_traits<SenderTypes>::template value_types<Tuple, Variant>...>>;

    template <template <class...> class Variant>
    using error_types = boost::mp11::mp_unique<boost::mp11::mp_append<
        typename p0443_v2::sender_traits<SenderTypes>::template error_types<Variant>...>>;

    static constexpr bool sends_done = std::disjunction<
        std::bool_constant<p0443_v2::sender_traits<SenderTypes>::sends_done>...>::value;

    template <class Receiver>
    struct operation
    {
        using next_operation_type =
            std::variant<p0443_v2::operation_type<SenderTypes, Receiver>...>;
        next_operation_type next_operation_;

        void start() {
            std::visit([](auto &op) { p0443_v2::start(op); }, next_operation_);
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        using op_type = operation<p0443_v2::remove_cvref_t<Receiver>>;
        return op_type{std::visit(
            [&](auto &&sender) -> typename op_type::next_operation_type {
                auto next_op = p0443_v2::connect(std::forward<decltype(sender)>(sender),
                                                 std::forward<Receiver>(receiver));
                return {p0443_v2::connect((decltype(sender) &&)sender, (Receiver &&)(receiver))};
            },
            std::move(senders_))};
    }
};
} // namespace p0443_v2

namespace mqtt5_v2::protocol
{
struct control_packet
{
private:
    using body_storage_type =
        std::variant<connect, connack, publish, puback, subscribe, suback, unsubscribe, unsuback, pingreq, pingresp>;
    template <class T>
    using is_body_type = std::bool_constant<boost::mp11::mp_find<body_storage_type, T>::value !=
                                            boost::mp11::mp_size<body_storage_type>::value>;

    header header_;
    body_storage_type body_;

public:
    control_packet() = default;

    template <class T, std::enable_if_t<is_body_type<T>::value> * = nullptr>
    T *body_as() & {
        return std::get_if<T>(&body_);
    }

    template <class T, std::enable_if_t<is_body_type<T>::value> * = nullptr>
    const T *body_as() const & {
        return std::get_if<T>(&body_);
    }

    template <class T, std::enable_if_t<is_body_type<T>::value> * = nullptr>
    std::optional<T> body_as() && {
        auto *ptr = std::get_if<T>(&body_);
        if (ptr) {
            return std::move(*ptr);
        }
        return {};
    }

    template <class T, std::enable_if_t<is_body_type<T>::value> * = nullptr>
    std::optional<T> body_as() const && {
        auto *ptr = std::get_if<T>(&body_);
        if (ptr) {
            return std::move(*ptr);
        }
        return {};
    }

    template <class T, std::enable_if_t<is_body_type<T>::value> * = nullptr>
    bool is_type() const {
        return std::get_if<T>(&body_) != nullptr;
    }

    body_storage_type &body() {
        return body_;
    }

    const body_storage_type &body() const {
        return body_;
    }

    std::uint8_t packet_type() const {
        return std::visit([](auto &p) { return p.type_value; }, body_);
    }

    template <class Packet,
              std::enable_if_t<boost::mp11::mp_find<body_storage_type, Packet>::value !=
                               boost::mp11::mp_size<body_storage_type>::value> * = nullptr>
    control_packet(Packet p) : body_(std::move(p)) {
    }

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        auto get_parse_body = p0443_v2::lazy([this, data_fetcher]() mutable {
            return p0443_v2::transform(
                data_fetcher.get_data(header_.remaining_length()),
                [this, data_fetcher](auto...) mutable {
                    auto packet_data = data_fetcher.cspan(header_.remaining_length());
                    auto buffer_fetcher = transport::buffer_data_fetcher(packet_data);

                    if (header_.type() == connect::type_value) {
                        body_.template emplace<connect>(std::in_place, buffer_fetcher);
                    }
                    else if (header_.type() == connack::type_value) {
                        body_.template emplace<connack>(std::in_place, buffer_fetcher);
                    }
                    else if (header_.type() == publish::type_value) {
                        body_.template emplace<publish>(std::in_place, header_, buffer_fetcher);
                    }
                    else if (header_.type() == puback::type_value) {
                        body_.template emplace<puback>(std::in_place, header_.remaining_length(),
                                                       buffer_fetcher);
                    }
                    else if (header_.type() == subscribe::type_value) {
                        body_.template emplace<subscribe>(std::in_place, header_, buffer_fetcher);
                    }
                    else if(header_.type() == suback::type_value) {
                        body_.template emplace<suback>(std::in_place, header_, buffer_fetcher);
                    }
                    else if (header_.type() == unsubscribe::type_value) {
                        body_.template emplace<unsubscribe>(std::in_place, header_, buffer_fetcher);
                    }
                    else if(header_.type() == unsuback::type_value) {
                        body_.template emplace<unsuback>(std::in_place, header_, buffer_fetcher);
                    }
                    else if(header_.type() == pingreq::type_value) {
                        body_.template emplace<pingreq>();
                    }
                    else if(header_.type() == pingresp::type_value) {
                        body_.template emplace<pingresp>();
                    }
                    else {
                        throw std::runtime_error("Received unknown control packet type");
                    }

                    data_fetcher.consume(header_.remaining_length());
                });
        });

        return p0443_v2::sequence(header_.inplace_deserializer(data_fetcher),
                                  std::move(get_parse_body));
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        std::visit([&](auto &d) { d.serialize(writer); }, body_);
    }
};
} // namespace mqtt5_v2::protocol
