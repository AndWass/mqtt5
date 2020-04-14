
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace mqtt5
{
class topic_filter
{
private:
    std::vector<std::string> levels_;

    bool is_valid_level(const std::string &level, std::string_view rest_of_filter) {
        /**
         * Multilevel wildcard must be the last level
         * and must not be part of a filter.
         *
         * MQTT5 4.7.1.2:
         * Non-normative comment
         * “sport/#” also matches the singular “sport”, since # includes the parent level.
         * “#” is valid and will receive every Application Message
         * “sport/tennis/#” is valid
         * “sport/tennis#” is not valid
         * “sport/tennis/#/ranking” is not valid
         */

        if (level == "#") {
            return rest_of_filter.empty();
        }
        if (level.find('#') != std::string::npos) {
            return false;
        }

        /**
         * Single-level wildcard can appear anywhere but
         * must be the sole character of that level.
         */
        if (level.find('+') != std::string::npos && level.size() > 1) {
            return false;
        }

        return true;
    }

    void parse_inplace(std::string_view filter) {
        assert(!filter.empty());

        levels_.clear();
        while (!filter.empty()) {
            auto next_separator = std::find(filter.begin(), filter.end(), '/');
            levels_.emplace_back(filter.begin(), next_separator);
            if (next_separator != filter.end()) {
                // Make sure to remove the separator as well
                filter.remove_prefix(1 + next_separator - filter.begin());
                if(filter.empty()) {
                    levels_.emplace_back();
                }
            }
            else {
                filter = std::string_view{};
            }
            assert(is_valid_level(levels_.back(), filter));
        }
    }

public:
    topic_filter() = default;
    topic_filter(const char* topic) {
        parse_inplace(topic);
    }
    topic_filter(std::string_view topic) {
        parse_inplace(topic);
    }
    std::string to_string() {
        assert(!levels_.empty());

        std::string retval;
        auto iter = levels_.begin();

        for (; iter != levels_.end(); iter++) {
            if(iter != levels_.begin()) {
                retval += "/";
            }
            retval += *iter;
        }
        return retval;
    }
    static topic_filter from_string(std::string_view string) {
        topic_filter retval(string);
        return retval;
    }

    enum class relationship_t { unrelated, equal, left_covers_right, right_covers_left };
    
    /**
     * Checks if this topic filter matches a given topic name
     *
     * The topic name must not contain wildcards
     */
    bool matches(const std::string &topic_name) const {
        assert(std::find_if(topic_name.begin(), topic_name.end(),
                            [](auto ch) { return ch == '+' || ch == '#'; }) == topic_name.end());

        if(topic_name.starts_with("$") && !levels_.front().starts_with("$")) {
            return false;
        }
        auto name_as_filter = from_string(topic_name);

        auto matches_n_levels = [&](const std::size_t n) {
            for (std::size_t i = 0; i < n; i++) {
                if (i == levels_.size() - 1) {
                    if (levels_[i] == "#") {
                        return true;
                    }
                }
                if (levels_[i] != "+") {
                    if (levels_[i] != name_as_filter.levels_[i]) {
                        return false;
                    }
                }
            }
            return true;
        };

        if (name_as_filter.levels_.size() < levels_.size()) {
            // If the name has fewer levels than the filter,
            // the only way to match is if the filter contains a
            // '#' wildcard and is exactly one level greater (wild card level)
            if (name_as_filter.levels_.size() != levels_.size() - 1) {
                return false;
            }

            if (levels_.back() != "#") {
                return false;
            }

            return matches_n_levels(name_as_filter.levels_.size());
        }
        else if (name_as_filter.levels_.size() > levels_.size()) {
            // Only way to match is if levels_.back() == "#" and all preceeding levels matches
            if (levels_.back() == "#") {
                return matches_n_levels(levels_.size() - 1);
            }
            return false;
        }

        // lengths are equal
        return matches_n_levels(levels_.size());
    }
};
} // namespace mqtt5