#pragma once

#include <cstdint>
#include <vector>

#include "connect.hpp"
#include "mqtt5/type/integer.hpp"
#include "packet_type.hpp"

namespace mqtt5
{
namespace message
{
struct raw
{
    packet_type type = packet_type::reserved;
    std::uint8_t flags = 0;
    std::vector<std::uint8_t> data;

    std::vector<std::uint8_t> to_bytes() const noexcept;

    raw &operator=(const mqtt5::connect &rhs) {
        using namespace mqtt5;
        using namespace mqtt5::type;
        type = packet_type::connect;
        flags = 0;
        data.clear();
        (void)serialize(rhs, std::back_inserter(data));
        return *this;
    }

    template <class T,
              std::void_t<decltype(mqtt5::deserialize_into(
                  std::declval<T &>(), std::declval<std::vector<std::uint8_t>::const_iterator>(),
                  std::declval<std::vector<std::uint8_t>::const_iterator>()))> * = nullptr>
    T convert_to() const noexcept {
        T retval;
        (void)mqtt5::deserialize_into(retval, data.begin(), data.end());
        return retval;
    }
};

template <class Iter>
[[nodiscard]] Iter serialize(const raw &ph, Iter out) {
    using namespace mqtt5;
    using namespace mqtt5::type;

    std::uint8_t first_byte = (static_cast<std::uint8_t>(ph.type) << 4) | (ph.flags & 0x0f);
    *out = first_byte;
    ++out;

    varlen_integer remaining_length(static_cast<std::uint32_t>(ph.data.size()));

    out = serialize(remaining_length, out);
    return std::copy(ph.data.begin(), ph.data.end(), out);
}

template <class Iter>
Iter deserialize_into(raw &ph, Iter begin, Iter end) {
    using namespace mqtt5;
    using namespace mqtt5::type;
    std::uint8_t first_byte;
    begin = deserialize_into(first_byte, begin, end);

    varlen_integer remaining_length;
    begin = deserialize_into(remaining_length, begin, end);
    ph.type = static_cast<packet_type>(first_byte >> 4);
    ph.flags = first_byte & 0x0f;
    return mqtt5::deserialize_into(ph.data, remaining_length.value(), begin, end);
}

std::vector<std::uint8_t> raw::to_bytes() const noexcept {
    using namespace mqtt5;
    using namespace mqtt5::type;
    std::vector<std::uint8_t> retval;
    (void)serialize(*this, std::back_inserter(retval));
    return retval;
}
} // namespace message
} // namespace mqtt5