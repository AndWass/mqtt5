
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5_v2/protocol/binary.hpp>
#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/string.hpp>
#include <mqtt5_v2/protocol/varlen_int.hpp>

#include <p0443_v2/start.hpp>

#include <boost/mp11/algorithm.hpp>

#include <tuple>
#include <variant>
#include <vector>

namespace mqtt5_v2::protocol
{
struct property
{
    using value_storage =
        std::variant<fixed_int<std::uint8_t>, fixed_int<std::uint16_t>, fixed_int<std::uint32_t>,
                     varlen_int, string, binary, key_value_pair>;

    template <class Stream>
    struct value_deserialize_sender
    {
        template <template <class...> class Tuple, template <class...> class Variant>
        using value_types = Variant<Tuple<transport::data_fetcher<Stream>>>;

        template <template <class...> class Variant>
        using error_types = Variant<std::exception_ptr>;

        static constexpr bool sends_done = false;

        property *prop_;
        transport::data_fetcher<Stream> data_;

        template <class Receiver>
        struct operation
        {
            template <class V>
            using v_to_op_transform =
                p0443_v2::operation_type<decltype(std::declval<V>().inplace_deserializer(
                                             std::declval<transport::data_fetcher<Stream>>())),
                                         Receiver>;

            using op_storage_t = boost::mp11::mp_transform<v_to_op_transform, value_storage>;

            property *prop_;
            Receiver next_;
            transport::data_fetcher<Stream> data_;
            std::optional<op_storage_t> next_op_storage_;

            void start() {
                prop_->activate_id(prop_->identifier.value);
                std::visit(
                    [this](auto &value) {
                        next_op_storage_.emplace(
                            p0443_v2::connect(value.inplace_deserializer(data_), std::move(next_)));
                    },
                    prop_->value);
                std::visit([](auto &op) { p0443_v2::start(op); }, *next_op_storage_);
            }
        };
        template <class Receiver>
        auto connect(Receiver &&r) {
            using recv_t = p0443_v2::remove_cvref_t<Receiver>;
            return operation<recv_t>{prop_, std::forward<Receiver>(r), data_};
        }
    };

    template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data) {
        return p0443_v2::sequence(identifier.inplace_deserializer(data),
                                  value_deserialize_sender<Stream>{this, data});
    }

    void activate_id(std::uint8_t id) {
#define ACTIVATE(X)                                                                                \
    value.emplace<X>();                                                                            \
    break
        switch (id) {
        case 1:
            ACTIVATE(0);
        case 2:
            ACTIVATE(2);
        case 3:
            ACTIVATE(4);
        case 8:
            ACTIVATE(4);
        case 9:
            ACTIVATE(5);
        case 11:
            ACTIVATE(5);
        case 17:
            ACTIVATE(2);
        case 18:
            ACTIVATE(4);
        case 19:
            ACTIVATE(1);
        case 21:
            ACTIVATE(4);
        case 22:
            ACTIVATE(5);
        case 23:
            ACTIVATE(0);
        case 24:
            ACTIVATE(2);
        case 25:
            ACTIVATE(0);
        case 26:
            ACTIVATE(4);
        case 28:
            ACTIVATE(4);
        case 31:
            ACTIVATE(4);
        case 33:
            ACTIVATE(1);
        case 34:
            ACTIVATE(1);
        case 35:
            ACTIVATE(1);
        case 36:
            ACTIVATE(0);
        case 37:
            ACTIVATE(0);
        case 38:
            ACTIVATE(6);
        case 39:
            ACTIVATE(2);
        case 40:
            ACTIVATE(0);
        case 41:
            ACTIVATE(0);
        case 42:
            ACTIVATE(0);
        }
#undef ACTIVATE
    }

    varlen_int identifier;
    value_storage value;

    nonstd::span<const std::uint8_t> set_from_bytes(nonstd::span<const std::uint8_t> data) {
        data = identifier.set_from_bytes(data);
        activate_id(identifier.value);
        return std::visit([&](auto &val) { return val.set_from_bytes(data); }, value);
    }
};
struct properties
{
    template <class Stream>
    auto implace_deserializer(transport::data_fetcher<Stream> data) {
        auto length_deserializer = properties_length_.inplace_deserializer(data);
        auto storage_resizer_ = p0443_v2::then(std::move(length_deserializer),
                                               [this](transport::data_fetcher<Stream> data) {
                                                   storage_.emplace<0>(properties_length_.value);
                                                   return data.get_data(properties_length_.value);
                                               });
        return p0443_v2::transform(std::move(storage_resizer_), [this](auto data) {
            this->from_bytes(std::get<0>(this->storage_));
            return data;
        });
    }

    nonstd::span<const std::uint8_t> from_bytes(nonstd::span<const std::uint8_t> data) {
        std::vector<property> new_properties;
        while(!data.empty()) {
            new_properties.emplace_back();
            data = new_properties.back().set_from_bytes(data);
        }
        storage_ = std::move(new_properties);
        return data;
    }

private:
    varlen_int properties_length_;
    std::variant<std::vector<std::uint8_t>, std::vector<property>> storage_;
};
} // namespace mqtt5_v2::protocol