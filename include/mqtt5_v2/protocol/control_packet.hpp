
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/mp11/algorithm.hpp>

#include <mqtt5_v2/protocol/connack.hpp>
#include <mqtt5_v2/protocol/connect.hpp>
#include <mqtt5_v2/protocol/publish.hpp>
#include <mqtt5_v2/protocol/header.hpp>
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>

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

    static constexpr bool sends_done = (p0443_v2::sender_traits<SenderTypes>::sends_dones || ...);

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

#include <p0443_v2/sink_receiver.hpp>

namespace mqtt5_v2::protocol
{
struct control_packet
{
private:
    using body_storage_type = std::variant<connect, connack, publish>;
    template<class T>
    using is_body_type = std::bool_constant<boost::mp11::mp_find<body_storage_type, T>::value !=
                               boost::mp11::mp_size<body_storage_type>::value>;

    header header_;
    body_storage_type body_;
public:
    control_packet() = default;

    template<class T, std::enable_if_t<is_body_type<T>::value>* = nullptr>
    T* body_as() {
        return std::get_if<T>(&body_);
    }

    template<class T, std::enable_if_t<is_body_type<T>::value>* = nullptr>
    const T* body_as() const {
        return std::get_if<T>(&body_);
    }

    template<class T, std::enable_if_t<is_body_type<T>::value>* = nullptr>
    bool is_type() const {
        return body_as<T>() != nullptr;
    }

    body_storage_type& body() {
        return body_;
    }

    const body_storage_type& body() const {
        return body_;
    }

    template <class Packet,
              std::enable_if_t<boost::mp11::mp_find<body_storage_type, Packet>::value !=
                               boost::mp11::mp_size<body_storage_type>::value> * = nullptr>
    control_packet(Packet p) : body_(std::move(p)) {
    }

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data_fetcher) {
        using bound_inplace_deserializer_for =
            boost::mp11::mp_bind_back<inplace_deserializer_for, decltype(data_fetcher)>;
        using return_type = boost::mp11::mp_append<
            p0443_v2::one_of_sender<>,
            boost::mp11::mp_transform<bound_inplace_deserializer_for::template fn,
                                      body_storage_type>>;
        
        auto body_deserializer = [this, data_fetcher](auto fetcher) {
            if (header_.type() == 1) {
                body_.template emplace<0>();
            }
            else if (header_.type() == 2) {
                body_.template emplace<1>();
            }
            else if (header_.type() == 3) {
                body_.template emplace<2>(std::in_place, header_.flags(), header_.remaining_length());
            }

            return std::visit(
                [this, data_fetcher](auto &p) {
                    return return_type(p.inplace_deserializer(data_fetcher));
                },
                body_);
        };

        return p0443_v2::then(header_.inplace_deserializer(data_fetcher),
                              body_deserializer);
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        std::visit(
            [&](auto &d) {
                d.serialize(writer);
            },
            body_);
    }
};
} // namespace mqtt5_v2::protocol
