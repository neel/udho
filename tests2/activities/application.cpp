#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <udho/router.h>
#include <boost/lexical_cast.hpp>
#include <udho/server.h>
#include <iostream>
#include <udho/configuration.h>
#include <udho/activities.h>
#define CATCH_CONFIG_MAIN
#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif

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

void unprepared(udho::contexts::stateless ctx){
    auto& io = ctx.aux()._io;
    
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
        
        ctx.respond(boost::lexical_cast<std::string>(sum), "text/plain");
    }).after(t2).after(t3);
    
    t1();
}

void prepared(udho::contexts::stateless ctx){
    auto& io = ctx.aux()._io;
    
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
        
        ctx.respond(boost::lexical_cast<std::string>(sum), "text/plain");
    }).after(t2).after(t3);
    
    t1();
}

void unprepared_a1_fail(udho::contexts::stateless ctx){
    auto& io = ctx.aux()._io;
    
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
        
        ctx.respond(boost::lexical_cast<std::string>(pre.reason), "text/plain");
    }).after(t2).after(t3).force();
    
    t1();
}

TEST_CASE( "activity application", "[activities]" ) {

    SECTION("unprepared_subtasks"){
        boost::asio::io_service io;
        udho::servers::quiet::stateless server(io);
        auto urls = udho::router()
            | (udho::get(&unprepared).deferred() = "^/unprepared");
        server.serve(urls, 9198);
        
        udho::servers::quiet::stateless::request_type req;
        udho::servers::quiet::stateless::attachment_type attachment(io);
        udho::contexts::stateless ctx(attachment.aux(), req, attachment);
        
        ctx.client().get("http://localhost:9198/unprepared")
            .done([ctx, &io](boost::beast::http::status status, const std::string& body) mutable {
                CHECK(status == boost::beast::http::status::ok);
                CHECK(body == "128");
                io.stop();
            });
        
        io.run();
    }

    SECTION("prepared_subtasks"){
        boost::asio::io_service io;
        udho::servers::quiet::stateless server(io);
        auto urls = udho::router()
            | (udho::get(&prepared).deferred() = "^/prepared");
        server.serve(urls, 9198);
        
        udho::servers::quiet::stateless::request_type req;
        udho::servers::quiet::stateless::attachment_type attachment(io);
        udho::contexts::stateless ctx(attachment.aux(), req, attachment);
        
        ctx.client().get("http://localhost:9198/prepared")
            .done([ctx, &io](boost::beast::http::status status, const std::string& body) mutable {
                CHECK(status == boost::beast::http::status::ok);
                CHECK(body == "128");
                io.stop();
            });
        
        io.run();
    }

    SECTION("unprepared_subtasks_a1_fail"){
        boost::asio::io_service io;
        udho::servers::quiet::stateless server(io);
        auto urls = udho::router()
            | (udho::get(&unprepared_a1_fail).deferred() = "^/unprepared_a1_fail");
        server.serve(urls, 9198);
        
        udho::servers::quiet::stateless::request_type req;
        udho::servers::quiet::stateless::attachment_type attachment(io);
        udho::contexts::stateless ctx(attachment.aux(), req, attachment);
        
        ctx.client().get("http://localhost:9198/unprepared_a1_fail")
            .done([ctx, &io](boost::beast::http::status status, const std::string& body) mutable {
                CHECK(status == boost::beast::http::status::ok);
                CHECK(body == "100");
                io.stop();
            });
        
        io.run();
    }

}
