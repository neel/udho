#define CATCH_CONFIG_MAIN

#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

#include <udho/url/url.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/fabric.h>
#include <udho/manifold/pipeline.h>
#include <udho/manifold/features.h>
#include <udho/manifold/config.h>
#include <udho/manifold/config.h>
#include <udho/manifold/journal.h>
#include <boost/beast/_experimental/test/stream.hpp>
#include <udho/www/components/handler.h>
#include <udho/www/components/protocol.h>
#include <udho/www/components/routing.h>
#include <udho/www/components/cookies.h>
#include <udho/www/components/session.h>
#include <udho/www/components/pg.h>
#include <udho/www/components/navigator.h>
#include <udho/www/components/resources.h>
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
#include <udho/view/bridges/lua.h>
#include <udho/net/listener.h>


using namespace udho::www::components;
using namespace udho::manifold;

template <typename StreamT>
struct basic_callbacks{
    using handler = basic_handler<StreamT>;

    struct nodef{
        nodef() = delete;
        nodef(int) {}
    };

    static BOOST_SYMBOL_EXPORT void f0(basic_context<StreamT, handler, cookies> context){
        context << "f0";
        context.finish();
        return;
    }

    static BOOST_SYMBOL_EXPORT int f1(basic_context<StreamT, handler, navigators::pretty, cookies> context, std::string a, const std::string& b, const double& c, int d){
        context << std::to_string(a.size()+b.size()+c+d);
        std::cout << "context.resource(): " << context.portal().resource()  << std::endl;
        context.finish();
        return 42;
    }

    static BOOST_SYMBOL_EXPORT std::string f2(basic_context<StreamT, handler, cookies> context, int a, const std::string& b){
        context << std::to_string(a+b.size());
        context.finish();
        return "hello";
    }

    static BOOST_SYMBOL_EXPORT std::string f_nodef(basic_context<StreamT, handler> context, nodef, int a){
        context << std::to_string(a);
        context.finish();
        return "hello";
    }
};

using test_callbacks = basic_callbacks<boost::beast::test::stream>;

namespace testing{

template <typename StreamT>
auto router() {
    using namespace udho::hazo::string::literals;
    auto actions1 =
        udho::url::slot("f0"_h,  &basic_callbacks<StreamT>::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &basic_callbacks<StreamT>::f1)  << udho::url::regx(udho::url::verb::post, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &basic_callbacks<StreamT>::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto actions2 =
        udho::url::slot("f0"_h,  &basic_callbacks<StreamT>::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &basic_callbacks<StreamT>::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &basic_callbacks<StreamT>::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto actions3 =
        udho::url::slot("f0"_h,  &basic_callbacks<StreamT>::f0)  << udho::url::home(udho::url::verb::get)                                                         |
        udho::url::slot("f1"_h,  &basic_callbacks<StreamT>::f1)  << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
        udho::url::slot("f2"_h,  &basic_callbacks<StreamT>::f2)  << udho::url::regx(udho::url::verb::get, "/f2-(\\d+)/(\\w+)", "/f2-{}/{}")
    ;

    auto mount_point1 = udho::url::mount("root"_h, "/",    std::move(actions1));
    auto mount_point2 = udho::url::mount("m2"_h,   "/m2",  std::move(actions2));
    auto mount_point3 = udho::url::mount("m3"_h,   "/m3",  std::move(actions2));

    auto table      = std::move(mount_point1) | std::move(mount_point2) | std::move(mount_point3);

    return udho::url::router(std::move(table));
}

auto test_url() {
    return router<boost::beast::test::stream>();
}

auto tcp_url() {
    return router<udho::net::detail::wire_types<boost::asio::ip::tcp>::socket_type>();
}

template <typename StreamT>
using test_case_router_type = std::decay_t<decltype(router<StreamT>())>;


template <typename StreamT>
struct basic_www{
    using router_type                = test_case_router_type<StreamT>;
    using handler_component_type     = udho::www::components::basic_handler<StreamT>;
    using db_component_type          = udho::www::components::db::pg<>;
    using routing_component_type     = udho::www::components::routing<router_type>;
    using stream_type                = StreamT;
    using protocol_component_type    = udho::www::components::protocols::http<stream_type>;
    using navigator_component_type   = udho::www::components::navigators::pretty;
    using cookies_component_type     = udho::www::components::cookies;
    using session_component_type     = udho::www::components::session<udho::session::storage::fs, udho::session::modes::lazy>;
    using resources_component_type   = udho::www::components::resources<udho::view::data::bridges::lua>;
};

}

template <typename StreamT>
struct udho::manifold::sketch<testing::basic_www<StreamT>>{
    using www_type = testing::basic_www<StreamT>;

    using stream_type = StreamT;

    using composition_type = udho::manifold::composition<
        typename www_type::handler_component_type,
        typename www_type::db_component_type,
        typename www_type::protocol_component_type,
        typename www_type::navigator_component_type,
        typename www_type::routing_component_type,
        typename www_type::cookies_component_type,
        typename www_type::session_component_type,
        typename www_type::resources_component_type
    >;

    using order_type = udho::manifold::order<
        udho::www::feature::header_reader,
        udho::www::feature::identifier,
        udho::www::feature::locator,
        udho::www::feature::cookie_load,
        udho::www::feature::session_load,
        udho::www::feature::body_reader
    >;
};

namespace testing{

template <typename StreamT = boost::beast::test::stream>
struct framework{
    using label_type             = testing::basic_www<StreamT>;
    using sketch_type            = udho::manifold::sketch<label_type>;
    using runtime_type           = udho::manifold::basic_runtime<label_type, StreamT>;
    using flow_type              = typename runtime_type::flow_type;
    using composition_type       = typename runtime_type::composition_type;
    using routing_component_type = typename label_type::routing_component_type;
    using router_type            = typename label_type::router_type;
    using handler_component_type = typename label_type::handler_component_type;

    static_assert(runtime_type::Count >= 2);

    template <typename... Components>
    framework(test_case_router_type<StreamT>&& router, Components&&... components)
        : _handler(router.summary()), _routing(std::move(router)), _runtime(_routing, _handler, std::forward<Components>(components)...) {}

    runtime_type& runtime() { return _runtime; }

private:
    handler_component_type  _handler;
    routing_component_type  _routing;
    runtime_type            _runtime;
};

}

template <typename StreamT>
struct udho::manifold::basic_terminal<testing::basic_www<StreamT>, StreamT> {
    using label_type        = testing::basic_www<StreamT>;
    using stream_type       = StreamT;
    using ostream_type      = udho::net::basic_ostream<StreamT>;
    using runtime_type      = basic_runtime<label_type, StreamT>;
    using handler_type      = udho::www::components::basic_handler<StreamT>;
    using flow_type         = typename runtime_type::flow_type;
    using composition_type  = typename runtime_type::composition_type;
    using journal_type      = typename flow_type::journal_type;
    using configs_type      = typename runtime_type::configs_type;

    basic_terminal() = delete;
    basic_terminal(const basic_terminal&) = delete;

    basic_terminal(composition_type& composition, const configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

    basic_terminal(basic_terminal&&) = delete;

    bool reenter(stream_type& stream) { return true; }

    template <typename... Args>
    void prepare(stream_type& stream, Args&&... args) { }

    template <typename... Args>
    void internal_error(udho::manifold::evaluation_result success, flow_type& flow, stream_type& stream, Args&&... args){
        if(!success) {
            try{
                success.rethrow();
            } catch(const udho::http::error& error) {
                std::cout << "exception: " << error.what() << std::endl;
                handle_error(flow, error, stream, std::forward<Args>(args)...);
            } catch(boost::system::error_code error) {
                std::cout << "system error: " << error << std::endl;
                handle_error(flow, error, stream, std::forward<Args>(args)...);
            }catch(const std::exception& ex) {
                std::cout << "exception: " << ex.what() << std::endl;
                handle_error(flow, ex, stream, std::forward<Args>(args)...);
            }
        }
    }

    template <typename... Args>
    void user_error(const udho::exceptions::captured& capex, flow_type& flow, stream_type& stream, Args&&... args){
        handler_type& handler = _composition.template get<handler_type>().component();
        ostream_type& ostream = handler.ostream(flow.id()); // Expect ostream to exist

        assert(ostream.has_exception());

        try{
            capex.rethrow();
        } catch(const std::exception& exception) {
            handle_error(flow, exception, stream, std::forward<Args>(args)...);
        }
    }

private:

    template <typename... Args>
    void handle_error(flow_type& flow, const udho::http::error& error, stream_type& stream, Args&&... args) {
        ostream_type& ostream = get_ostream(flow, true, stream, std::forward<Args>(args)...);
        ostream.status(error.status());
        ostream.finish();
    }

    template <typename... Args>
    void handle_error(flow_type& flow, boost::system::error_code error, stream_type& stream, Args&&... args) {
        if(error == boost::asio::error::eof) {
            flow.abort();
        }

        flow.abort();
    }

    template <typename... Args>
    void handle_error(flow_type& flow, const std::exception& error, stream_type& stream, Args&&... args) {
        flow.abort();
    }

private:

    template <typename... Args>
    ostream_type& get_ostream(flow_type& flow, bool restart, stream_type& stream, Args&&... args) {
        auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        auto lambda = [&flow, restart, &stream, args_tuple = std::move(args_tuple)](boost::system::error_code error, std::size_t bytes_written){
            if(error) {
                // TODO Error while writing to socket
                return;
            }

            if(restart) {
                std::apply(
                    [&](auto&&... args) {
                        flow.restart(stream, std::forward<Args>(args)...);
                    },
                    args_tuple
                );
            } else {
                flow.abort();
            }
        };

        auto ex_lambda = [&flow, restart, &stream, args_tuple = std::move(args_tuple)](ostream_type& ostream){
            ostream.finish();
        };

        handler_type& handler = _composition.template get<handler_type>().component();
        ostream_type& ostream = handler.add(flow.id(), stream, std::move(lambda), std::move(ex_lambda));
        ostream.prepare();
        return ostream;
    }

private:
    composition_type&   _composition;
    const configs_type& _configs;
    const journal_type& _journal;
};


static constexpr const std::size_t route_locator_stage = udho::www::feature::locator::stage;
template <typename StreamT>
struct udho::manifold::transition<testing::basic_www<StreamT>, StreamT, route_locator_stage>{
    using label_type             = testing::basic_www<StreamT>;
    using stream_type            = StreamT;
    using sketch_type            = sketch<label_type>;
    using runtime_type           = basic_runtime<label_type, stream_type>;
    using flow_type              = typename runtime_type::flow_type;
    using composition_type       = typename runtime_type::composition_type;
    using journal_type           = typename flow_type::journal_type;
    using pipeline_type          = typename runtime_type::template pipeline_at<route_locator_stage>;
    using configs_type           = typename runtime_type::configs_type;
    using portal_type            = typename udho::manifold::detail::get_portal_type<composition_type>::type;
    using start_pipeline_type    = typename runtime_type::start_pipeline_type;
    using routing_component_type = typename label_type::routing_component_type;

    template <typename... Args>
    static void apply(flow_type& flow, pipeline_type& p, configs_type& config, Args&&... args) {
        // { essentials
        composition_type& composition = p.composition();
        const journal_type& journal   = p.journal();
        configs_type& configs         = p.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::www::feature::locator>();
        assert(route.ready());
        const udho::url::detail::route_index& route_index = *route;
        assert(route_index.valid());
        const routing_component_type& routing_component = composition.template get<routing_component_type>().component();
        const auto& router = routing_component.router();
        router.reconfigure_for(route_index, configs);
        // }
        p.next(flow, std::forward<Args>(args)...);
    }
};

static constexpr const std::size_t action_transition_stage = 2;

template <typename StreamT>
struct udho::manifold::transition<testing::basic_www<StreamT>, StreamT, action_transition_stage>{
    using label_type             = testing::basic_www<StreamT>;
    using stream_type            = StreamT;
    using sketch_type            = sketch<label_type>;
    using runtime_type           = basic_runtime<label_type, stream_type>;
    using flow_type              = typename runtime_type::flow_type;
    using composition_type       = typename runtime_type::composition_type;
    using journal_type           = typename flow_type::journal_type;
    using pipeline_type          = typename runtime_type::template pipeline_at<action_transition_stage>;
    using configs_type           = typename runtime_type::configs_type;
    using portal_type            = typename udho::manifold::detail::get_portal_type<composition_type>::type;
    using context_type           = typename udho::manifold::detail::get_context_for_portal<StreamT, portal_type>::type;
    using start_pipeline_type    = typename runtime_type::start_pipeline_type;
    using routing_component_type = typename label_type::routing_component_type;

    template <typename... Args>
    static void apply(flow_type& flow, pipeline_type& p, configs_type& config, StreamT& stream, Args&&... args) {
        // { essentials
        composition_type& composition = p.composition();
        const journal_type& journal   = p.journal();
        configs_type& configs         = p.configs();
        // }

        // { patch the configs as per the route
        const auto& route = journal.template at<udho::www::feature::locator>();
        assert(route.ready());
        const udho::url::detail::route_index& route_index = *route;
        assert(route_index.valid());
        const routing_component_type& routing_component = composition.template get<routing_component_type>().component();
        const auto& router = routing_component.router();
        // }

        // { add finish lambda to handler component
        auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        auto lambda = [&p, &stream, &flow, args_tuple = std::move(args_tuple)](boost::system::error_code error, std::size_t bytes_written){
            if(error) {
                // TODO Error while writing to socket
                return;
            }

            std::apply(
                [&](auto&&... args) {
                    p.next(flow, stream, std::forward<Args>(args)...);
                },
                args_tuple
            );
        };
        using handler_type = udho::www::components::basic_handler<StreamT>;
        using ostream_type = udho::net::basic_ostream<StreamT>;

        auto ex_lambda = [&flow, &stream, args_tuple = std::move(args_tuple)](ostream_type& ostream){
            if(ostream.has_exception()){
                const udho::exceptions::captured& capex = ostream.exception();
                std::apply(
                    [&](auto&&... args) {
                        flow.user_error(capex, stream, std::forward<Args>(args)...);
                    },
                    args_tuple
                );
            } else {
                ostream.finish();
            }
        };

        handler_type& handler = composition.template get<handler_type>().component();
        ostream_type& ostream = handler.add(flow.id(), stream, std::move(lambda), std::move(ex_lambda));
        ostream.prepare();
        // }

        // { create context
        portal_type portal(composition, configs, journal);
        std::string resource = portal.resource();
        std::cout << "resource: " << resource << std::endl;
        context_type context(ostream, portal, flow.id());
        // }

        // { invoke action
        router.invoke_at(route_index, context);
        // }
    }
};

static_assert(udho::www::feature::body_reader::stage > udho::www::feature::identifier::stage);

TEST_CASE("udho manifold pipeline stage 0", "[manifold][pipeline]") {
    boost::asio::io_context io_context;
    // { session component
    using catalogue_type = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;
    catalogue_type catalogue{udho::session::storage::fs{}};
    auto session    = udho::www::components::session(catalogue);
    // }
    // { resources: views, assets
    udho::view::data::bridges::lua lua;
    lua.init();

    udho::view::resources::store<udho::view::data::bridges::lua> store{lua};
    store.lock();
    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{store};
    auto resources  = udho::www::components::resources(cstore);
    // }

    std::string request_data =
        "POST /f1/hello/world/23/24?name=test&id=42&filter=active HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Cookie: session=s1; theme=dark; uid=42\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, World!"
        "POST /f1/hello/world/25/23?name=test&id=42&filter=active HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 13\r\n"
        "Cookie: session=s2; theme=light; uid=99\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, Earth!"
        ;
    boost::beast::test::stream stream_in(io_context, request_data);
    boost::beast::test::stream stream_out(io_context);
    stream_in.connect(stream_out);

    auto framework  = testing::framework(testing::test_url(), session, resources);
    auto& flow      = framework.runtime().spawn(std::move(stream_in));

    using framework_type = std::decay_t<decltype(framework)>;
    using runtime_type   = framework_type::runtime_type;
    using flow_type      = framework_type::flow_type;
    using journal_type   = flow_type::journal_type;


    using portal_type            = typename udho::manifold::detail::get_portal_type<framework_type::composition_type>::type;
    using context_type           = typename udho::manifold::detail::get_context_for_portal<boost::beast::test::stream, portal_type>::type;

    lua.bind(udho::view::data::type<context_type>{});


    std::size_t counter = 0;

    flow.then([&counter](const auto& flow, bool reenter) {
        std::cout << "finished: " << reenter << std::endl;
        const journal_type& journal = flow.journal();

        const auto& w_request    = journal.at<udho::www::feature::header_reader>();
        const auto& w_route_desc = journal.at<udho::www::feature::identifier>();
        const auto& w_uri        = journal.at<udho::www::feature::locator>();
        const auto& w_jar        = journal.at<udho::www::feature::cookie_load>();
        const auto& w_body       = journal.at<udho::www::feature::body_reader>();

        if(counter == 0) {
            CHECK(reenter);

            REQUIRE(w_request.ready());
            REQUIRE(w_route_desc.ready());
            REQUIRE(w_uri.ready());
            REQUIRE(w_jar.ready());
            REQUIRE(w_body.ready());

            const udho::www::feature::header_reader::result& request    = w_request;
            const udho::www::feature::identifier::result&    route_desc = w_route_desc;
            const udho::www::feature::locator::result&       uri        = w_uri;
            const udho::www::feature::cookie_load::result&   jar        = w_jar;
            const udho::www::feature::body_reader::result&   body       = w_body;

            CHECK(request.method() == boost::beast::http::verb::post);
            CHECK(request.target() == "/f1/hello/world/23/24?name=test&id=42&filter=active");
            CHECK(request[boost::beast::http::field::host] == "example.com");
            CHECK(request[boost::beast::http::field::content_type] == "text/plain");
            CHECK(request[boost::beast::http::field::content_length] == "13");

            CHECK(route_desc.resource() == "/f1/hello/world/23/24");
            {
                const auto& params = route_desc.params();
                auto get1 = [&](const std::string& k) -> std::string {
                    auto it = params.find(k);
                    REQUIRE(it != params.end());
                    return it->second;
                };
                CHECK(get1("name") == "test");
                CHECK(get1("id") == "42");
                CHECK(get1("filter") == "active");
            }

            CHECK(jar.count("session") == 1);
            CHECK(jar.count("theme")   == 1);
            CHECK(jar.count("uid")     == 1);

            {
                auto v = jar.by_name("session");
                REQUIRE(v.size() == 1);
                CHECK(v[0].name() == "session");
                CHECK(v[0].value() == "s1");
                CHECK(!v[0].domain().has_value());
                CHECK(!v[0].path().has_value());
            } {
                auto v = jar.by_name("theme");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "dark");
            } {
                auto v = jar.by_name("uid");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "42");
            }


            CHECK(body.str() == "Hello, World!");
            CHECK(body.bytes_transferred() == 13);
            CHECK(body.error() == std::error_code{});

        } else if(counter == 1) {
            CHECK(reenter);

            CHECK(w_request.ready());
            CHECK(w_route_desc.ready());
            CHECK(w_uri.ready());
            CHECK(w_jar.ready());
            CHECK(w_body.ready());

            const udho::www::feature::header_reader::result& request    = w_request;
            const udho::www::feature::identifier::result&    route_desc = w_route_desc;
            const udho::www::feature::locator::result&       uri        = w_uri;
            const udho::www::feature::cookie_load::result&   jar        = w_jar;
            const udho::www::feature::body_reader::result&   body       = w_body;

            CHECK(request.method() == boost::beast::http::verb::post);
            CHECK(request.target() == "/f1/hello/world/25/23?name=test&id=42&filter=active");
            CHECK(request[boost::beast::http::field::host] == "example.com");
            CHECK(request[boost::beast::http::field::content_type] == "text/plain");
            CHECK(request[boost::beast::http::field::content_length] == "13");

            CHECK(route_desc.resource() == "/f1/hello/world/25/23");
            {
                const auto& params = route_desc.params();
                auto get1 = [&](const std::string& k) -> std::string {
                    auto it = params.find(k);
                    REQUIRE(it != params.end());
                    return it->second;
                };
                CHECK(get1("name") == "test");
                CHECK(get1("id") == "42");
                CHECK(get1("filter") == "active");
            }

            CHECK(jar.count("session") == 1);
            CHECK(jar.count("theme")   == 1);
            CHECK(jar.count("uid")     == 1);

            {
                auto v = jar.by_name("session");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "s2");
            } {
                auto v = jar.by_name("theme");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "light");
            } {
                auto v = jar.by_name("uid");
                REQUIRE(v.size() == 1);
                CHECK(v[0].value() == "99");
            }

            CHECK(body.str() == "Hello, Earth!");
            CHECK(body.bytes_transferred() == 13);
            CHECK(body.error() == std::error_code{});

        } else if(counter == 2) {
            CHECK(!reenter);
            REQUIRE(!w_request.ready());
        }

        ++counter;
    });

    flow.start();


    // the callback gets called in two circumstances
    //  1. pipeline finished processing
    //  2. pipeline encountered error
    // in both circumstances a decision whether to reenter or not
    // has been made already which is passed to reenter argument

    io_context.run();

    CHECK(counter == 3);
    CHECK(framework.runtime().count() == 0);

    std::string output = stream_out.str();
    std::cout << "stream_out: " << std::endl << output << std::endl;
}

TEST_CASE("udho manifold pipeline stage 0 with tcp stream", "[manifold][pipeline]") {
    boost::asio::io_context io;
    // { session component
    using catalogue_type = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;
    catalogue_type catalogue{udho::session::storage::fs{}};
    auto session    = udho::www::components::session(catalogue);
    // }
    // { resources: views, assets
    udho::view::data::bridges::lua lua;
    lua.init();

    udho::view::resources::store<udho::view::data::bridges::lua> store{lua};
    store.lock();
    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{store};
    auto resources  = udho::www::components::resources(cstore);
    // }

    using socket_type    = udho::net::detail::wire_types<boost::asio::ip::tcp>::socket_type;
    using framework_type = testing::framework<socket_type>;
    using portal_type    = typename udho::manifold::detail::get_portal_type<framework_type::composition_type>::type;
    using context_type   = typename udho::manifold::detail::get_context_for_portal<socket_type, portal_type>::type;
    using listener_type  = udho::net::basic_listener<boost::asio::ip::tcp, framework_type::runtime_type>;
    using endpoint_type  = typename listener_type::endpoint_type;

    auto framework  = framework_type(testing::tcp_url(), session, resources);

    lua.bind(udho::view::data::type<context_type>{});

    listener_type listener(io, framework.runtime(), endpoint_type{boost::asio::ip::tcp::v4(), 9999});

    listener.start();

    io.run_for(std::chrono::seconds(10));
}
