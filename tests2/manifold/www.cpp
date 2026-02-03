#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/url/url.h>
#include <udho/manifold/www.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/fabric.h>
#include <udho/manifold/pipeline.h>
#include <udho/manifold/features.h>
#include <udho/manifold/config.h>
#include <udho/manifold/components/navigator.h>
#include <udho/manifold/config.h>
#include <udho/manifold/journal.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/manifold/components/handler.h>
#include <udho/manifold/components/protocol.h>
#include <udho/manifold/components/routing.h>
#include <udho/manifold/components/cookies.h>
#include <udho/manifold/components/session.h>
#include <udho/manifold/components/pg.h>
#include <udho/manifold/components/resources.h>
#include <udho/session/storage/fs.h>
#include <udho/session/storage/fs_mem.h>
#include <udho/session/storage/redis.h>
#include <udho/manifold/portal.h>
#include <udho/manifold/context.h>
#include <udho/manifold/runtime.h>
#include <udho/manifold/composition_view.h>
#include <udho/manifold/journal_view.h>
#include <udho/manifold/configs_view.h>
#include <udho/manifold/transition.h>
#include <udho/manifold/journal.h>
#include <udho/manifold/flow.h>
#include <udho/manifold/visualize.h>
#include <udho/view/bridges/lua.h>
#include <udho/net/listener.h>

namespace callbacks{

    using namespace udho::manifold::www;
    using namespace udho::manifold::components;

    struct nodef{
        nodef() = delete;
        nodef(int) {}
    };

    static void f0(context<cookies> context){
        context << "f0";
        context.finish();
        return;
    }

    static int f1(context<navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
        context << std::to_string(a.size()+b.size()+c+d);
        std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
        context.finish();
        return 42;
    }

    static std::string f2(context<cookies> context, int a, const std::string& b){
        context << std::to_string(a+b.size());
        context.finish();
        return "hello";
    }

    static std::string f_nodef(context<> context, nodef, int a){
        context << std::to_string(a);
        context.finish();
        return "hello";
    }
};

auto url() {
    using namespace udho::hazo::string::literals;
    auto actions1 =
        udho::url::slot("f0"_h,  &callbacks::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto actions2 =
        udho::url::slot("f0"_h,  &callbacks::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto actions3 =
        udho::url::slot("f0"_h,  &callbacks::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &callbacks::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &callbacks::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    udho::url::mount_point mount_point1{"root"_h, "/",    std::move(actions1)};
    udho::url::mount_point mount_point2{"m2"_h,   "/m2",  std::move(actions2)};
    udho::url::mount_point mount_point3{"m3"_h,   "/m3",  std::move(actions2)};

    auto table      = std::move(mount_point1)/* | std::move(mount_point2) | std::move(mount_point3)*/;

    return table;
}


TEST_CASE("udho manifold www pipeline", "[manifold][pipeline][www]") {
    boost::asio::io_context io;

    // { session component
    using catalogue_type = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;
    catalogue_type catalogue{udho::session::storage::fs{}};
    auto session    = udho::manifold::components::session(catalogue);
    // }

    // { resources: views, assets, docroot
    udho::view::data::bridges::lua lua;
    lua.init();
    udho::view::resources::store<udho::view::data::bridges::lua> store{lua};
    // populate(store)
    store.lock();
    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{store};
    auto resources  = udho::manifold::components::resources(cstore);
    // }

    // { resources: assets, docroot
    // udho::view::resources::store<> store;
    // // populate(store)
    // store.lock();
    // udho::view::resources::const_store<> cstore{store};
    // auto resources  = udho::manifold::components::resources(cstore);
    // }

    using framework_type = udho::manifold::framework<udho::manifold::www::stateful::lua::lazy_fs>;
    using endpoint_type  = typename framework_type::endpoint_type;

    auto framework = framework_type::apply(url());
    auto runtime   = framework.runtime(session, resources);
    auto listener  = udho::net::listener(io, runtime, {boost::asio::ip::tcp::v4(), 9999});

    listener.start();

    io.run_for(std::chrono::seconds(10));
}

