#pragma once

#include <boost/stl_interfaces/container_interface.hpp>
#include <vector>

#include "integer.hpp"

namespace mqtt5
{
namespace type
{
namespace detail
{
class binary_validation
{
public:
    void validate_size(std::size_t sz) {
        if (sz > max_size()) {
            throw std::length_error("Maximum binary size is 65535");
        }
    }

    binary_validation() noexcept = default;
    binary_validation(std::size_t count) {
        validate_size(count);
    };

    template <class Iter>
    binary_validation(Iter begin, Iter end) {
        validate_size(std::distance(begin, end));
    }

    explicit binary_validation(const std::vector<std::uint8_t> &data) {
        validate_size(data.size());
    }

    constexpr std::size_t max_size() const noexcept {
        return 65535;
    }
};
} // namespace detail

class binary : public boost::stl_interfaces::container_interface<binary, true>,
               private detail::binary_validation
{
public:
    using value_type = std::uint8_t;
    using reference = std::uint8_t &;
    using const_reference = const std::uint8_t &;
    using iterator = std::vector<std::uint8_t>::iterator;
    using const_iterator = std::vector<std::uint8_t>::const_iterator;
    using reverse_iterator = boost::stl_interfaces::reverse_iterator<iterator>;
    using const_reverse_iterator = boost::stl_interfaces::reverse_iterator<const_iterator>;
    using difference_type = std::vector<std::uint8_t>::difference_type;
    using size_type = std::vector<std::uint8_t>::size_type;

    binary() noexcept = default;
    binary(size_type count, std::uint8_t val)
        : detail::binary_validation(count), storage_(count, val){};

    template <class Iter>
    binary(Iter begin, Iter end) : detail::binary_validation(begin, end), storage_(begin, end) {
    }

    explicit binary(const std::vector<std::uint8_t> &data)
        : detail::binary_validation(data), storage_(data) {
    }
    explicit binary(std::vector<std::uint8_t> &&data)
        : detail::binary_validation(data), storage_(std::move(data)) {
    }

    binary &operator=(const std::vector<std::uint8_t> &data) {
        storage_ = data;
        return *this;
    }

    binary &operator=(std::vector<std::uint8_t> &&data) noexcept(
        noexcept(std::declval<std::vector<std::uint8_t> &>() = std::move(data))) {
        storage_ = std::move(data);
        return *this;
    }

    iterator begin() noexcept {
        return storage_.begin();
    }

    iterator end() noexcept {
        return storage_.end();
    }

    bool operator==(const binary &rhs) {
        return storage_ == rhs.storage_;
    }

    void swap(binary &rhs) {
        storage_.swap(rhs.storage_);
    }

    constexpr size_type max_size() const noexcept {
        return binary_validation::max_size();
    }

    explicit operator std::vector<std::uint8_t>() {
        return storage_;
    }

    template <class... Args>
    iterator emplace(const_iterator pos, Args &&... args) {
        binary_validation::validate_size(size() + 1);
        return storage_.emplace(pos, std::forward<Args>(args)...);
    }

    template <class... Args>
    iterator emplace_front(Args &&... args) {
        binary_validation::validate_size(size() + 1);
        return storage_.emplace(begin(), std::forward<Args>(args)...);
    }

    template <class Iter>
    iterator insert(const_iterator pos, Iter begin, Iter end) {
        binary_validation::validate_size(size() + (end - begin));
        return storage_.insert(pos, begin, end);
    }

    iterator erase(const_iterator first, const_iterator last) noexcept(
        noexcept(std::declval<std::vector<value_type>>().erase(first, last))) {
        return storage_.erase(first, last);
    }

    template <class... Args>
    reference emplace_back(Args &&... args) {
        binary_validation::validate_size(size() + 1);
        return storage_.emplace_back(std::forward<Args>(args)...);
    }

    void resize(size_type new_size, value_type value) {
        storage_.resize(new_size, value);
    }

    using base_type = boost::stl_interfaces::container_interface<binary, true>;
    using base_type::begin;
    using base_type::end;
    using base_type::erase;
    using base_type::insert;
    using base_type::resize;

private:
    std::vector<std::uint8_t> storage_;
};

template <class Iter>
[[nodiscard]] Iter serialize(const binary &bin, Iter out) {
    using mqtt5::type::serialize;
    integer16 size(static_cast<std::uint16_t>(bin.size()));
    out = serialize(size, out);
    return std::copy(bin.begin(), bin.end(), out);
}

template <class Iter>
[[nodiscard]] Iter deserialize_into(binary &bin, Iter begin, Iter end) {
    integer16 size;
    begin = deserialize_into(size, begin, end);
    auto data_left = end - begin;
    if (data_left < size.value()) {
        throw std::length_error("not enough data to deserialize a string");
    }
    bin = binary(begin, begin + size.value());
    return begin + size.value();
}
} // namespace type
} // namespace mqtt5