#include <string>
#include <memory>
#include <iostream>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/contexts.h>
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
        std::cout << e.message() << std::endl;
        std::cout << "A1 begin" << std::endl;
        A1SData data;
        data.value = 42;
        success(data);
        std::cout << "A1 end" << std::endl;
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
        std::cout << "A2 begin" << std::endl;
        if(!err && !_accessor.failed<A1>()){
            A1SData pre = _accessor.success<A1>();
            A2SData data;
            data.value = pre.value + 2;
            success(data);
        }
        std::cout << "A2 end" << std::endl;
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
        std::cout << "A3 begin" << std::endl;
        if(!err && !_accessor.failed<A1>()){
            A1SData pre = _accessor.success<A1>();
            A3SData data;
            data.value = pre.value * 2;
            success(data);
        }
        std::cout << "A3 end" << std::endl;
    }
};

void planet(udho::contexts::stateless ctx, std::string name){
    auto& io = ctx.aux()._io;
    
    auto data = udho::activities::collect<A1, A2, A3>(ctx, "A");
    
    auto t1 = udho::activities::perform<A1>::with(data, io);
    auto t2 = udho::activities::perform<A2>::require<A1>::with(data, io).after(t1);
    auto t3 = udho::activities::perform<A3>::require<A1>::with(data, io).after(t1);
        
    udho::activities::require<A2, A3>::with(data).exec([ctx, name](const udho::activities::accessor<A1, A2, A3>& d) mutable{
        std::cout << "A4 begin" << std::endl;
        
        int sum = 0;
        
        if(!d.failed<A2>()){
            A2SData pre = d.success<A2>();
            sum += pre.value;
            std::cout << "A2 " << pre.value << std::endl;
        }
        
        if(!d.failed<A3>()){
            A3SData pre = d.success<A3>();
            sum += pre.value;
            std::cout << "A3 " << pre.value << std::endl;
        }
        
        ctx.respond(boost::lexical_cast<std::string>(sum), "text/plain");
        
        std::cout << "A4 end" << std::endl;
    }).after(t2).after(t3);
    
    t1();
}

int main(){
    boost::asio::io_service io;
    udho::servers::ostreamed::stateless server(io, std::cout);

    auto urls = udho::router() | "/planet/(\\w+)"  >> udho::get(&planet).deferred();

    server.serve(urls, 9198);

    io.run();
    return 0;
}


