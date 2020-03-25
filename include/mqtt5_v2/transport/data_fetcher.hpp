
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <optional>
#include <memory>

#include <nonstd/span.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <p0443_v2/set_done.hpp>
#include <p0443_v2/set_error.hpp>
#include <p0443_v2/set_value.hpp>

#include <p0443_v2/asio/read_some.hpp>
#include <p0443_v2/connect.hpp>
#include <p0443_v2/start.hpp>

namespace mqtt5_v2
{
namespace transport
{
template <class Stream>
class data_fetcher
{
private:
    Stream *stream;
    boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> *buffer;
public:
    void consume(std::size_t sz) {
        buffer->consume(sz);
    }
    nonstd::span<const std::uint8_t> cspan() {
        return {cdata(), ssize()};
    }

    const std::uint8_t *cdata() {
        return static_cast<const std::uint8_t *>(buffer->cdata().data());
    }

    std::size_t size() {
        return buffer->size();
    }

    std::ptrdiff_t ssize() {
        return std::ptrdiff_t(size());
    }

    template<class Predicate>
    struct data_reader
    {
        template <class Receiver>
        struct operation
        {
            struct read_some_receiver
            {
                operation *owner_;
                void set_value(std::size_t bytes_read) {
                    owner_->fetcher.buffer->commit(bytes_read);
                    owner_->start();
                }

                void set_done() {
                    p0443_v2::set_done((Receiver &&) owner_->next_);
                }

                template <class E>
                void set_error(E &&e) {
                    p0443_v2::set_error((Receiver &&) owner_->next_, std::forward<E>(e));
                }
            };
            data_fetcher fetcher;
            Predicate predicate_;
            Receiver next_;
            std::optional<
                p0443_v2::operation_type<p0443_v2::asio::read_some<Stream>, read_some_receiver>>
                read_some_op;

            void start() {
                try {
                    auto amount_requested = predicate_(fetcher);
                    if (amount_requested == 0) {
                        p0443_v2::set_value((Receiver &&) next_, fetcher);
                    }
                    else {
                        if(amount_requested < 64) {
                            amount_requested = 64;
                        }
                        read_some_op = p0443_v2::connect(
                            p0443_v2::asio::read_some(*fetcher.stream,
                                                    fetcher.buffer->prepare(amount_requested)),
                            read_some_receiver{this});
                        p0443_v2::start(*read_some_op);
                    }
                }
                catch(std::exception& e) {
                    (void)e;
                    p0443_v2::set_error((Receiver &&) next_, std::current_exception());
                }
            }
        };

        template <template <class...> class Tuple, template <class...> class Variant>
        using value_types = Variant<Tuple<data_fetcher>>;

        template <template <class...> class Variant>
        using error_types = Variant<std::exception_ptr>;

        static constexpr bool sends_done = true;

        data_fetcher fetcher;
        Predicate predicate_;

        template <class Receiver>
        auto connect(Receiver &&receiver) {
            return operation<p0443_v2::remove_cvref_t<Receiver>>{fetcher, std::move(predicate_),
                                                                 std::forward<Receiver>(receiver)};
        }
    };

    auto get_data(std::uint32_t bytes_requested) {
        return get_data_until([bytes_requested](data_fetcher fetcher) -> std::uint32_t {
            if(fetcher.size() < bytes_requested) {
                return static_cast<std::uint32_t>(bytes_requested - fetcher.size());
            }
            return 0;
        });
    }

    template<class Predicate, std::enable_if_t<std::is_invocable_v<Predicate, data_fetcher>>* = nullptr>
    auto get_data_until(Predicate&& pred) {
        using pred_type = p0443_v2::remove_cvref_t<Predicate>;
        using result_type = std::invoke_result_t<pred_type, data_fetcher>;
        static_assert(std::is_integral_v<result_type>, "Predicate must return number of additional bytes to read");
        return data_reader<pred_type>{*this, std::forward<Predicate>(pred)};
    }

    data_fetcher(Stream &stream,
                 boost::beast::basic_flat_buffer<std::allocator<std::uint8_t>> &buffer)
        : stream(& stream), buffer(& buffer) {
    }
};

template<class T>
struct buffered_data {
    using buffer_type = T;
};

template<class BufferType>
class data_fetcher<buffered_data<BufferType>>
{
private:
    using value_type = typename BufferType::value_type;
    static_assert(sizeof(value_type) == 1, "sizeof(BufferType::value_type) must be 1");

    BufferType *data_;

    template<class Predicate>
    struct data_reader
    {
        template <class Receiver>
        struct operation
        {
            data_fetcher fetcher;
            Predicate predicate_;
            Receiver next_;
    
            void start() {
                try {
                    auto amount_requested = predicate_(fetcher);
                    if (amount_requested == 0) {
                        p0443_v2::set_value((Receiver &&) next_, fetcher);
                    }
                    else {
                        // The buffer does not have a source for more data,
                        // so if the data available in the buffer isn't enough
                        // then we will simply set an error.
                        throw std::runtime_error("Attempt to read more data than available in buffer");
                    }
                }
                catch(std::exception& e) {
                    (void)e;
                    p0443_v2::set_error((Receiver &&) next_, std::current_exception());
                }
            }
        };
    
        template <template <class...> class Tuple, template <class...> class Variant>
        using value_types = Variant<Tuple<data_fetcher>>;
    
        template <template <class...> class Variant>
        using error_types = Variant<std::exception_ptr>;
    
        static constexpr bool sends_done = true;
    
        data_fetcher fetcher;
        Predicate predicate_;
    
        template <class Receiver>
        auto connect(Receiver &&receiver) {
            return operation<p0443_v2::remove_cvref_t<Receiver>>{fetcher, std::move(predicate_),
                                                                 std::forward<Receiver>(receiver)};
        }
    };

public:
    void consume(std::size_t sz) {
        if(sz >= size()) {
            data_->clear();
        }
        else {
            data_->erase(data_->begin(), data_->begin() + sz);
        }

    }
    nonstd::span<const std::uint8_t> cspan() const {
        return *data_;
    }

    const std::uint8_t *cdata() const {
        return data_->data();
    }

    std::size_t size() const {
        return data_->size();
    }

    std::ptrdiff_t ssize() const {
        return std::ptrdiff_t(size());
    }

    
    auto get_data(std::uint32_t bytes_requested) {
        return get_data_until([bytes_requested](data_fetcher fetcher) -> std::uint32_t {
            if(fetcher.size() < bytes_requested) {
                return static_cast<std::uint32_t>(bytes_requested - fetcher.size());
            }
            return 0;
        });
    }

    template<class Predicate, std::enable_if_t<std::is_invocable_v<Predicate, data_fetcher>>* = nullptr>
    auto get_data_until(Predicate&& pred) {
        using pred_type = p0443_v2::remove_cvref_t<Predicate>;
        using result_type = std::invoke_result_t<pred_type, data_fetcher>;
        static_assert(std::is_integral_v<result_type>, "Predicate must return number of additional bytes to read");
        return data_reader<pred_type>{*this, std::forward<Predicate>(pred)};
    }

    explicit data_fetcher(BufferType& data): data_(&data) {}

};

template<class BufferType>
using buffer_data_fetcher_t = data_fetcher<buffered_data<BufferType>>;

template<class BufferType>
buffer_data_fetcher_t<BufferType> buffer_data_fetcher(BufferType& buffer) {
    return buffer_data_fetcher_t<BufferType>(buffer);
}

} // namespace transport
} // namespace mqtt5_v2
