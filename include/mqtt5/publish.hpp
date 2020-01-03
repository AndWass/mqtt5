#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <vector>
#include <tuple>
#include <string_view>

namespace mqtt5
{
class publish
{
public:
    enum class quality_of_service : std::uint8_t { qos0 = 0, qos1, qos2, reserved };

    std::vector<std::pair<std::string, std::string>> user_properties;
    std::string topic_name;
    std::string response_topic;
    std::string content_type;
    std::vector<std::uint8_t> correlation_data;
    std::chrono::seconds message_expiry_interval{0};
    std::uint32_t subsription_identifier{0};
    std::uint16_t topic_alias = 0;
    std::uint16_t packet_identifier = 0;
    std::uint8_t payload_format_indicator = 0;
    quality_of_service qos{quality_of_service::qos0};
    bool duplicate = false;
    bool retain = false;

    std::vector<std::uint8_t> payload;

    void set_payload(std::string_view data)
    {
        payload.clear();
        std::copy(data.begin(), data.end(), std::back_inserter(payload));
    }
private:
};
}