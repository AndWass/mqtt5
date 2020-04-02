
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5/transport/data_fetcher.hpp>

#include <optional>
#include <variant>

#include <p0443_v2/connect.hpp>
#include <p0443_v2/set_value.hpp>
#include <p0443_v2/start.hpp>
#include <p0443_v2/transform.hpp>

namespace mqtt5::protocol
{
namespace detail
{
template <class T, class Stream>
struct optional_inplace_deserializer_sender
{
    using data_fetcher_type = transport::data_fetcher<Stream>;

    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<data_fetcher_type>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    std::optional<T> *val_;
    data_fetcher_type data_fetcher_;

    template <class Receiver>
    struct operation
    {
        using next_sender_type =
            decltype(std::declval<T &>().inplace_deserializer(std::declval<data_fetcher_type>()));

        std::variant<Receiver, p0443_v2::operation_type<next_sender_type, Receiver>> next_;
        data_fetcher_type data_fetcher_;
        std::optional<T> *val_;

        void start() {
            if (val_->has_value()) {
                auto &ref = val_->value();
                auto recv_ = std::move(std::get<0>(next_));
                auto &next = next_.template emplace<1>(
                    p0443_v2::connect(ref.inplace_deserializer(data_fetcher_), std::move(recv_)));
                p0443_v2::start(next);
            }
            else {
                p0443_v2::set_value(std::get<0>(std::move(next_)), data_fetcher_);
            }
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        using recv_type = p0443_v2::remove_cvref_t<Receiver>;
        return operation<recv_type>{std::forward<Receiver>(receiver), data_fetcher_, val_};
    }
};
} // namespace detail

template <class T, class Stream>
auto inplace_deserializer(T &val, transport::data_fetcher<Stream> data_fetcher)
    -> decltype(val.inplace_deserializer(data_fetcher)) {
    return val.inplace_deserializer(data_fetcher);
}

template <class T, class Stream>
auto inplace_deserializer(std::optional<T> &val, transport::data_fetcher<Stream> data_fetcher) {
    return detail::optional_inplace_deserializer_sender<T, Stream>{std::addressof(val),
                                                                   data_fetcher};
}

template <class Stream>
auto inplace_deserializer(std::vector<std::uint8_t> &val,
                          transport::data_fetcher<Stream> data_fetcher) {
    return p0443_v2::transform(
        data_fetcher.get_data(static_cast<std::uint32_t>(val.size())), [&val](auto fetcher) {
            auto data_span = fetcher.cspan();
            std::copy(data_span.begin(), data_span.begin() + val.size(), val.begin());
            fetcher.consume(val.size());
            return fetcher;
        });
}

template <class T, class Fetcher>
using inplace_deserializer_for =
    decltype(inplace_deserializer(std::declval<T &>(), std::declval<Fetcher>()));

} // namespace mqtt5::protocol
