#pragma once

#include <boost/stl_interfaces/container_interface.hpp>
#include <vector>

#include "integer.hpp"

namespace mqtt5
{
namespace detail
{
class binary_validation
{
    void validate(std::size_t sz) {
        if (sz > max_size()) {
            throw std::length_error("Maximum binary size is 65535");
        }
    }

public:
    binary_validation() noexcept = default;
    binary_validation(std::size_t count) {
        validate(count);
    };

    template <class Iter>
    binary_validation(Iter begin, Iter end) {
        validate(std::distance(begin, end));
    }

    explicit binary_validation(const std::vector<std::uint8_t> &data) {
        validate(data.size());
    }

    constexpr std::size_t max_size() const noexcept {
        return 65535;
    }
};
} // namespace detail

class binary : public boost::stl_interfaces::container_interface<binary, true>,
               detail::binary_validation
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
        return storage_.emplace(pos, std::forward<Args>(args)...);
    }

    template <class Iter>
    iterator insert(const_iterator pos, Iter begin, Iter end) {
        return storage_.insert(pos, begin, end);
    }

    iterator erase(const_iterator first, const_iterator last) noexcept {
        return storage_.erase(first, last);
    }

    template <class... Args>
    reference emplace_back(Args &&... args) {
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
Iter serialize(const binary &bin, Iter out) {
    integer16 size(bin.size());
    out = serialize(size, out);
    return std::copy(bin.begin(), bin.end(), out);
}

} // namespace mqtt5