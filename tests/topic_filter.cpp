#include <mqtt5/topic_filter.hpp>
#include <string_view>
#include <doctest/doctest.h>

TEST_CASE("topic_filter: from string back to string") {
    auto check_roundtrip = [](std::string_view sv) {
        auto filter = mqtt5::topic_filter::from_string(sv);
        REQUIRE(filter.to_string() == std::string(sv));
    };
    std::vector<const char *> test_strings{"/",           "//",       "///",    "////",  "abc",
                                           "/abc",        "abc/",     "/abc/",  "//abc", "//abc//",
                                           "//abc//cdef//", "/abc/def", "abc/def", "/abc/def", "/abc/def/"};
    for (const auto *str : test_strings) {
        check_roundtrip(str);
    }
}

TEST_CASE("topic_filter: multilevel wildcard matches") {
    SUBCASE("expects matching all")
    {
        auto filter = mqtt5::topic_filter::from_string("#");
        REQUIRE(filter.matches("/"));
        REQUIRE(filter.matches("/sport"));
        REQUIRE(filter.matches("/sport/player1"));
        REQUIRE(filter.matches("/sport/player1/score"));
        REQUIRE(filter.matches("sport/player1/score"));

        REQUIRE_FALSE(filter.matches("$"));
        REQUIRE_FALSE(filter.matches("$/"));
        REQUIRE_FALSE(filter.matches("$/sport"));
        REQUIRE_FALSE(filter.matches("$/sport/player1"));
        REQUIRE_FALSE(filter.matches("$/sport/player1/score"));
        REQUIRE_FALSE(filter.matches("$sport/player1/score"));
    }
    SUBCASE("expects matching $")
    {
        auto filter = mqtt5::topic_filter::from_string("$SYS/#");
        REQUIRE(filter.matches("$SYS"));
        REQUIRE(filter.matches("$SYS/"));
        REQUIRE(filter.matches("$SYS/sport"));
        REQUIRE(filter.matches("$SYS/sport/player1"));
        REQUIRE(filter.matches("$SYS/sport/player1/score"));
        REQUIRE_FALSE(filter.matches("$sport/player1/score"));
    }
    SUBCASE("matches parts")
    {
        auto filter = mqtt5::topic_filter::from_string("sport/player1/#");
        REQUIRE_FALSE(filter.matches("/"));
        REQUIRE_FALSE(filter.matches("/sport"));
        REQUIRE_FALSE(filter.matches("/sport/player1"));
        REQUIRE_FALSE(filter.matches("/sport/player1/score"));

        REQUIRE(filter.matches("sport/player1"));
        REQUIRE(filter.matches("sport/player1/score"));
        REQUIRE(filter.matches("sport/player1/score/high"));

        REQUIRE_FALSE(filter.matches("sport/player2"));
        REQUIRE_FALSE(filter.matches("sport/player2/score"));
        REQUIRE_FALSE(filter.matches("sport/player2/score/high"));
    }
}

TEST_CASE("topic_filter: single level wildcard")
{
    auto filter = mqtt5::topic_filter::from_string("+");
    REQUIRE(filter.matches("hello"));
    REQUIRE_FALSE(filter.matches("/hello"));
    REQUIRE_FALSE(filter.matches("/"));

    filter = mqtt5::topic_filter::from_string("/+");
    REQUIRE(filter.matches("/hello"));
    REQUIRE(filter.matches("/"));
    REQUIRE_FALSE(filter.matches("hello"));

    filter = mqtt5::topic_filter::from_string("+/+");
    REQUIRE(filter.matches("/hello"));
    REQUIRE(filter.matches("/"));
    REQUIRE_FALSE(filter.matches("hello"));
}