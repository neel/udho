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

struct A1: udho::activities::result<A1SData, A1FData>, std::enable_shared_from_this<A1>{
    typedef udho::activities::result<A1SData, A1FData> base;
    typedef A1 self_type;
    
    boost::asio::deadline_timer _timer;
    
    template <typename CollectorT>
    A1(std::shared_ptr<CollectorT> c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(5));
        _timer.async_wait(boost::bind(&self_type::finished, self_type::shared_from_this(), boost::asio::placeholders::error));
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

struct A2: udho::activities::result<A2SData, A2FData>, std::enable_shared_from_this<A2>{
    typedef udho::activities::result<A2SData, A2FData> base;
    typedef A2 self_type;
    
    boost::asio::deadline_timer _timer;
    
    template <typename CollectorT>
    A2(std::shared_ptr<CollectorT> c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(10));
        _timer.async_wait(boost::bind(&self_type::finished, self_type::shared_from_this(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& e){
        std::cout << e << std::endl;
        std::cout << "A2 begin" << std::endl;
        A2SData data;
        data.value = 32;
        success(data);
        std::cout << "A2 end" << std::endl;
    }
};

struct A3SData{
    int value;
};

struct A3FData{
    int reason;
};


struct A3: udho::activities::result<A3SData, A3FData>, std::enable_shared_from_this<A3>{
    typedef udho::activities::result<A3SData, A3FData> base;
    typedef A3 self_type;
    
    boost::asio::deadline_timer _timer;
    
    template <typename CollectorT>
    A3(std::shared_ptr<CollectorT> c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(5));
        _timer.async_wait(boost::bind(&self_type::finished, self_type::shared_from_this(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& e){
        std::cout << e << std::endl;
        std::cout << "A3 begin" << std::endl;
        A3SData data;
        data.value = 64;
        success(data);
        std::cout << "A3 end" << std::endl;
    }
};

template <typename CollectorT>
struct A4: std::enable_shared_from_this<A4<CollectorT>>{
    
    udho::contexts::stateless _ctx;
    std::string _planet;
    std::shared_ptr<CollectorT> _collector;
    
    A4(std::shared_ptr<CollectorT> c, udho::contexts::stateless& ctx, std::string planet): _ctx(ctx), _planet(planet), _collector(c){}
    
    void operator()(){
        
        if(_collector->template exists<udho::activities::result_data<A3SData, A3FData>>()){
            udho::activities::result_data<A3SData, A3FData> data;
            *_collector >> data;
            std::cout << "A3 " << data.success_data().value << std::endl;
        }
        
        if(_collector->template exists<udho::activities::result_data<A2SData, A2FData>>()){
            udho::activities::result_data<A2SData, A2FData> data;
            *_collector >> data;
            std::cout << "A2 " << data.success_data().value << std::endl;
        }
        
        if(_collector->template exists<udho::activities::result_data<A1SData, A1FData>>()){
            udho::activities::result_data<A1SData, A1FData> data;
            *_collector >> data;
            std::cout << "A1 " << data.success_data().value << std::endl;
        }
        
        std::cout << "A4 begin" << std::endl;
        _ctx.respond(_planet, "text/plain");
        std::cout << "A4 end" << std::endl;
    }
};

void planet(udho::contexts::stateless ctx, std::string name){
    typedef udho::activities::collector<
        A3::result_type, 
        A2::result_type, 
        A1::result_type
    > collector_type;
    
    auto collector = std::make_shared<collector_type>(ctx.aux().config(), "A");

    auto t4 = udho::activities::task<A4<collector_type>, A2, A3>::with(collector, ctx, name);
    auto t2 = udho::activities::task<A2, A1>::with(collector, ctx.aux()._io).done(t4);
    auto t3 = udho::activities::task<A3, A1>::with(collector, ctx.aux()._io).done(t4);
    auto t1 = udho::activities::task<A1>::with(collector, ctx.aux()._io).done(t2).done(t3);
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


