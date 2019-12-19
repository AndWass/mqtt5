#pragma once

#include <cstdint>

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
} // namespace mqtt5