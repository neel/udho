#include <catch2/catch_test_macros.hpp>
#include <udho/url/url.h>
#include <iostream>

#include <udho/net/listener.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/manifold/fabric.h>
#include <curl/curl.h>
#include <udho/www/www.h>
#include <udho/logging/setup.h>

using stream_type = udho::net::types::socket;

using namespace udho::www::components;
using namespace udho::www;

namespace callbacks{

struct nodef{
    nodef() = delete;
    nodef(int) {}
};

using namespace udho::www;
using namespace udho::www::components;

BOOST_SYMBOL_EXPORT void f0(context<cookies> context){
    context << "f0";
    context.finish();
    return;
}

BOOST_SYMBOL_EXPORT int f1(context<navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
    context << std::to_string(a.size()+b.size()+c+d);
    std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
    context.finish();
    return 42;
}

BOOST_SYMBOL_EXPORT std::string f2(context<cookies> context, int a, const std::string& b){
    context << std::to_string(a+b.size());
    context.finish();
    return "hello";
}

BOOST_SYMBOL_EXPORT std::string f_nodef(context<> context, nodef, int a){
    context << std::to_string(a);
    context.finish();
    return "hello";
}

}

TEST_CASE("URL router construction", "[url][routing][construction]") {
    using namespace udho::hazo::string::literals;

    auto actions_1 =
        udho::url::slot("f0"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get)
    ;

    static_assert(udho::url::is_action<decltype(actions_1)>::value);
    static_assert(actions_1.key() == "f0"_h);

    auto actions_2 =
        udho::url::slot("f0"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)         << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")
    ;

    static_assert(udho::url::is_action_table<decltype(actions_2)>::value);
    static_assert(actions_2.length() == 2);
    CHECK(actions_2["f0"_h].key() == "f0"_h);
    CHECK(actions_2["f1"_h].key() == "f1"_h);

    auto actions_3 =
        udho::url::slot("f0"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)         << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)         << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    static_assert(udho::url::is_action_table<decltype(actions_3)>::value);
    static_assert(actions_3.length() == 3);
    CHECK(actions_3["f0"_h].key() == "f0"_h);
    CHECK(actions_3["f1"_h].key() == "f1"_h);
    CHECK(actions_3["f2"_h].key() == "f2"_h);

    SECTION("chain of actions constructed through concatenation") {
        auto action_a = udho::url::slot("f0"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get);
        auto action_b = udho::url::slot("f1"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get);
        auto action_c = udho::url::slot("f2"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get);
        auto action_d = udho::url::slot("f3"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get);
        auto action_e = udho::url::slot("f4"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get);
        auto action_f = udho::url::slot("f5"_h,  &callbacks::f0)         << udho::url::home(udho::url::verb::get);

        auto chain_2 = std::move(action_a) | std::move(action_b);
        auto chain_3 = std::move(chain_2)  | std::move(action_c);

        static_assert(udho::url::is_action_table<decltype(chain_2)>::value);
        static_assert(chain_2.length() == 2);

        CHECK(chain_2.at<0>().key() == "f0"_h);
        CHECK(chain_2.at<1>().key() == "f1"_h);

        static_assert(udho::url::is_action_table<decltype(chain_3)>::value);
        static_assert(chain_3.length() == 3);

        CHECK(chain_3.at<0>().key() == "f0"_h);
        CHECK(chain_3.at<1>().key() == "f1"_h);
        CHECK(chain_3.at<2>().key() == "f2"_h);

        auto chain_x = std::move(action_d) | std::move(action_e) | std::move(action_f);

        SECTION("action <> action table") {
            static_assert(udho::url::is_action_table<decltype(chain_x)>::value);
            static_assert(chain_x.length() == 3);

            CHECK(chain_x.at<0>().key() == "f3"_h);
            CHECK(chain_x.at<1>().key() == "f4"_h);
            CHECK(chain_x.at<2>().key() == "f5"_h);
        }

        SECTION("action table <> action table 2+3") {
            auto chain_m = std::move(chain_2) | std::move(chain_x);

            static_assert(udho::url::is_action_table<decltype(chain_m)>::value);
            static_assert(chain_m.length() == 5);

            CHECK(chain_m.at<0>().key() == "f0"_h);
            CHECK(chain_m.at<1>().key() == "f1"_h);
            CHECK(chain_m.at<2>().key() == "f3"_h);
            CHECK(chain_m.at<3>().key() == "f4"_h);
            CHECK(chain_m.at<4>().key() == "f5"_h);
        }

        SECTION("action table <> action table 3+3") {
            auto chain_m = std::move(chain_3) | std::move(chain_x);

            static_assert(udho::url::is_action_table<decltype(chain_m)>::value);
            static_assert(chain_m.length() == 6);

            CHECK(chain_m.at<0>().key() == "f0"_h);
            CHECK(chain_m.at<1>().key() == "f1"_h);
            CHECK(chain_m.at<2>().key() == "f2"_h);
            CHECK(chain_m.at<3>().key() == "f3"_h);
            CHECK(chain_m.at<4>().key() == "f4"_h);
            CHECK(chain_m.at<5>().key() == "f5"_h);
        }
    }

    SECTION("mount points") {
        auto mp_1 = udho::url::mount("prefix1"_h, "/path1", std::move(actions_1));
        auto mp_2 = udho::url::mount("prefix2"_h, "/path2", std::move(actions_2));
        auto mp_3 = udho::url::mount("prefix3"_h, "/path3", std::move(actions_3));

        static_assert(mp_1.length() == 1);
        CHECK(mp_1.name() == "prefix1");
        CHECK(mp_1.key()  == "prefix1"_h);
        CHECK(mp_1.path() == "/path1");
        CHECK(mp_1["f0"_h].key() == "f0"_h);

        static_assert(mp_2.length() == 2);
        CHECK(mp_2.name() == "prefix2");
        CHECK(mp_2.key()  == "prefix2"_h);
        CHECK(mp_2.path() == "/path2");
        CHECK(mp_2["f0"_h].key() == "f0"_h);
        CHECK(mp_2["f1"_h].key() == "f1"_h);

        static_assert(mp_3.length() == 3);
        CHECK(mp_3.name() == "prefix3");
        CHECK(mp_3.key()  == "prefix3"_h);
        CHECK(mp_3.path() == "/path3");
        CHECK(mp_3["f0"_h].key() == "f0"_h);
        CHECK(mp_3["f1"_h].key() == "f1"_h);
        CHECK(mp_3["f2"_h].key() == "f2"_h);

        auto router_1 = udho::url::router(std::move(mp_1));
        auto router_2 = udho::url::router(std::move(mp_2));
        auto router_3 = udho::url::router(std::move(mp_3));
    }

    SECTION("router") {
        auto mp_1 = udho::url::mount("prefix1"_h, "/path1", std::move(actions_1));
        auto mp_2 = udho::url::mount("prefix2"_h, "/path2", std::move(actions_2));
        auto mp_3 = udho::url::mount("prefix3"_h, "/path3", std::move(actions_3));

        SECTION("single mountpoint of different sizes") {
            auto router_1 = udho::url::router(std::move(mp_1));
            auto router_2 = udho::url::router(std::move(mp_2));
            auto router_3 = udho::url::router(std::move(mp_3));
        }

        SECTION("mountpoints table") {
            auto mtab_1_2 = std::move(mp_1) | std::move(mp_2);
            auto mtab_2_1 = std::move(mp_2) | std::move(mp_1);
            auto mtab_1_3 = std::move(mp_1) | std::move(mp_3);
            auto mtab_3_1 = std::move(mp_3) | std::move(mp_1);
            auto mtab_2_3 = std::move(mp_2) | std::move(mp_3);
            auto mtab_3_2 = std::move(mp_3) | std::move(mp_2);

            static_assert(udho::url::is_mountpoints_table<decltype(mtab_1_2)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_2_1)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_1_3)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_3_1)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_2_3)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_3_2)>::value);

            static_assert(mtab_1_2.length() == 2);

            CHECK(mtab_1_2.at<0>().key() == "prefix1"_h);
            CHECK(mtab_1_2.at<1>().key() == "prefix2"_h);

            static_assert(mtab_2_1.length() == 2);
            static_assert(mtab_1_3.length() == 2);
            static_assert(mtab_3_1.length() == 2);
            static_assert(mtab_2_3.length() == 2);
            static_assert(mtab_3_2.length() == 2);

            CHECK(mtab_2_1.at<0>().key() == "prefix2"_h);
            CHECK(mtab_2_1.at<1>().key() == "prefix1"_h);

            CHECK(mtab_1_3.at<0>().key() == "prefix1"_h);
            CHECK(mtab_1_3.at<1>().key() == "prefix3"_h);

            CHECK(mtab_3_1.at<0>().key() == "prefix3"_h);
            CHECK(mtab_3_1.at<1>().key() == "prefix1"_h);

            CHECK(mtab_2_3.at<0>().key() == "prefix2"_h);
            CHECK(mtab_2_3.at<1>().key() == "prefix3"_h);

            CHECK(mtab_3_2.at<0>().key() == "prefix3"_h);
            CHECK(mtab_3_2.at<1>().key() == "prefix2"_h);

            auto mtab_1_2_3 = std::move(mp_1) | std::move(mp_2) | std::move(mp_3);
            auto mtab_3_2_1 = std::move(mp_3) | std::move(mp_2) | std::move(mp_1);
            auto mtab_2_3_1 = std::move(mp_2) | std::move(mp_3) | std::move(mp_1);

            static_assert(udho::url::is_mountpoints_table<decltype(mtab_1_2_3)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_3_2_1)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_2_3_1)>::value);

            static_assert(mtab_1_2_3.length() == 3);

            CHECK(mtab_1_2_3.at<0>().key() == "prefix1"_h);
            CHECK(mtab_1_2_3.at<1>().key() == "prefix2"_h);
            CHECK(mtab_1_2_3.at<2>().key() == "prefix3"_h);

            static_assert(mtab_3_2_1.length() == 3);
            static_assert(mtab_2_3_1.length() == 3);

            CHECK(mtab_3_2_1.at<0>().key() == "prefix3"_h);
            CHECK(mtab_3_2_1.at<1>().key() == "prefix2"_h);
            CHECK(mtab_3_2_1.at<2>().key() == "prefix1"_h);

            CHECK(mtab_2_3_1.at<0>().key() == "prefix2"_h);
            CHECK(mtab_2_3_1.at<1>().key() == "prefix3"_h);
            CHECK(mtab_2_3_1.at<2>().key() == "prefix1"_h);

            auto mtab_1_2_x_m1 = std::move(mtab_1_2) | std::move(mp_1);
            auto mtab_1_2_x_m2 = std::move(mtab_1_2) | std::move(mp_2);
            auto mtab_1_2_x_m3 = std::move(mtab_1_2) | std::move(mp_3);

            static_assert(udho::url::is_mountpoints_table<decltype(mtab_1_2_x_m1)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_1_2_x_m2)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_1_2_x_m3)>::value);

            static_assert(mtab_1_2_x_m1.length() == 3);
            static_assert(mtab_1_2_x_m2.length() == 3);
            static_assert(mtab_1_2_x_m3.length() == 3);

            CHECK(mtab_1_2_x_m1.at<0>().key() == "prefix1"_h);
            CHECK(mtab_1_2_x_m1.at<1>().key() == "prefix2"_h);
            CHECK(mtab_1_2_x_m1.at<2>().key() == "prefix1"_h);

            CHECK(mtab_1_2_x_m2.at<0>().key() == "prefix1"_h);
            CHECK(mtab_1_2_x_m2.at<1>().key() == "prefix2"_h);
            CHECK(mtab_1_2_x_m2.at<2>().key() == "prefix2"_h);

            CHECK(mtab_1_2_x_m3.at<0>().key() == "prefix1"_h);
            CHECK(mtab_1_2_x_m3.at<1>().key() == "prefix2"_h);
            CHECK(mtab_1_2_x_m3.at<2>().key() == "prefix3"_h);

            auto mtab_m1_x_1_2 = std::move(mp_1) | std::move(mtab_1_2);
            auto mtab_m2_x_1_2 = std::move(mp_2) | std::move(mtab_1_2);
            auto mtab_m3_x_1_2 = std::move(mp_3) | std::move(mtab_1_2);

            static_assert(udho::url::is_mountpoints_table<decltype(mtab_m1_x_1_2)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_m2_x_1_2)>::value);
            static_assert(udho::url::is_mountpoints_table<decltype(mtab_m3_x_1_2)>::value);

            static_assert(mtab_m1_x_1_2.length() == 3);
            static_assert(mtab_m2_x_1_2.length() == 3);
            static_assert(mtab_m3_x_1_2.length() == 3);

            CHECK(mtab_m1_x_1_2.at<0>().key() == "prefix1"_h);
            CHECK(mtab_m1_x_1_2.at<1>().key() == "prefix1"_h);
            CHECK(mtab_m1_x_1_2.at<2>().key() == "prefix2"_h);

            CHECK(mtab_m2_x_1_2.at<0>().key() == "prefix2"_h);
            CHECK(mtab_m2_x_1_2.at<1>().key() == "prefix1"_h);
            CHECK(mtab_m2_x_1_2.at<2>().key() == "prefix2"_h);

            CHECK(mtab_m3_x_1_2.at<0>().key() == "prefix3"_h);
            CHECK(mtab_m3_x_1_2.at<1>().key() == "prefix1"_h);
            CHECK(mtab_m3_x_1_2.at<2>().key() == "prefix2"_h);

            auto mtab_1_2_x_2_3 = std::move(mtab_1_2) | std::move(mtab_2_3);

            static_assert(udho::url::is_mountpoints_table<decltype(mtab_1_2_x_2_3)>::value);

            static_assert(mtab_1_2_x_2_3.length() == 4);

            CHECK(mtab_1_2_x_2_3.at<0>().key() == "prefix1"_h);
            CHECK(mtab_1_2_x_2_3.at<1>().key() == "prefix2"_h);
            CHECK(mtab_1_2_x_2_3.at<2>().key() == "prefix2"_h);
            CHECK(mtab_1_2_x_2_3.at<3>().key() == "prefix3"_h);
        }
    }
}
