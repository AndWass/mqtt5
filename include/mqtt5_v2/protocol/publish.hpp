
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5_v2/transport/data_fetcher.hpp>
#include <mqtt5_v2/protocol/header.hpp>
#include <mqtt5_v2/protocol/string.hpp>
#include <mqtt5_v2/protocol/properties.hpp>
#include <mqtt5_v2/protocol/inplace_deserializer.hpp>

#include <p0443_v2/then.hpp>
#include <p0443_v2/transform.hpp>
#include <p0443_v2/sequence.hpp>

#include <optional>
#include <variant>

namespace mqtt5_v2::protocol
{
using std::begin;
using std::end;

class publish
{
private:
    string topic_;
    std::optional<string> packet_identifier_;
    properties properties_;

    /**
     * if the vector is active then the publish packet is a "normal" publish.
     * If the uint32_t is active data_ will contain the remaining length
     * of data. This is used when deserializing a publish packet
     * by first reading remaining length bytes, store this in
     * the vector and then parse the packet from this data
     */
    std::variant<std::vector<std::uint8_t>, std::uint32_t> data_;

    std::uint8_t header_flags_;

    std::vector<std::uint8_t>& data_ref() {
        return std::get<0>(data_);
    }
    
    const std::vector<std::uint8_t>& data_ref() const {
        return std::get<0>(data_);
    }

    
public:

    static constexpr std::uint8_t type_value = 3;

    publish() = default;
    publish(std::in_place_t, std::uint8_t flags, std::uint32_t payload_length): data_(std::in_place_index<1>, payload_length), header_flags_(flags) {}

    void set_duplicate(bool dup) {
        header_flags_ = (header_flags_ & 0x07) + ((dup ? 1:0) << 3);
    }

    bool duplicate_flag() const {
        return header_flags_ & 0x08;
    }

    void set_quality_of_service(std::uint8_t qos) {
        qos &= 0x03;
        if(qos == 0) {
            set_duplicate(false);
        }
        header_flags_ = (header_flags_ & 0x09) + (qos << 1);
    }

    std::uint8_t quality_of_service() const {
        return (header_flags_ & 0x09) >> 1;
    }

    void set_retain(bool retain) {
        header_flags_ = (header_flags_ & 12) + (retain ? 1:0);
    }

    bool retain_flag() const {
        return header_flags_ & 0x01;
    }

    void set_topic(std::string topic) {
        topic_ = std::move(topic);
    }

    std::vector<std::uint8_t>& get_payload_ref() {
        if(data_.index() != 0) {
            data_ = std::vector<std::uint8_t>{};
        }
        return std::get<0>(data_);
    }

    template<class T, class = decltype(begin(std::declval<T>())), class = decltype(end(std::declval<T>()))>
    void set_payload(T&& t) {
        auto &ref = get_payload_ref();
        ref.clear();
        std::copy(begin(t), end(t), std::back_inserter(ref));
    }

    /**
     * Publish packets must pretty much be read in its entirety into a buffer first,
     * and then parsed into its respective fields.
     *
     * A buffer_data_fetcher can be used to handle this.
     *
     * @return An inplace deserializer sender
     */
    template<class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> fetcher) {
        if(data_.index() != 1) {
            throw std::runtime_error("publish message not initialized for deserialization");
        }
        auto payload_length = std::get<1>(data_);

        // Initialize the vector with enough data to hold the entire payload
        // data_ will then be used with a buffer data fetcher to fill the various
        // fields. When all fields have been set it will contain the payload itself
        data_.template emplace<0>(payload_length);

        // Read the entire packet into data_ vector,
        // then use a buffer_data_fetcher with this buffer to inplace deserialize all
        // fields of the publish packet.
        // The data left in data_ will be the actual payload.
        return p0443_v2::then(protocol::inplace_deserializer(std::get<0>(data_), fetcher), [this](auto fetcher) {
            auto buffer_fetcher = transport::buffer_data_fetcher<std::vector<std::uint8_t>>(std::get<0>(data_));
            if(quality_of_service() > 0) {
                packet_identifier_.emplace();
            }
            else {
                packet_identifier_.reset();
            }

            // We want to "forward" the original fetcher to the receiver,
            // not the middle-man buffer_data_fetcher used in the sequence.
            return p0443_v2::transform(
                p0443_v2::sequence(
                    protocol::inplace_deserializer(topic_, buffer_fetcher),
                    protocol::inplace_deserializer(packet_identifier_, buffer_fetcher),
                    protocol::inplace_deserializer(properties_, buffer_fetcher)
                )
                ,
                [fetcher](auto) {
                    return fetcher;
                });
        });
    }

    template <class Writer>
    void serialize_body(Writer &&writer) const {
        topic_.serialize(writer);
        if(packet_identifier_) {
            packet_identifier_->serialize(writer);
        }
        properties_.serialize(writer);

        for(auto b: data_ref()) {
            writer(b);
        }
    }

    template <class Writer>
    void serialize(Writer &&writer) const {
        header hdr(type_value, header_flags_, *this);
        hdr.serialize(writer);
        serialize_body(writer);
    }
};
}
