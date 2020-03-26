
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mqtt5_v2/protocol/binary.hpp>
#include <mqtt5_v2/protocol/fixed_int.hpp>
#include <mqtt5_v2/protocol/string.hpp>
#include <mqtt5_v2/protocol/varlen_int.hpp>

#include <p0443_v2/start.hpp>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/bind.hpp>

#include <tuple>
#include <variant>
#include <vector>

namespace mqtt5_v2::protocol
{
struct property
{
    struct varlen_value
    {
        std::uint32_t value;

        varlen_value& operator=(std::uint32_t v) {
            value = v;
            return *this;
        }
    };

    template<class Stream>
    struct deserializer
    {
        transport::data_fetcher<Stream> data;
        void operator()(std::uint8_t& v) {
            v = fixed_int<std::uint8_t>::deserialize(data);
        }
        void operator()(std::uint16_t& v) {
            v = fixed_int<std::uint16_t>::deserialize(data);
        }
        void operator()(std::uint32_t& v) {
            v = fixed_int<std::uint32_t>::deserialize(data);
        }
        void operator()(varlen_value& v) {
            v.value = varlen_int::deserialize(data);
        }

        void operator()(std::string& v) {
            v = string::deserialize(data);
        }

        void operator()(std::vector<std::uint8_t>& v) {
            v = binary::deserialize(data);
        }

        void operator()(key_value_pair& v) {
            v = key_value_pair::deserialize(data);
        }
    };

    template<class Writer>
    struct serializer
    {
        Writer* writer;
        void operator()(std::uint8_t v) {
            fixed_int<std::uint8_t>::serialize(v, *writer);
        }
        void operator()(std::uint16_t v) {
            fixed_int<std::uint16_t>::serialize(v, *writer);
        }
        void operator()(std::uint32_t v) {
            fixed_int<std::uint32_t>::serialize(v, *writer);
        }
        void operator()(varlen_value v) {
            varlen_int::serialize(v.value, *writer);
        }

        void operator()(const std::string& v) {
            string::serialize(v, *writer);
        }

        void operator()(const std::vector<std::uint8_t>& v) {
            binary::serialize(v, *writer);
        }

        void operator()(const key_value_pair& v) {
            key_value_pair::serialize(v, *writer);
        }
    };
    using value_storage =
        std::variant<std::uint8_t, std::uint16_t, std::uint32_t,
                    varlen_value,
                     std::string, std::vector<std::uint8_t>, key_value_pair>;

    template <class Stream>
    static property deserialize(transport::data_fetcher<Stream> data)
    {
        property retval;
        retval.identifier = varlen_int::deserialize(data);
        retval.activate_id(retval.identifier);
        std::visit(deserializer<Stream>{data}, retval.value);
        return retval;
    }

    template<class Writer>
    static void serialize(const property& prop, Writer& writer) {
        varlen_int::serialize(prop.identifier, writer);
        std::visit(serializer<Writer>{std::addressof(writer)}, prop.value);
    }

    void activate_id(std::uint8_t id) {
#define ACTIVATE(X)                                                                                \
    value.emplace<X>();                                                                            \
    break
        switch (id) {
        case 1:
            ACTIVATE(0);
        case 2:
            ACTIVATE(2);
        case 3:
            ACTIVATE(4);
        case 8:
            ACTIVATE(4);
        case 9:
            ACTIVATE(5);
        case 11:
            ACTIVATE(5);
        case 17:
            ACTIVATE(2);
        case 18:
            ACTIVATE(4);
        case 19:
            ACTIVATE(1);
        case 21:
            ACTIVATE(4);
        case 22:
            ACTIVATE(5);
        case 23:
            ACTIVATE(0);
        case 24:
            ACTIVATE(2);
        case 25:
            ACTIVATE(0);
        case 26:
            ACTIVATE(4);
        case 28:
            ACTIVATE(4);
        case 31:
            ACTIVATE(4);
        case 33:
            ACTIVATE(1);
        case 34:
            ACTIVATE(1);
        case 35:
            ACTIVATE(1);
        case 36:
            ACTIVATE(0);
        case 37:
            ACTIVATE(0);
        case 38:
            ACTIVATE(6);
        case 39:
            ACTIVATE(2);
        case 40:
            ACTIVATE(0);
        case 41:
            ACTIVATE(0);
        case 42:
            ACTIVATE(0);
        }
#undef ACTIVATE
    }

    template<class T>
    void set_id_value(std::uint8_t id, const T& val) {
        activate_id(id);
        identifier = id;
        set_value(val);
    }

    template<class T>
    void set_value(const T& val) {
        static_assert(
            boost::mp11::mp_any_of<
                value_storage,
                boost::mp11::mp_bind_back<std::is_assignable, T>::template fn
            >::value,
            "T cannot be used to assign to any potential property value"
        );
        std::visit([&](auto& elem) {
            if constexpr(std::is_assignable_v<decltype(elem), T>) {
                elem = val;
            }
            else {
                throw std::runtime_error("Mismatched attempt to set property types");
            }
        }, value);
    }

    property() = default;

    template<class T>
    property(std::uint8_t id, const T& val) {
        set_id_value(id, val);
    }

    varlen_int::type identifier;
    value_storage value;
};
struct properties
{
    /*template <class Stream>
    auto inplace_deserializer(transport::data_fetcher<Stream> data) {
        auto length_deserializer = properties_length_.inplace_deserializer(data);
        auto storage_resizer_ = p0443_v2::then(std::move(length_deserializer),
                                               [this](transport::data_fetcher<Stream> data) {
                                                   return data.get_data(properties_length_.value);
                                               });
        return p0443_v2::transform(std::move(storage_resizer_), [this](auto data) {
            this->from_bytes(data.cspan().subspan(0, properties_length_.value));
            data.consume(properties_length_.value);
            return data;
        });
    }

    nonstd::span<const std::uint8_t> from_bytes(nonstd::span<const std::uint8_t> data) {
        std::vector<property> new_properties;
        while(!data.empty()) {
            new_properties.emplace_back();
            data = new_properties.back().set_from_bytes(data);
        }
        storage_ = std::move(new_properties);
        return data;
    }*/

    template <class Stream>
    void deserialize(transport::data_fetcher<Stream> data)
    {
        varlen_int::type property_data_length = varlen_int::deserialize(data);
        auto data_span = data.cspan(property_data_length);
        while(!data_span.empty()) {
            properties_.emplace_back(property::deserialize(transport::buffer_data_fetcher(data_span)));
        }
        data.consume(property_data_length);
    }

    template<class Writer>
    void serialize(Writer& writer) const {
        std::uint32_t prop_len = 0;
        auto len_finder = [&](std::uint8_t) {
            prop_len++;
        };
        // Find the length of the contained properties
        for(const auto& p: properties_) {
            property::serialize(p, len_finder);
        }

        varlen_int::serialize(prop_len, writer);
        for(const auto& p: properties_) {
            property::serialize(p, writer);
        }
    }

    const std::vector<property>& properties_ref() const {
        return properties_;
    }

    void set_properties(std::vector<property> props) {
        properties_ = std::move(props);
    }

    void add_property(property prop) {
        if(prop.identifier == 38)
        {
            properties_.emplace_back(std::move(prop));
        }
        else
        {
            auto iter = std::find_if(properties_.begin(), properties_.end(), [&](auto &p) {
                return p.identifier == prop.identifier;
            });
            if(iter != properties_.end()) {
                iter->value = std::move(prop.value);
            }
            else {
                properties_.emplace_back(std::move(prop));
            }
        }
    }

    template<class T>
    void add_property(std::uint8_t id, const T& value) {
        add_property(property(id, value));
    }

private:
    std::vector<property> properties_;
};
} // namespace mqtt5_v2::protocol
