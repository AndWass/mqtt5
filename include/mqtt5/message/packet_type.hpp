#pragma once

namespace mqtt5
{
enum class packet_type {
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
}