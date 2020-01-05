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
private:
    void set_connect(const connect &conn) {
        type = packet_type::connect;
        flags = 0;
        data.clear();
        (void)serialize(conn, std::back_inserter(data));
    }
public:
    raw() = default;
    raw(const connect &conn) {
        set_connect(conn);
    }
    packet_type type = packet_type::reserved;
    std::uint8_t flags = 0;
    std::vector<std::uint8_t> data;

    std::vector<std::uint8_t> to_bytes() const noexcept;

    raw &operator=(const connect &rhs) {
        set_connect(rhs);
        return *this;
    }

    template <class T,
              std::void_t<decltype(deserialize_into(
                  std::declval<T &>(), std::declval<std::vector<std::uint8_t>::const_iterator>(),
                  std::declval<std::vector<std::uint8_t>::const_iterator>()))> * = nullptr>
    T convert_to() const noexcept {
        T retval;
        (void)deserialize_into(retval, data.begin(), data.end());
        return retval;
    }
};

template <class Iter>
[[nodiscard]] Iter serialize(const raw &ph, Iter out) {
    std::uint8_t first_byte = (static_cast<std::uint8_t>(ph.type) << 4) | (ph.flags & 0x0f);
    *out = first_byte;
    ++out;

    type::varlen_integer remaining_length(static_cast<std::uint32_t>(ph.data.size()));

    out = type::serialize(remaining_length, out);
    return std::copy(ph.data.begin(), ph.data.end(), out);
}

template <class Iter>
[[nodiscard]] Iter deserialize_into(raw &ph, Iter begin, Iter end) {
    std::uint8_t first_byte;
    begin = type::deserialize_into(first_byte, begin, end);

    type::varlen_integer remaining_length;
    begin = type::deserialize_into(remaining_length, begin, end);
    ph.type = static_cast<packet_type>(first_byte >> 4);
    ph.flags = first_byte & 0x0f;
    return type::deserialize_into(ph.data, remaining_length.value(), begin, end);
}

inline std::vector<std::uint8_t> raw::to_bytes() const noexcept {
    std::vector<std::uint8_t> retval;
    (void)message::serialize(*this, std::back_inserter(retval));
    return retval;
}
} // namespace message
} // namespace mqtt5