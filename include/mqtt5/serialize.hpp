#pragma once

#include <cstdint>
#include <vector>

#include <boost/stl_interfaces/iterator_interface.hpp>

namespace mqtt5
{
namespace detail
{
struct sink
{
    template<class T>
    sink& operator=(const T&) {
        return *this;
    }
};

template<class Iter>
void throw_if_empty(Iter begin, Iter end)
{
    if(begin == end)
    {
        throw std::length_error("not enough data to deserialize");
    }
}
}
struct count_iterator
    : boost::stl_interfaces::iterator_interface<count_iterator, std::output_iterator_tag,
        void, detail::sink>
{
    count_iterator& operator++() {
        return *this;
    }
    detail::sink operator*() {
        count++;
        return {};
    }
    std::size_t count = 0;
};

template<class Iter>
Iter deserialize_into(std::uint8_t &byte, Iter begin, Iter end)
{
    if(begin == end) {
        throw std::length_error("not enough data to deserialize");
    }
    byte = *begin;
    ++begin;
    return begin;
}

template<class Iter>
Iter deserialize_into(std::vector<std::uint8_t> &data, std::size_t cnt, Iter begin, Iter end)
{
    auto data_left = end-begin;
    if(data_left < cnt) {
        throw std::length_error("not enough data to deserialize vector");
    }
    data.reserve(sz);
    return std::copy(begin, begin+cnt, std::back_inserter(data));
}
} // namespace mqtt5