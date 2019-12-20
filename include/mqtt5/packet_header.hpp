#pragma once

#include <vector>
#include <cstdint>

#include "integer.hpp"

namespace mqtt5
{
struct packet_header
{
    enum class type
    {
        reserved,
        connect,
        connack,
        publish,
        puback,
        pubrec,
        pubrel,
        pubcomp,
        subscribe,
        suback,
        unsubscribe,
        unsuback,
        pingreq,
        pingresp,
        disconnect,
        auth
    };

    type packet_type = type::reserved;
    std::uint8_t flags = 0;
};

template<class Iter>
Iter serialize(const packet_header& ph, Iter out)
{
    std::uint8_t first_byte = (static_cast<std::uint8_t>(ph.packet_type) << 4) | (ph.flags & 0x0f);
    *out = first_byte;
    ++out;

    varlen_integer remaining_length(ph.data.size());

    out = serialize(remaining_length, out);
    return std::copy(ph.data.begin(), ph.data.end(), out);
}

template<class Iter>
Iter deserialize_into(packet_header& ph, Iter begin, Iter end)
{
    std::uint8_t first_byte;
    begin = mqtt5::deserialize_into(first_byte, begin, end);

    varlen_integer remaining_length;
    begin = deserialize_into(remaining_length, begin, end);
    ph.packet_type = static_cast<packet_header::type>(first_byte >> 4);
    ph.flags = first_byte & 0x0f;
    return mqtt5::deserialize_into(ph.data, remaining_length.value(), begin, end);
}

}