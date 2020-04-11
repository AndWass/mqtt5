
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

namespace mqtt5
{
enum class puback_reason_code : std::uint8_t {
    success = 0,
    no_matching_subscribers = 16,
    unspecified = 128,
    implementation_specific_error = 131,
    not_authorized = 135,
    topic_name_invalid = 144,
    packet_identifier_in_use = 145,
    quota_exceeded = 151,
    payload_format_invalid = 153
};
}