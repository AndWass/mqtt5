#pragma once

template <class Type>
struct value_receiver
{
    Type *value;
    boost::system::error_code *error;

    value_receiver(Type &value, boost::system::error_code &error): value(std::addressof(value)),
        error(&error) {}
    void set_value(Type v) {
        *value = v;
    }
    void set_error(boost::system::error_code ec) {
        *error = std::move(ec);
    }
};