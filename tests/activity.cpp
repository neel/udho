#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "udho Unit Test (udho::activity)"
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <udho/router.h>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <udho/server.h>
#include <iostream>
#include <udho/configuration.h>
#include <udho/activities.h>

struct A1SData{
    int value;
};

struct A1FData{
    int reason;
};

struct A1: udho::activities::activity<A1, A1SData, A1FData>{
    typedef udho::activities::activity<A1, A1SData, A1FData> base;
    
    boost::asio::deadline_timer _timer;
    
    template <typename CollectorT>
    A1(CollectorT c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(5));
        _timer.async_wait(boost::bind(&A1::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& e){
        A1SData data;
        data.value = 42;
        success(data);
    }
};

struct A2SData{
    int value;
};

struct A2FData{
    int reason;
};

struct A2: udho::activities::activity<A2, A2SData, A2FData>{
    typedef udho::activities::activity<A2, A2SData, A2FData> base;
    
    boost::asio::deadline_timer _timer;
    udho::activities::accessor<A1> _accessor;
    
    template <typename CollectorT>
    A2(CollectorT c, boost::asio::io_context& io): base(c), _timer(io), _accessor(c){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(10));
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

struct A3SData{
    int value;
};

struct A3FData{
    int reason;
};


struct A3: udho::activities::activity<A3, A3SData, A3FData>{
    typedef udho::activities::activity<A3, A3SData, A3FData> base;
    
    boost::asio::deadline_timer _timer;
    udho::activities::accessor<A1> _accessor;
    
    template <typename CollectorT>
    A3(CollectorT c, boost::asio::io_context& io): base(c), _timer(io), _accessor(c){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(5));
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

void hello(udho::contexts::stateless ctx){
    auto& io = ctx.aux()._io;
    
    auto data = udho::activities::collect<A1, A2, A3>(ctx, "A");
    
    auto t1 = udho::activities::perform<A1>::with(data, io);
    auto t2 = udho::activities::perform<A2>::require<A1>::with(data, io).after(t1);
    auto t3 = udho::activities::perform<A3>::require<A1>::with(data, io).after(t1);
        
    udho::activities::require<A2, A3>::with(data).exec([ctx](const udho::activities::accessor<A1, A2, A3>& d) mutable{
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

BOOST_AUTO_TEST_SUITE(activity)

BOOST_AUTO_TEST_CASE(success){
    boost::asio::io_service io;
    udho::servers::quiet::stateless server(io);
    auto urls = udho::router()
        | (udho::get(&hello).deferred() = "^/hello");
    server.serve(urls, 9198);
    
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);
    
    ctx.client().get("http://localhost:9198/hello")
        .done([ctx, &io](boost::beast::http::status status, const std::string& body) mutable {
            BOOST_CHECK(status == boost::beast::http::status::ok);
            BOOST_CHECK(body == "128");
            io.stop();
        });
    
    io.run();
}

BOOST_AUTO_TEST_SUITE_END()
