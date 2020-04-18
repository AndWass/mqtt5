
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstdint>

namespace mqtt5
{
enum class disconnect_reason: std::uint8_t
{
normal,
with_will = 4,
unspecified_error = 128,
malformed_packet,
protocol_error,
implementation_specific_error,
not_authorized = 135,
server_busy = 137,
server_shutting_down = 139,
keep_alive_timeout = 141,
session_taken_over = 142,
topic_filter_invalid = 143,
toipc_name_invalid = 144,
receiver_maximum_exceeded = 147,
topic_alias_invalid = 148,
packet_too_large = 149,
message_rate_too_high = 150,
quota_exceeded = 151,
administrative_action = 152,
payload_format_invalid = 153,
retain_not_supported = 154,
qos_not_supported = 155,
use_another_server = 156,
server_moved = 157,
shared_subscriptions_not_supported = 158,
connection_rate_exceeded = 159,
maximum_connect_time = 160,
subscription_identifiers_not_supported = 161,
wildcard_subscriptions_not_supported
};
}