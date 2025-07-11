#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <boost/lexical_cast.hpp>
#include <udho/activities.h>
#define CATCH_CONFIG_MAIN
#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <curl/curl.h>
#include <udho/net/listener.h>
#include <udho/net/connection.h>
#include <udho/net/protocols/protocols.h>
#include <udho/net/common.h>
#include <udho/net/server.h>
#include <curl/curl.h>
#include <udho/net/artifacts.h>
#include <udho/url/url.h>
#include <udho/session/storage/fs.h>

using socket_type     = udho::net::types::socket;
using http_protocol   = udho::net::protocols::http<socket_type>;
using scgi_protocol   = udho::net::protocols::scgi<socket_type>;
using http_connection = udho::net::connection<http_protocol>;
using scgi_connection = udho::net::connection<scgi_protocol>;
using http_listener   = udho::net::listener<http_connection>;
using scgi_listener   = udho::net::listener<scgi_connection>;

static size_t curl_writef(void *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

struct http_results{
    long code;
    std::string   body;
    std::map<std::string, std::string> headers;
};

http_results curl_fetch(CURL* curl, const std::string method, const std::string& url){
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "http");
    struct curl_slist *headers = NULL;
    std::string response_headers;
    std::string response_body;
    std::map<std::string, std::string> headers_map;
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   curl_writef);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,  curl_writef);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA,      &response_headers);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,       &response_body);
    res = curl_easy_perform(curl);
    long response_code = 0;
    if(res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        std::vector<std::string> header_lines;
        boost::algorithm::split(header_lines, response_headers, boost::is_any_of("\r\n"));
        for(const std::string& line: header_lines){
            std::vector<std::string> header_parts;
            boost::algorithm::split(header_parts, line, boost::is_any_of(":"));
            if(header_parts.size() >= 2){
                headers_map.insert(std::make_pair(boost::algorithm::trim_copy(header_parts[0]), boost::algorithm::trim_copy(header_parts[1])));
            }
        }
    }
    return http_results{response_code, response_body, headers_map};
}

#define SMALL_TIMEOUT 1
#define LARGE_TIMEOUT 2

struct A1SData{
    int value;
    
    A1SData(): value(0){}
};

struct A1FData{
    int reason;
};

struct A1: udho::activity<A1, A1SData, A1FData>{
    typedef udho::activity<A1, A1SData, A1FData> base;
    
    boost::asio::deadline_timer _timer;
    bool _succeed;
    
    template <typename CollectorT>
    A1(CollectorT c, boost::asio::io_context& io, bool succeed = true): base(c), _timer(io), _succeed(succeed){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(SMALL_TIMEOUT));
        _timer.async_wait(boost::bind(&A1::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& e){
        if(_succeed){
            A1SData data;
            data.value = 42;
            success(data);
        }else{
            A1FData data;
            data.reason = 100;
            failure(data);
        }
    }
};

struct A2SData{
    int value;
};

struct A2FData{
    int reason;
};

struct A2: udho::activity<A2, A2SData, A2FData>{
    typedef udho::activity<A2, A2SData, A2FData> base;
    
    boost::asio::deadline_timer _timer;
    udho::accessor<A1> _accessor;
    
    template <typename CollectorT>
    A2(CollectorT c, boost::asio::io_context& io): base(c), _timer(io), _accessor(c){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(LARGE_TIMEOUT));
        _timer.async_wait(boost::bind(&A2::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& err){
        if(!err && !_accessor.failed<A1>()){
            A1SData pre = _accessor.success<A1>();
            A2SData data;
            data.value = pre.value + 2;
            success(data);
        }
    }
};

struct A2i: udho::activity<A2i, A2SData, A2FData>{
    typedef udho::activity<A2i, A2SData, A2FData> base;
    
    int prevalue;
    boost::asio::deadline_timer _timer;
    
    template <typename CollectorT>
    A2i(CollectorT c, boost::asio::io_context& io, int p): base(c), prevalue(p), _timer(io){}
    
    template <typename CollectorT>
    A2i(CollectorT c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(LARGE_TIMEOUT));
        _timer.async_wait(boost::bind(&A2i::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& err){
        if(!err){
            A2SData data;
            data.value = prevalue + 2;
            success(data);
        }
    }
};

struct A3SData{
    int value;
};

struct A3FData{
    int reason;
};


struct A3: udho::activity<A3, A3SData, A3FData>{
    typedef udho::activity<A3, A3SData, A3FData> base;
    
    boost::asio::deadline_timer _timer;
    udho::accessor<A1> _accessor;
    
    template <typename CollectorT>
    A3(CollectorT c, boost::asio::io_context& io): base(c), _timer(io), _accessor(c){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(SMALL_TIMEOUT));
        _timer.async_wait(boost::bind(&A3::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& err){
        if(!err && !_accessor.failed<A1>()){
            A1SData pre = _accessor.success<A1>();
            A3SData data;
            data.value = pre.value * 2;
            success(data);
        }
    }
};

struct A3i: udho::activity<A3i, A3SData, A3FData>{
    typedef udho::activity<A3i, A3SData, A3FData> base;
    
    boost::asio::deadline_timer _timer;
    int prevalue;
    
    template <typename CollectorT>
    A3i(CollectorT c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(SMALL_TIMEOUT));
        _timer.async_wait(boost::bind(&A3i::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& err){
        if(!err){
            A3SData data;
            data.value = prevalue * 2;
            success(data);
        }
    }
};

void unprepared(udho::net::stream ctx){
    auto& io = ctx.io();
    
    auto data = udho::collect<A1, A2, A3>(ctx);
    
    auto t1 = udho::perform<A1>::with(data, io);
    auto t2 = udho::perform<A2>::require<A1>::with(data, io).after(t1);
    auto t3 = udho::perform<A3>::require<A1>::with(data, io).after(t1);
        
    udho::require<A2, A3>::with(data).exec([ctx](const udho::accessor<A1, A2, A3>& d) mutable{
        CHECK(d.completed<A1>());
        CHECK(d.completed<A2>());
        CHECK(d.completed<A3>());
        CHECK(!d.failed<A1>());
        CHECK(!d.failed<A2>());
        CHECK(!d.failed<A3>());
        
        int sum = 0;
        
        if(!d.failed<A2>()){
            A2SData pre = d.success<A2>();
            sum += pre.value;
        }
        
        if(!d.failed<A3>()){
            A3SData pre = d.success<A3>();
            sum += pre.value;
        }
        
        ctx << sum;
        ctx.response().set(boost::beast::http::field::content_type, "text/plain");
        ctx.finish();
    }).after(t2).after(t3);
    
    t1();
}

void prepared(udho::net::stream ctx){
    auto& io = ctx.io();
    
    auto data = udho::collect<A1, A2i, A3i>(ctx);
    
    auto t1 = udho::perform<A1>::with(data, io);
    auto t2 = udho::perform<A2i>::require<A1>::with(data, io).after(t1).prepare([data](A2i& a2i){
        udho::accessor<A1> a1_access(data);
        A1SData pre = a1_access.success<A1>();
        a2i.prevalue = pre.value;
    });
    auto t3 = udho::perform<A3i>::require<A1>::with(data, io).after(t1).prepare([data](A3i& a3i){
        udho::accessor<A1> a1_access(data);
        A1SData pre = a1_access.success<A1>();
        a3i.prevalue = pre.value;
    });
        
    udho::require<A2i, A3i>::with(data).exec([ctx](const udho::accessor<A1, A2i, A3i>& d) mutable{
        CHECK(d.completed<A1>());
        CHECK(d.completed<A2i>());
        CHECK(d.completed<A3i>());
        CHECK(!d.failed<A1>());
        CHECK(!d.failed<A2i>());
        CHECK(!d.failed<A3i>());
        
        int sum = 0;
        
        if(!d.failed<A2i>()){
            A2SData pre = d.success<A2i>();
            sum += pre.value;
        }
        
        if(!d.failed<A3i>()){
            A3SData pre = d.success<A3i>();
            sum += pre.value;
        }
        
        ctx << sum;
        ctx.response().set(boost::beast::http::field::content_type, "text/plain");
        ctx.finish();
    }).after(t2).after(t3);
    
    t1();
}

void unprepared_a1_fail(udho::net::stream ctx){
    auto& io = ctx.io();
    
    auto data = udho::collect<A1, A2, A3>(ctx);
    
    auto t1 = udho::perform<A1>::with(data, io, false);
    auto t2 = udho::perform<A2>::require<A1>::with(data, io).after(t1);
    auto t3 = udho::perform<A3>::require<A1>::with(data, io).after(t1);
        
    udho::require<A2, A3>::with(data).exec([ctx](const udho::accessor<A1, A2, A3>& d) mutable{
        CHECK(d.completed<A1>());
        CHECK(d.failed<A1>());
        CHECK(!d.completed<A2>());
        CHECK(!d.completed<A3>());
        
        A1FData pre = d.failure<A1>();
        
        ctx << pre.reason;
        ctx.response().set(boost::beast::http::field::content_type, "text/plain");
        ctx.finish();
    }).after(t2).after(t3).force();
    
    t1();
}

TEST_CASE( "activity application", "[activities]" ) {
    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<udho::net::context<udho::view::data::bridges::lua>>{});

    udho::view::resources::store<udho::view::data::bridges::lua> resources{lua};
    udho::pages::system::setup(resources);
    resources.assets().base("assets");
    resources.lock();


    udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{resources};
    using namespace udho::hazo::string::literals;
    auto router = udho::url::router(
        udho::url::mount_point{"root"_h, "/",
            udho::url::slot("unprepared"_h,  &unprepared) << udho::url::fixed(udho::url::verb::get, "/unprepared", "/unprepared") |
            udho::url::slot("prepared"_h,    &prepared)   << udho::url::fixed(udho::url::verb::get, "/prepared",   "/prepared")   |
            udho::url::slot("unprepared_a1_fail"_h,  &unprepared_a1_fail) << udho::url::fixed(udho::url::verb::get, "/unprepared_a1_fail", "/unprepared_a1_fail")
        }
    );

    boost::asio::io_context service;
    udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy> sessions{udho::session::storage::fs{}};

    auto server     = udho::net::server<http_listener>(service, 9000);
    auto artifacts  = udho::net::artifacts{router, resources};

    server.run(artifacts);

    std::thread thread([&]{
        service.run();
    });

    CURL* curl;
    curl = curl_easy_init();
    CHECK(curl != 0x0);


    SECTION("unprepared_subtasks"){
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/unprepared");
        CHECK(results.code == 200);
        CHECK(results.body == "128");
    }

    SECTION("prepared_subtasks"){
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/prepared");
        CHECK(results.code == 200);
        CHECK(results.body == "128");
    }

    SECTION("unprepared_subtasks_a1_fail"){
        http_results results = curl_fetch(curl, "GET", "http://localhost:9000/unprepared_a1_fail");
        CHECK(results.code == 200);
        CHECK(results.body == "100");
    }

    curl_easy_cleanup(curl);
    server.stop();
    thread.join();

}
