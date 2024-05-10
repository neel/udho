#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/sections.h>

TEST_CASE("Parser correctly identifies Meta Blocks", "[template]") {
    std::string input = R"TEMPLATE(<?! register "views.user.badge"; lang "lua" ?>)TEMPLATE";
    std::vector<udho::view::sections::section> sections;
    udho::view::sections::parser parser;
    parser.parse(input.begin(), input.end(), std::back_inserter(sections));

    REQUIRE(sections.size() == 1);
    REQUIRE(sections[0].type() == udho::view::sections::section::meta);
    REQUIRE(sections[0].content() == R"(register "views.user.badge"; lang "lua")");
}

TEST_CASE("Parser correctly identifies Echo Blocks", "[template]") {
    const char* input = R"TEMPLATE(Hello <?= d.world ?>)TEMPLATE";
    std::vector<udho::view::sections::section> sections;
    udho::view::sections::parser parser;
    parser.parse(input, input + strlen(input), std::back_inserter(sections));

    REQUIRE(sections.size() == 2);
    REQUIRE(sections[0].type() == udho::view::sections::section::text);
    REQUIRE(sections[0].content() == "Hello ");
    REQUIRE(sections[1].type() == udho::view::sections::section::echo);
    REQUIRE(sections[1].content() == "d.world");
}

TEST_CASE("Parser correctly identifies Eval Blocks", "[template]") {
    const char* input = R"TEMPLATE(<? d = udho.view() ?>)TEMPLATE";
    std::vector<udho::view::sections::section> sections;
    udho::view::sections::parser parser;
    parser.parse(input, input + strlen(input), std::back_inserter(sections));

    REQUIRE(sections.size() == 1);
    REQUIRE(sections[0].type() == udho::view::sections::section::eval);
    REQUIRE(sections[0].content() == "d = udho.view()");
}

TEST_CASE("Parser correctly ignores Comment Blocks", "[template]") {
    const char* input = R"TEMPLATE(<# Some comments that will be ignored #>)TEMPLATE";
    std::vector<udho::view::sections::section> sections;
    udho::view::sections::parser parser;
    parser.parse(input, input + strlen(input), std::back_inserter(sections));

    REQUIRE(sections.size() == 1);
    REQUIRE(sections[0].type() == udho::view::sections::section::comment);
    REQUIRE(sections[0].content() == "Some comments that will be ignored");
}

TEST_CASE("Parser correctly handles Embed Blocks", "[template]") {
    const char* input = R"TEMPLATE(<?:score udho.view() ?>)TEMPLATE";
    std::vector<udho::view::sections::section> sections;
    udho::view::sections::parser parser;
    parser.parse(input, input + strlen(input), std::back_inserter(sections));

    REQUIRE(sections.size() == 1);
    REQUIRE(sections[0].type() == udho::view::sections::section::embed);
    REQUIRE(sections[0].content() == "score udho.view()");
}

TEST_CASE("Parser correctly handles Verbatim Blocks", "[template]") {
    const char* input = R"TEMPLATE(<@ verbatim block @>)TEMPLATE";
    std::vector<udho::view::sections::section> sections;
    udho::view::sections::parser parser;
    parser.parse(input, input + strlen(input), std::back_inserter(sections));

    REQUIRE(sections.size() == 1);
    REQUIRE(sections[0].type() == udho::view::sections::section::verbatim);
    REQUIRE(sections[0].content() == " verbatim block ");
}
