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
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/www/components/handler.h>
#include <udho/www/components/protocol.h>
#include <udho/www/components/routing.h>
#include <udho/www/components/cookies.h>
#include <udho/www/components/session.h>
#include <udho/www/components/pg.h>
#include <udho/www/components/resources.h>
#include <udho/www/components/navigator.h>
#include <udho/manifold/composition_view.h>
#include <udho/manifold/journal_view.h>
#include <udho/manifold/configs_view.h>

namespace opt {
    HAZO_ELEMENT(a, std::string);
    HAZO_ELEMENT(b, std::size_t);
    HAZO_ELEMENT(c, double);
    HAZO_ELEMENT(d, bool);
    HAZO_ELEMENT(e, int);
    HAZO_ELEMENT(f, std::string);
    HAZO_ELEMENT(g, std::string);
    HAZO_ELEMENT(h, std::string);
}

using all_options_type = udho::url::basic_options<
    opt::a,
    opt::b,
    opt::c,
    opt::d,
    opt::e,
    opt::f,
    opt::g,
    opt::h
>;

using options_type = udho::url::basic_options<
    opt::a,
    opt::b,
    opt::c
>;

using stream_type      = boost::beast::test::stream; // udho::net::types::socket;

namespace callbacks{
using namespace udho::www::components;
using namespace udho::manifold;

using handler = basic_handler<stream_type>;

struct nodef{
    nodef() = delete;
    nodef(int) {}
};

BOOST_SYMBOL_EXPORT void f0(basic_context<stream_type, handler, cookies> context){
    context << "f0";
    context.finish();
    return;
}

BOOST_SYMBOL_EXPORT int f1(basic_context<stream_type, handler, navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
    context << std::to_string(a.size()+b.size()+c+d);
    std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
    context.finish();
    return 42;
}

BOOST_SYMBOL_EXPORT std::string f2(basic_context<stream_type, handler, cookies> context, int a, const std::string& b){
    context << std::to_string(a+b.size());
    context.finish();
    return "hello";
}

BOOST_SYMBOL_EXPORT std::string f_nodef(basic_context<stream_type, handler> context, nodef, int a){
    context << std::to_string(a);
    context.finish();
    return "hello";
}

}

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



TEST_CASE("url router can apply configurations", "[url][router][options]") {
    using namespace udho::hazo::string::literals;

    auto routes1 =
        udho::url::slot("f0"_h,  &callbacks::f0)  << udho::url::home(udho::url::verb::get)
    |   udho::url::slot("f1"_h,  &callbacks::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}").options(opt::a("a1"), opt::b(42), opt::c(4.2))
    |   udho::url::slot("f2"_h,  &callbacks::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}").options(opt::a("a2"))
    ;

    auto routes2 =
        udho::url::slot("f0"_h,  &callbacks::f0)  << udho::url::home(udho::url::verb::get).options(opt::a("x0"), opt::b(84), opt::c(8.4))
    |   udho::url::slot("f1"_h,  &callbacks::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")
    |   udho::url::slot("f2"_h,  &callbacks::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}").options(opt::a("x2"))
    ;

    auto table  = udho::url::mount("root"_h, "/", std::move(routes1)) | udho::url::mount("m2"_h, "/m2", std::move(routes2));
    auto router = udho::url::router(std::move(table));

    all_options_type all_options;

    {
        router.reconfigure_for(udho::url::detail::route_index{"f1", 0, 0}, all_options);
        CHECK(all_options[opt::a::val] == "a2");
    }{
        router.reconfigure_for(udho::url::detail::route_index{"f1", 0, 1}, all_options);
        CHECK(all_options[opt::a::val] == "a1");
    }
}
