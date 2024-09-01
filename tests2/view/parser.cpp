#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/view/tmpl/sections.h>
#include <udho/view/tmpl/parser.h>
#include <udho/view/tmpl/detail/trie.h>

struct parsed_document{
    using const_iterator = std::vector<udho::view::tmpl::section>::const_iterator;

    inline void operator()(const udho::view::tmpl::section& section){
        _sections.emplace_back(section);
    }
    inline std::size_t size() const { return _sections.size(); }
    inline const udho::view::tmpl::section& operator[](std::size_t index) const {
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
        std::vector<udho::view::tmpl::section> _sections;
};

TEST_CASE("Tokenizer correctly builds the trie", "[trie]") {
    udho::view::detail::trie trie;

    std::string ab = "ab",
                abc = "abc",
                abcdef = "abcdef",
                xycdef = "xycdef";

    trie.add(ab, 101);
    trie.add(abc, 102);
    trie.add(abcdef, 103);
    trie.add(xycdef, 104);

    std::string subject = "hello ab I am a string abcd abcdef and then abcxycdef";
    auto begin = subject.begin();
    auto end = subject.end();

    std::vector<std::uint32_t> tokens;

    auto pos = begin;
    while(pos != end){
        auto tpos = trie.next(pos, end);
        pos = tpos.token_end;
        tokens.push_back(tpos.token_id);
        std::string buff;
        std::copy(tpos.token_begin, tpos.token_end, std::back_inserter(buff));
        REQUIRE(trie[tpos.token_id] == buff);
        REQUIRE(std::distance(tpos.token_begin, tpos.token_end) == buff.size());
    }
    REQUIRE(tokens[0] == 101);
    REQUIRE(tokens[1] == 102);
    REQUIRE(tokens[2] == 103);
    REQUIRE(tokens[3] == 102);
    REQUIRE(tokens[4] == 104);
}

TEST_CASE("Parser correctly identifies Meta Blocks", "[template]") {
    std::string input = R"TEMPLATE(<?! register "views.user.badge"; lang "lua" ?>)TEMPLATE";
    udho::view::tmpl::parser parser;
    parsed_document script;
    parser.parse(input.begin(), input.end(), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::tmpl::section::meta);
    REQUIRE(script[0].content() == R"(register "views.user.badge"; lang "lua")");
}

TEST_CASE("Parser correctly identifies Echo Blocks", "[template]") {
    const char* input = R"TEMPLATE(Hello <?= d.world ?>)TEMPLATE";
    udho::view::tmpl::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 2);
    REQUIRE(script[0].type() == udho::view::tmpl::section::text);
    REQUIRE(script[0].content() == "Hello ");
    REQUIRE(script[1].type() == udho::view::tmpl::section::echo);
    REQUIRE(script[1].content() == "d.world");
}

TEST_CASE("Parser correctly identifies Eval Blocks", "[template]") {
    const char* input = R"TEMPLATE(<? d = udho.view() ?>)TEMPLATE";
    udho::view::tmpl::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::tmpl::section::eval);
    REQUIRE(script[0].content() == "d = udho.view()");
}

TEST_CASE("Parser correctly ignores Comment Blocks", "[template]") {
    const char* input = R"TEMPLATE(<# Some comments that will be ignored #>)TEMPLATE";
    udho::view::tmpl::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::tmpl::section::comment);
    REQUIRE(script[0].content() == "Some comments that will be ignored");
}

TEST_CASE("Parser correctly handles Embed Blocks", "[template]") {
    const char* input = R"TEMPLATE(<?:score udho.view() ?>)TEMPLATE";
    udho::view::tmpl::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::tmpl::section::embed);
    REQUIRE(script[0].content() == "score udho.view()");
}

TEST_CASE("Parser correctly handles Verbatim Blocks", "[template]") {
    const char* input = R"TEMPLATE(<@ verbatim block @>)TEMPLATE";
    udho::view::tmpl::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 1);
    REQUIRE(script[0].type() == udho::view::tmpl::section::verbatim);
    REQUIRE(script[0].content() == " verbatim block ");
}

TEST_CASE("Parser correctly handles templates with multiple sections", "[template]") {
    static char input[] = R"TEMPLATE(
        <?! bridge lua; lang lua ?>

        <? if jit then ?>
        LuaJIT is being used
        LuaJIT version: <?= jit.version ?>
        <? else ?>
        LuaJIT is not being used
        <? end ?>

        Hello <?= d.name ?>

        <?:score udho.view() ?>

        <# Some comments that will be ignored #>

        <@ verbatim block @>

    )TEMPLATE";

    udho::view::tmpl::parser parser;
    parsed_document script;
    parser.parse(input, input + strlen(input), script);

    REQUIRE(script.size() == 19);
    REQUIRE(script[0].type() == udho::view::tmpl::section::text);
    REQUIRE(script[0].content() == "\n        ");
    REQUIRE(script[1].type() == udho::view::tmpl::section::meta);
    REQUIRE(script[1].content() == R"(bridge lua; lang lua)");
    REQUIRE(script[2].type() == udho::view::tmpl::section::text);
    REQUIRE(script[2].content() == "\n\n        ");
    REQUIRE(script[3].type() == udho::view::tmpl::section::eval);
    REQUIRE(script[3].content() == "if jit then");
    REQUIRE(script[4].type() == udho::view::tmpl::section::text);
    REQUIRE(script[4].content() == "\n        LuaJIT is being used\n        LuaJIT version: ");
    REQUIRE(script[5].type() == udho::view::tmpl::section::echo);
    REQUIRE(script[5].content() == "jit.version");
    REQUIRE(script[6].type() == udho::view::tmpl::section::text);
    REQUIRE(script[6].content() == "\n        ");
    REQUIRE(script[7].type() == udho::view::tmpl::section::eval);
    REQUIRE(script[7].content() == "else");
    REQUIRE(script[8].type() == udho::view::tmpl::section::text);
    REQUIRE(script[8].content() == "\n        LuaJIT is not being used\n        ");
    REQUIRE(script[9].type() == udho::view::tmpl::section::eval);
    REQUIRE(script[9].content() == "end");
    REQUIRE(script[10].type() == udho::view::tmpl::section::text);
    REQUIRE(script[10].content() == "\n\n        Hello ");
    REQUIRE(script[11].type() == udho::view::tmpl::section::echo);
    REQUIRE(script[11].content() == "d.name");
    REQUIRE(script[12].type() == udho::view::tmpl::section::text);
    REQUIRE(script[12].content() == "\n\n        ");
    REQUIRE(script[13].type() == udho::view::tmpl::section::embed);
    REQUIRE(script[13].content() == "score udho.view()");
    REQUIRE(script[14].type() == udho::view::tmpl::section::text);
    REQUIRE(script[14].content() == "\n\n        ");
    REQUIRE(script[15].type() == udho::view::tmpl::section::comment);
    REQUIRE(script[15].content() == "Some comments that will be ignored");
    REQUIRE(script[16].type() == udho::view::tmpl::section::text);
    REQUIRE(script[16].content() == "\n\n        ");
    REQUIRE(script[17].type() == udho::view::tmpl::section::verbatim);
    REQUIRE(script[17].content() == " verbatim block ");
    REQUIRE(script[18].type() == udho::view::tmpl::section::text);
    REQUIRE(script[18].content() == "\n\n    ");
}
