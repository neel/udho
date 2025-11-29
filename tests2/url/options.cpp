#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/url/options.h>
#include <udho/hazo/map.h>
#include <boost/lexical_cast.hpp>
#include <udho/url/url.h>

namespace opt {
    HAZO_ELEMENT(a, std::string);
    HAZO_ELEMENT(b, std::size_t);
    HAZO_ELEMENT(c, double);
    HAZO_ELEMENT(d, bool);
    HAZO_ELEMENT(e, int);
}

using options_type = udho::url::basic_options<
    opt::a,
    opt::b,
    opt::c
>;

TEST_CASE("url options", "[url][options]") {
    auto options = udho::url::options(opt::a("a"), opt::b(42), opt::c(4.2));

    CHECK(std::is_same_v<options_type, std::decay_t<decltype(options)>>);

    SECTION("options are default constructible, copiable, assignable and movable") {
        options_type def_options;

        CHECK(def_options[opt::a::val].value().empty());
        CHECK(3 == def_options.length);

        def_options = options;

        CHECK(def_options[opt::a::val] == "a");
        CHECK(def_options[opt::b::val] == 42);
        CHECK(def_options[opt::c::val] == 4.2);

        options_type copied_options(options);
        CHECK(copied_options[opt::a::val] == "a");
        CHECK(copied_options[opt::b::val] == 42);
        CHECK(copied_options[opt::c::val] == 4.2);

        options_type moved_options(std::move(copied_options));
        CHECK(moved_options[opt::a::val] == "a");
        CHECK(moved_options[opt::b::val] == 42);
        CHECK(moved_options[opt::c::val] == 4.2);
    }

    SECTION("Superset hazo map") {
        using superset_type = udho::hazo::map_d<
            opt::a,
            opt::b,
            opt::c,
            opt::d,
            opt::e
        >;

        superset_type superset;
        superset[opt::a::val] = "opt::a";
        superset[opt::b::val] = 423;
        superset[opt::c::val] = 42.3;
        superset[opt::d::val] = false;
        superset[opt::e::val] = -23;

        std::size_t count = options.apply(superset);

        CHECK(count == options.length);
        CHECK(superset[opt::a::val] == "a");
        CHECK(superset[opt::b::val] == 42);
        CHECK(superset[opt::c::val] == 4.2);
        CHECK(superset[opt::d::val] == false);
        CHECK(superset[opt::e::val] == -23);

        CHECK(options[opt::a::val] == "a");
        CHECK(options[opt::b::val] == 42);
        CHECK(options[opt::c::val] == 4.2);
    }

    SECTION("empty options are valid") {
        auto empty_options = udho::url::options();

        using superset_type = udho::hazo::map_d<
            opt::a,
            opt::b,
            opt::c,
            opt::d,
            opt::e
        >;

        superset_type superset;
        superset[opt::a::val] = "opt::a";
        superset[opt::b::val] = 423;
        superset[opt::c::val] = 42.3;
        superset[opt::d::val] = false;
        superset[opt::e::val] = -23;

        std::size_t count = empty_options.apply(superset);

        CHECK(count == empty_options.length);
        CHECK(0 == empty_options.length);
        CHECK(superset[opt::a::val] == "opt::a");
        CHECK(superset[opt::b::val] == 423);
        CHECK(superset[opt::c::val] == 42.3);
        CHECK(superset[opt::d::val] == false);
        CHECK(superset[opt::e::val] == -23);
    }

}

TEST_CASE("url match incorporates with options", "[url][pattern][options]") {
    SECTION("home matcher with options"){
        udho::url::pattern::match<udho::url::pattern::formats::home, udho::url::no_options, char> match_without_options = udho::url::home(udho::url::verb::get);
        CHECK(match_without_options.options().length == 0);

        udho::url::pattern::match<udho::url::pattern::formats::home, options_type, char> match_with_options{udho::url::verb::get, udho::url::options(opt::a("a"), opt::b(42), opt::c(4.2))};
        CHECK(match_with_options.options().length == 3);
        CHECK(match_with_options.options()[opt::a::val] == "a");
        CHECK(match_with_options.options()[opt::b::val] == 42);
        CHECK(match_with_options.options()[opt::c::val] == 4.2);

        auto match_added_options = match_without_options.options(opt::a("a"), opt::b(42), opt::c(4.2));
        CHECK(std::is_same_v<std::decay_t<decltype(match_added_options)>, udho::url::pattern::match<udho::url::pattern::formats::home, options_type, char>>);
        CHECK(match_added_options.options().length == 3);
        CHECK(match_added_options.options()[opt::a::val] == "a");
        CHECK(match_added_options.options()[opt::b::val] == 42);
        CHECK(match_added_options.options()[opt::c::val] == 4.2);

        CHECK(match_with_options.method()       == match_without_options.method());
        CHECK(match_with_options.pattern()      == match_without_options.pattern());
        CHECK(match_with_options.replacement()  == match_without_options.replacement());

        CHECK(match_added_options.method()      == match_without_options.method());
        CHECK(match_added_options.pattern()     == match_without_options.pattern());
        CHECK(match_added_options.replacement() == match_without_options.replacement());
    }

    SECTION("fixed matcher with options"){
        udho::url::pattern::match<udho::url::pattern::formats::fixed, udho::url::no_options, char> match_without_options = udho::url::fixed(udho::url::verb::get, "/example/path", "/example/path");
        CHECK(match_without_options.options().length == 0);

        udho::url::pattern::match<udho::url::pattern::formats::fixed, options_type, char> match_with_options{udho::url::verb::get, "/example/path", "/example/path", udho::url::options(opt::a("a"), opt::b(42), opt::c(4.2))};
        CHECK(match_with_options.options().length == 3);
        CHECK(match_with_options.options()[opt::a::val] == "a");
        CHECK(match_with_options.options()[opt::b::val] == 42);
        CHECK(match_with_options.options()[opt::c::val] == 4.2);

        auto match_added_options = match_without_options.options(opt::a("a"), opt::b(42), opt::c(4.2));
        CHECK(std::is_same_v<std::decay_t<decltype(match_added_options)>, udho::url::pattern::match<udho::url::pattern::formats::fixed, options_type, char>>);
        CHECK(match_added_options.options().length == 3);
        CHECK(match_added_options.options()[opt::a::val] == "a");
        CHECK(match_added_options.options()[opt::b::val] == 42);
        CHECK(match_added_options.options()[opt::c::val] == 4.2);

        CHECK(match_with_options.method()       == match_without_options.method());
        CHECK(match_with_options.pattern()      == match_without_options.pattern());
        CHECK(match_with_options.replacement()  == match_without_options.replacement());

        CHECK(match_added_options.method()      == match_without_options.method());
        CHECK(match_added_options.pattern()     == match_without_options.pattern());
        CHECK(match_added_options.replacement() == match_without_options.replacement());
    }

    SECTION("scan matcher with options"){
        udho::url::pattern::match<udho::url::pattern::formats::p1729, udho::url::no_options, char> match_without_options = udho::url::scan(udho::url::verb::get, "/user/{}/{:d}", "/user/{}/{}");
        CHECK(match_without_options.options().length == 0);

        udho::url::pattern::match<udho::url::pattern::formats::p1729, options_type, char> match_with_options{udho::url::verb::get, "/user/{}/{:d}", "/user/{}/{}", udho::url::options(opt::a("a"), opt::b(42), opt::c(4.2))};
        CHECK(match_with_options.options().length == 3);
        CHECK(match_with_options.options()[opt::a::val] == "a");
        CHECK(match_with_options.options()[opt::b::val] == 42);
        CHECK(match_with_options.options()[opt::c::val] == 4.2);

        auto match_added_options = match_without_options.options(opt::a("a"), opt::b(42), opt::c(4.2));
        CHECK(std::is_same_v<std::decay_t<decltype(match_added_options)>, udho::url::pattern::match<udho::url::pattern::formats::p1729, options_type, char>>);
        CHECK(match_added_options.options().length == 3);
        CHECK(match_added_options.options()[opt::a::val] == "a");
        CHECK(match_added_options.options()[opt::b::val] == 42);
        CHECK(match_added_options.options()[opt::c::val] == 4.2);

        CHECK(match_with_options.method()       == match_without_options.method());
        CHECK(match_with_options.pattern()      == match_without_options.pattern());
        CHECK(match_with_options.replacement()  == match_without_options.replacement());

        CHECK(match_added_options.method()      == match_without_options.method());
        CHECK(match_added_options.pattern()     == match_without_options.pattern());
        CHECK(match_added_options.replacement() == match_without_options.replacement());
    }

    SECTION("regex matcher with options"){
        udho::url::pattern::match<udho::url::pattern::formats::regex, udho::url::no_options, char> match_without_options = udho::url::regx(udho::url::verb::get, "/user/(\\w+)/(\\d+)", "/user/{}/{}");
        CHECK(match_without_options.options().length == 0);

        udho::url::pattern::match<udho::url::pattern::formats::regex, options_type, char> match_with_options{udho::url::verb::get, "/user/(\\w+)/(\\d+)", "/user/{}/{}", udho::url::options(opt::a("a"), opt::b(42), opt::c(4.2))};
        CHECK(match_with_options.options().length == 3);
        CHECK(match_with_options.options()[opt::a::val] == "a");
        CHECK(match_with_options.options()[opt::b::val] == 42);
        CHECK(match_with_options.options()[opt::c::val] == 4.2);

        auto match_added_options = match_without_options.options(opt::a("a"), opt::b(42), opt::c(4.2));
        CHECK(std::is_same_v<std::decay_t<decltype(match_added_options)>, udho::url::pattern::match<udho::url::pattern::formats::regex, options_type, char>>);
        CHECK(match_added_options.options().length == 3);
        CHECK(match_added_options.options()[opt::a::val] == "a");
        CHECK(match_added_options.options()[opt::b::val] == 42);
        CHECK(match_added_options.options()[opt::c::val] == 4.2);

        CHECK(match_with_options.method()       == match_without_options.method());
        CHECK(match_with_options.pattern()      == match_without_options.pattern());
        CHECK(match_with_options.replacement()  == match_without_options.replacement());

        CHECK(match_added_options.method()      == match_without_options.method());
        CHECK(match_added_options.pattern()     == match_without_options.pattern());
        CHECK(match_added_options.replacement() == match_without_options.replacement());
    }
}
