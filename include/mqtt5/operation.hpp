#pragma once

#include <type_traits>

namespace mqtt5
{
auto on_value_or_error = [](auto on_value, auto on_error) {
    using value_type = decltype(on_value);
    using error_type = decltype(on_error);
    struct rx
    {
        value_type set_value;
        error_type set_error;
    };
    return rx{on_value, on_error};
};

auto on_done_or_error = [](auto on_done, auto on_error) {
    using done_type = decltype(on_done);
    using error_type = decltype(on_error);
    struct rx
    {
        done_type set_done;
        error_type set_error;
    };
    return rx{on_done, on_error};
};
} // namespace mqtt5