#pragma once

#include <cstdint>

#include "string.hpp"

namespace mqtt5
{

namespace detail
{
template<class Item, class Iter>
Iter try_deserialize(Item &item, Iter begin, Iter end, int &state) {
    auto retval = deserialize_into(item, begin, end);
    state++;
    return retval;
}
}

class connect
{
public:
    string protocol_name;
    std::uint8_t protocol_version;
    std::uint8_t connect_flags;

    struct deserializer;
private:
};

struct connect::deserializer
{
    int state_ = 0;
    connect result;
    template<class Iter>
    Iter operator()(Iter begin, Iter end) {
        switch(state_)
        {
        case 0:
            begin = detail::try_deserialize(result.protocol_name, begin, end);
            [[fallthrough]]
        case 1:
            begin = detail::try_deserialize(result.protocol_version, begin, end);
            [[fallthrough]]
        case 2:
            begin = detail::try_deserialize(result.connect_flags, begin, end);
            [[fallthrough]]
        }
        return begin;
    }
};
}