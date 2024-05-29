#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/sections.h>
#include <udho/view/parser.h>

struct parsed_document{
    using const_iterator = std::vector<udho::view::sections::section>::const_iterator;

    inline void operator()(const udho::view::sections::section& section){
        _sections.emplace_back(section);
    }
    inline std::size_t size() const { return _sections.size(); }
    inline const udho::view::sections::section& operator[](std::size_t index) const {
        if (index >= _sections.size()) {
            throw std::out_of_range("Index out of range");
        }
        return _sections[index];
    }

    // Returns an iterator to the beginning of the sections
    inline const_iterator begin() const { return _sections.begin(); }

    // Returns an iterator to the end of the sections
    inline const_iterator end() const { return _sections.end(); }
    private:
        std::vector<udho::view::sections::section> _sections;
};

TEST_CASE("Parser correctly identifies Meta Blocks", "[template]") {
    std::string input = R"TEMPLATE(<?! register "views.user.badge"; lang "lua" ?>)TEMPLATE";
    udho::view::sections::parser parser;
    parsed_document script;
    parser.parse(input.begin(), input.end(), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::sections::section::meta);
    REQUIRE(script[0].content() == R"(register "views.user.badge"; lang "lua")");
}

TEST_CASE("Parser correctly identifies Echo Blocks", "[template]") {
    const char* input = R"TEMPLATE(Hello <?= d.world ?>)TEMPLATE";
    udho::view::sections::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 2);
    REQUIRE(script[0].type() == udho::view::sections::section::text);
    REQUIRE(script[0].content() == "Hello ");
    REQUIRE(script[1].type() == udho::view::sections::section::echo);
    REQUIRE(script[1].content() == "d.world");
}

TEST_CASE("Parser correctly identifies Eval Blocks", "[template]") {
    const char* input = R"TEMPLATE(<? d = udho.view() ?>)TEMPLATE";
    udho::view::sections::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::sections::section::eval);
    REQUIRE(script[0].content() == "d = udho.view()");
}

TEST_CASE("Parser correctly ignores Comment Blocks", "[template]") {
    const char* input = R"TEMPLATE(<# Some comments that will be ignored #>)TEMPLATE";
    udho::view::sections::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::sections::section::comment);
    REQUIRE(script[0].content() == "Some comments that will be ignored");
}

TEST_CASE("Parser correctly handles Embed Blocks", "[template]") {
    const char* input = R"TEMPLATE(<?:score udho.view() ?>)TEMPLATE";
    udho::view::sections::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::sections::section::embed);
    REQUIRE(script[0].content() == "score udho.view()");
}

TEST_CASE("Parser correctly handles Verbatim Blocks", "[template]") {
    const char* input = R"TEMPLATE(<@ verbatim block @>)TEMPLATE";
    udho::view::sections::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::sections::section::verbatim);
    REQUIRE(script[0].content() == " verbatim block ");
}
