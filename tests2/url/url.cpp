#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <string>
#include <udho/url/url.h>
#include <type_traits>
#include <dlfcn.h>
#include <cxxabi.h>

struct nodef{
    nodef() = delete;
    nodef(int) {}
};

BOOST_SYMBOL_EXPORT void f0(){
    std::cout << "f0" << std::endl;
    return;
}

BOOST_SYMBOL_EXPORT int f1(int a, const std::string& b, const double& c, bool d){
    return a+b.size()+c+d;
}

BOOST_SYMBOL_EXPORT std::string f2(int a, const std::string& b){
    return std::to_string(a+b.size());
}

BOOST_SYMBOL_EXPORT std::string f_nodef(nodef, int a){
    return "hello";
}

struct X{
    mutable std::string _msg;
    mutable int         _a{};
    mutable std::string _b;
    mutable double      _c{};
    mutable bool        _d{};

    BOOST_SYMBOL_EXPORT void f0(){
        _msg ="f0";
        return;
    }

    BOOST_SYMBOL_EXPORT int f1(int a, const std::string& b, const double& c, bool d){
        _msg ="f1";
        _a = a;
        _b = b;
        _c = c;
        _d = d;
        return a+b.size()+c+d;
    }

    BOOST_SYMBOL_EXPORT std::string f2(int a, const std::string& b){
        _msg ="f2";
        _a = a;
        _b = b;
        return std::to_string(a+b.size());
    }

    BOOST_SYMBOL_EXPORT int f3(int a, const std::string& b, const double& c, bool d) const{
        _msg ="f3";
        _a = a;
        _b = b;
        _c = c;
        _d = d;
        return 84;
    }
};

TEST_CASE("DL_info", "[url][dlinfo]"){
    void (X::* pFunc)() = &X::f0;
    void* ptr = (void*&)pFunc;

    Dl_info f0_info, f1_info, xf0_info;
    dladdr(reinterpret_cast<void *>(&f0), &f0_info);
    dladdr(reinterpret_cast<void *>(&f1), &f1_info);
    dladdr(reinterpret_cast<void *>(ptr), &xf0_info);

    std::string f0_name, f1_name, xf0_name;

    if(f0_info.dli_sname) {
        char* buffer = 0x0;
        int status   = -4;
        buffer = abi::__cxa_demangle(f0_info.dli_sname, NULL, NULL, &status);
        if(status == 0 && buffer != 0x0) {
            f0_name = buffer;
            free(buffer);
        }
    }

    if(f1_info.dli_sname) {
        char* buffer = 0x0;
        int status   = -4;
        buffer = abi::__cxa_demangle(f1_info.dli_sname, NULL, NULL, &status);
        if(status == 0 && buffer != 0x0) {
            f1_name = buffer;
            free(buffer);
        }
    }

    if(xf0_info.dli_sname) {
        char* buffer = 0x0;
        int status   = -4;
        buffer = abi::__cxa_demangle(xf0_info.dli_sname, NULL, NULL, &status);
        if(status == 0 && buffer != 0x0) {
            xf0_name = buffer;
            free(buffer);
        }
    }

    CHECK(f0_name == "f0()");
    CHECK(f1_name == "f1(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, double const&, bool)");
    CHECK(xf0_name == "X::f0()");
}

TEST_CASE("Regex matching operations", "[url][pattern][regex]") {
    udho::url::pattern::match<udho::url::pattern::formats::regex, udho::url::no_options, char> match(udho::url::verb::get, "/user/(\\w+)/(\\d+)", "/user/{}/{}");

    SECTION("Successful match") {
        std::string subject = "/user/alexa/12345";
        REQUIRE(match.find(subject) == true);
    }

    SECTION("Unsuccessful match") {
        std::string subject = "/user/abc/def";
        REQUIRE(match.find(subject) == false);
    }

    SECTION("Successful capture") {
        std::string subject = "/user/alexa/12345";
        std::tuple<std::string, std::uint32_t> args;
        REQUIRE(match.find(subject, args) == true);
        REQUIRE(std::get<0>(args) == "alexa");
        REQUIRE(std::get<1>(args) == 12345);
    }

    SECTION("Unsuccessful capture") {
        std::string subject = "/user/alexa/robot";
        std::tuple<std::string, std::uint32_t> args;
        REQUIRE(match.find(subject, args) == false);
        REQUIRE(std::get<0>(args) != "alexa");
        REQUIRE(std::get<1>(args) != 12345);
    }

    SECTION("Pattern replacement") {
        REQUIRE(match.replace(std::make_tuple("alexa", 12345)) == "/user/alexa/12345");
    }
}

TEST_CASE("String matching operations using p1729 format", "[url][pattern][p1729]") {
    udho::url::pattern::match<udho::url::pattern::formats::p1729, udho::url::no_options, char> matcher(udho::url::verb::get, "/user/{}/{:d}", "/user/{}/{}");

    SECTION("Successful string match and extraction") {
        std::string subject = "/user/john/12345";
        std::tuple<std::string, int> args;
        REQUIRE(matcher.find(subject, args) == true);
        REQUIRE(std::get<0>(args) == "john");
        REQUIRE(std::get<1>(args) == 12345);
    }

    SECTION("Unsuccessful string match due to format mismatch") {
        std::string subject = "/user/john/abc";
        std::tuple<std::string, int> args;
        REQUIRE(matcher.find(subject, args) == false);
    }

    SECTION("Pattern and replacement equality when replacement is omitted") {
        udho::url::pattern::match<udho::url::pattern::formats::p1729, udho::url::no_options, char> simple_matcher(udho::url::verb::get, "/user/{}/{:d}");
        REQUIRE(simple_matcher.pattern() == "/user/{}/{:d}");
        REQUIRE(simple_matcher.replacement() == "/user/{}/{:d}");
        REQUIRE(simple_matcher.replace(std::make_tuple("john", 12345)) == "/user/john/12345");
    }

    SECTION("Pattern replacement") {
        std::tuple<std::string, int> args = std::make_tuple("john", 12345);
        REQUIRE(matcher.replace(args) == "/user/john/12345");
    }
}

TEST_CASE("Fixed string matching operations", "[url][pattern][fixed]") {
    udho::url::pattern::match<udho::url::pattern::formats::fixed, udho::url::no_options, char> matcher(udho::url::verb::get, "/example/path", "/example/path");

    SECTION("Successful string match") {
        std::string subject = "/example/path";
        REQUIRE(matcher.find(subject) == true);
    }

    SECTION("Unsuccessful string match due to different string") {
        std::string subject = "/another/path";
        REQUIRE(matcher.find(subject) == false);
    }

    SECTION("Pattern and replacement distinction") {
        REQUIRE(matcher.pattern() == "/example/path");
        REQUIRE(matcher.replacement() == "/example/path");
    }

    SECTION("Pattern replacement with arguments") {
        std::tuple<std::string, int> args = std::make_tuple("ignored", 123);
        REQUIRE(matcher.replace(args) == "/example/path");
    }
}

TEST_CASE("Home pattern matching operations", "[url][pattern][home]") {
    udho::url::pattern::match<udho::url::pattern::formats::home, udho::url::no_options, char> matcher(udho::url::verb::get);

    SECTION("Match explicit home pattern") {
        std::string subject = "/";
        REQUIRE(matcher.find(subject) == true);
    }

    SECTION("Match empty string as home") {
        std::string subject = "";
        REQUIRE(matcher.find(subject) == true);
    }

    SECTION("Unsuccessful match with non-home URL") {
        std::string subject = "/about";
        REQUIRE(matcher.find(subject) == false);
    }

    SECTION("Pattern and replacement output") {
        REQUIRE(matcher.pattern() == "/");
        REQUIRE(matcher.replacement() == "/");
    }

    SECTION("URL generation should always return home") {
        std::tuple<std::string, int> args = std::make_tuple("any", 123);  // Arguments should be ignored
        REQUIRE(matcher.replace(args) == "/");
    }
}


TEST_CASE("url common functionalities", "[url][pattern][router]") {
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f0))::return_type, void>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f0))::return_type, void>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f1))::return_type, int>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f1))::return_type, int>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f2))::return_type, std::string>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f2))::return_type, std::string>);

    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f0))::arguments_type, std::tuple<>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f0))::arguments_type, std::tuple<>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f1))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f1))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f2))::arguments_type, std::tuple<int, const std::string&>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f2))::arguments_type, std::tuple<int, const std::string&>>);

    using namespace udho::hazo::string::literals;

    auto slot_f0 = udho::url::slot("f0"_h, &f0);

    CHECK(slot_f0.args == 0);
    CHECK(slot_f0.key() == "f0"_h);

    std::string str = "hello";
    double d = 2.4;
    std::vector<std::string> args;
    args.push_back("24");
    args.push_back("world");
    args.push_back("2.42");
    args.push_back("0");

    auto f1_ = udho::url::detail::encapsulate_function(f1);
    CHECK(f1_.args == 4);
    CHECK(f1_(decltype(f1_)::decayed_arguments_type()) == 0);
    CHECK(f1_(decltype(f1_)::arguments_type(0, str, d, false)) == int(str.size()+d));
    CHECK(f1_(args.begin(), args.end()) == int(24+5+2.42+0));

    X x;

    auto xf1_ = udho::url::detail::encapsulate_mem_function(&X::f1, &x);
    CHECK(xf1_.args == 4);
    CHECK(xf1_(decltype(xf1_)::decayed_arguments_type()) == 0);
    CHECK(xf1_(decltype(xf1_)::arguments_type(0, str, d, false)) == int(str.size()+d));
    CHECK(xf1_(args.begin(), args.end()) == int(24+5+2.42+0));

    auto chain =
        udho::url::slot("f0"_h,  &f0)         << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &f1)         << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &f2)         << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")                       |
        udho::url::slot("xf0"_h, &X::f0, &x)  << udho::url::fixed(udho::url::verb::get, "/x/f0", "/x/f0")                                      |
        udho::url::slot("xf1"_h, &X::f1, &x)  << udho::url::regx(udho::url::verb::get,  "/x/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/x/f1/{}/{}/{}");

    // std::cout << chain << std::endl;
    {
        auto f0_ = chain["f0"_h];
        // CHECK(f0_(args.begin(), args.end()) == int(24+5+2.42+0));
        CHECK(f0_.invoke(boost::beast::http::verb::get, std::string("/")) == true);
        CHECK(f0_.fill(std::tuple<>()) == "/");
        CHECK(f0_() == "/");
        CHECK(f0_.symbol() == "f0()");

        auto f1_ = chain["f1"_h];
        CHECK(f1_(args.begin(), args.end()) == int(24+5+2.42+0));
        CHECK(f1_.invoke(boost::beast::http::verb::get, std::string("/f1/23/hello/24/1")) == true);
        CHECK(f1_.fill(std::make_tuple(24, "world", 2.4, 0)) == "/f1/24/world/2.4");
        CHECK(f1_(24, "world", 2.4, 0) == "/f1/24/world/2.4");

        auto f2_ = chain["f2"_h];
        CHECK(f2_(args.begin(), args.begin()+2) == "29");
        CHECK(f2_.invoke(boost::beast::http::verb::get, std::string("/f2-23/hello")) == true);
        CHECK(f2_.fill(std::make_tuple(24, "world")) == "/f2-24/world");
        CHECK(f2_(24, "world") == "/f2-24/world");

        auto xf0_ = chain["xf0"_h];
        // CHECK(xf0_(args.begin(), args.end()) == int(24+5+2.42+0));
        CHECK(xf0_.invoke(boost::beast::http::verb::get, std::string("/x/f0")) == true);
        CHECK(xf0_.fill(std::tuple<>()) == "/x/f0");
        CHECK(xf0_() == "/x/f0");
        CHECK(xf0_.symbol() == "X::f0()");

        auto xf1_ = chain["xf1"_h];
        CHECK(xf1_(args.begin(), args.end()) == int(24+5+2.42+0));
        CHECK(xf1_.invoke(boost::beast::http::verb::get, std::string("/x/f1/23/hello/24/1")) == true);
        CHECK(xf1_.fill(std::make_tuple(24, "world", 2.4, 0)) == "/x/f1/24/world/2.4");
        CHECK(xf1_(24, "world", 2.4, 0) == "/x/f1/24/world/2.4");
        CHECK(xf1_.symbol() == "X::f1(int, std::string const&, double const&, bool)");
    }

    auto ft_ = udho::url::detail::encapsulate_function(&f_nodef);
    CHECK(f_nodef(nodef(2), 42) == "hello");

    auto chain2 =
        udho::url::regx(udho::url::verb::get, "/x/f2-(\\d+)/(\\w+)", "/x/f2-{}/{}")                  >> udho::url::slot("xf2"_h, &X::f2, &x)  |
        udho::url::regx(udho::url::verb::get, "/x/f3/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/x/f3/{}/{}/{}") >> udho::url::slot("xf3"_h, &X::f3, &x);


    auto chain3 = chain | chain2;
    // chain3.xyz;
    // std::cout << chain3 << std::endl;

    auto mount_point = udho::url::mount("chain"_h, "/pchain", std::move(chain));
    CHECK(mount_point.find(boost::beast::http::verb::get, std::string("/")) == true);
    CHECK(mount_point.find(boost::beast::http::verb::get, std::string("/f1/23/hello/24/1")) == true);
    CHECK(mount_point.find(boost::beast::http::verb::get, std::string("/f1")) == false);
    CHECK(mount_point.find(boost::beast::http::verb::get, std::string("/f0/23/hello/24/1")) == false);
    CHECK(mount_point.invoke(boost::beast::http::verb::get, std::string("/")) == true);
    CHECK(mount_point.invoke(boost::beast::http::verb::get, std::string("/f1/23/hello/24/1")) == true);
    CHECK(mount_point.invoke(boost::beast::http::verb::get, std::string("/f1")) == false);
    CHECK(mount_point.invoke(boost::beast::http::verb::get, std::string("/f0/23/hello/24/1")) == false);
    auto f0_ = mount_point["f0"_h];
    CHECK(f0_() == "/");
    CHECK(mount_point("f1"_h, 24, "world", 2.4, 0) == "/pchain/f1/24/world/2.4");
    CHECK(mount_point.fill("f1"_h, std::make_tuple(24, "world", 2.4, 0)) == "/pchain/f1/24/world/2.4");

    CHECK(mount_point.index_of(boost::beast::http::verb::get, "/") == 0);
    CHECK(mount_point.index_of(boost::beast::http::verb::get, "/f1/23/hello/24/1") == 1);
    CHECK(mount_point.index_of(boost::beast::http::verb::get, "/f2-23/hello") == 2);
    CHECK(mount_point.index_of(boost::beast::http::verb::get, "/x/f0") == 3);
    CHECK(mount_point.index_of(boost::beast::http::verb::get, "/x/f1/23/hello/24/1") == 4);

    // std::cout << mount_point << std::endl;
    auto m2 = udho::url::mount("root"_h, "/", std::move(chain3));

    CHECK(m2.index_of(boost::beast::http::verb::get, "/x/f2-23/hello") == 5);
    CHECK(m2.index_of(boost::beast::http::verb::get, "/x/f3/hello/world/24/1") == 6);

    {
        m2.invoke_at(5, "/x/f2-23/hello");
        CHECK(x._msg == "f2");
        CHECK(x._a == 23);
        CHECK(x._b == "hello");
    } {
        m2.invoke_at(6, "/x/f3/42/world/24/1");
        CHECK(x._msg == "f3");
        CHECK(x._a == 42);
        CHECK(x._b == "world");
        CHECK(x._c == Catch::Approx(24.0));
        CHECK(x._d == true);
    }


    auto chain4 = std::move(mount_point) | std::move(m2);

    std::cout << "chain4" << std::endl << chain4 << std::endl;

    auto router = udho::url::router(std::move(chain4));

    CHECK(router["chain"_h]["f0"_h].symbol()                   == "f0()");

    auto chain_f1_index = router.index_of(boost::beast::http::verb::get, std::string("/pchain/f1/23/hello/24/1"));
    CHECK(chain_f1_index.valid());
    CHECK(chain_f1_index.mountpoint() == 0);
    CHECK(chain_f1_index.action() == 1);

    auto root_xf2_index = router.index_of(boost::beast::http::verb::get, std::string("/x/f2-23/hello"));
    CHECK(root_xf2_index.valid());
    CHECK(root_xf2_index.mountpoint() == 1);
    CHECK(root_xf2_index.action() == 5);

    router.table().invoke_at(root_xf2_index);
    CHECK(x._msg == "f2");
    CHECK(x._a == 23);
    CHECK(x._b == "hello");

    auto mounted_xf1_index = router.index_of(boost::beast::http::verb::get, std::string("/pchain/x/f1/31/mounted/12/0"));
    REQUIRE(mounted_xf1_index.valid());
    CHECK(mounted_xf1_index.mountpoint() == 0);
    CHECK(mounted_xf1_index.action() == 4);

    router.table().invoke_at(mounted_xf1_index);
    CHECK(x._msg == "f1");
    CHECK(x._a == 31);
    CHECK(x._b == "mounted");
    CHECK(x._c == Catch::Approx(12.0));
    CHECK(x._d == false);

    CHECK(router.find(boost::beast::http::verb::get, std::string("/pchain/"))                 == true);
    CHECK(router.find(boost::beast::http::verb::get, std::string("/pchain"))                  == true);
    CHECK(router.find(boost::beast::http::verb::get, std::string("/pchain/f1/23/hello/24/1")) == true);
    CHECK(router.find(boost::beast::http::verb::get, std::string("/f1/23/hello/24/1"))        == true);
    CHECK(router.find(boost::beast::http::verb::get, std::string("f1/23/hello/24/1"))         == false);
}

TEST_CASE("Router index lookup negative paths", "[url][router][invalid]") {
    using namespace udho::hazo::string::literals;

    auto actions =
        udho::url::slot("f0"_h, &f0) <<
        udho::url::fixed(udho::url::verb::get, "/action", "/action");
    auto router = udho::url::router(udho::url::mount("mounted"_h, "/mounted", std::move(actions)));

    SECTION("URL matches neither a mount point nor an action") {
        auto index = router.index_of(boost::beast::http::verb::get, std::string("/unknown/not-an-action"));

        CHECK_FALSE(index.valid());
    }

    SECTION("URL matches the mount point but no action in it") {
        auto index = router.index_of(boost::beast::http::verb::get, std::string("/mounted/not-an-action"));

        CHECK_FALSE(index.valid());
    }

    SECTION("URL matches an action path but not its mount point") {
        auto index = router.index_of(boost::beast::http::verb::get, std::string("/action"));

        CHECK_FALSE(index.valid());
    }
}

TEST_CASE("Extended pattern matching operations", "[url][pattern][extended]") {
    SECTION("Fixed pattern edge cases") {
        auto matcher = udho::url::fixed(udho::url::verb::get, "/fixed//path", "/fixed//path");
        CHECK(matcher.find("/fixed//path") == true);
        CHECK(matcher.find("/fixed/path") == false);

        auto case_sensitive = udho::url::fixed(udho::url::verb::get, "/CASE", "/CASE");
        CHECK(case_sensitive.find("/case") == false);
    }

    SECTION("Home pattern edge cases") {
        auto matcher = udho::url::home(udho::url::verb::get);
        CHECK(matcher.find("") == true);
        CHECK(matcher.find("/") == true);
        CHECK(matcher.find("/home") == false);
        CHECK(matcher.find("//") == false);
    }

    SECTION("P1729 pattern with different types") {
        auto matcher = udho::url::scan(udho::url::verb::get, "/data/{:d}/{:f}/{}", "/data/{}/{}/{}");

        std::string subject = "/data/42/3.14/hello";
        std::tuple<int, float, std::string> args;
        REQUIRE(matcher.find(subject, args) == true);
        REQUIRE(std::get<0>(args) == 42);
        REQUIRE(std::get<1>(args) == Catch::Approx(3.14));
        REQUIRE(std::get<2>(args) == "hello");

        SECTION("Replacement correctness") {
            auto replaced = matcher.replace(std::make_tuple(100, 2.718, "world"));
            REQUIRE(replaced == "/data/100/2.718/world");
        }
    }
}
