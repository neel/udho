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

template <typename CollectorT>
struct A1: udho::activities::result<A1SData, A1FData>, std::enable_shared_from_this<A1<CollectorT>>{
    typedef udho::activities::result<A1SData, A1FData> base;
    typedef A1<CollectorT> self_type;
    
    boost::asio::deadline_timer _timer;
    
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

template <typename CollectorT>
struct A2: udho::activities::result<A2SData, A2FData>, std::enable_shared_from_this<A2<CollectorT>>{
    typedef udho::activities::result<A2SData, A2FData> base;
    typedef A2<CollectorT> self_type;
    
    boost::asio::deadline_timer _timer;
    
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

template <typename CollectorT>
struct A3: udho::activities::result<A3SData, A3FData>, std::enable_shared_from_this<A3<CollectorT>>{
    typedef udho::activities::result<A3SData, A3FData> base;
    typedef A3<CollectorT> self_type;
    
    boost::asio::deadline_timer _timer;
    
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

struct A4SData{
    int value;
};

struct A4FData{
    int reason;
};

template <typename CollectorT>
struct A4: udho::activities::result<A4SData, A4FData>, std::enable_shared_from_this<A4<CollectorT>>{
    typedef udho::activities::result<A4SData, A4FData> base;
    
    udho::contexts::stateless _ctx;
    std::string _planet;
    std::shared_ptr<CollectorT> _collector;
    
    A4(std::shared_ptr<CollectorT> c, udho::contexts::stateless& ctx, std::string planet): base(c), _ctx(ctx), _planet(planet), _collector(c){}
    
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
        udho::activities::result_data<A4SData, A4FData>, 
        udho::activities::result_data<A3SData, A3FData>, 
        udho::activities::result_data<A2SData, A2FData>, 
        udho::activities::result_data<A1SData, A1FData>
    > collector_type;
    
//     collector_type collector(ctx.aux().config(), "A");
    auto collector = std::make_shared<collector_type>(ctx.aux().config(), "A");
    
//     A4<collector_type> a4(collector, ctx, name);
    auto a4 = std::make_shared<A4<collector_type>>(collector, ctx, name);
//     udho::activities::combinator<A4<collector_type>, A2<collector_type>, A3<collector_type>> c4(a4);
    auto c4 = std::make_shared<udho::activities::combinator<A4<collector_type>, A2<collector_type>, A3<collector_type>>>(a4);
//     A2<collector_type> a2(collector, ctx.aux()._io);
    auto a2 = std::make_shared<A2<collector_type>>(collector, ctx.aux()._io);
    a2->done(c4);
//     A3<collector_type> a3(collector, ctx.aux()._io);
    auto a3 = std::make_shared<A3<collector_type>>(collector, ctx.aux()._io);
    a3->done(c4);
//     udho::activities::combinator<A2<collector_type>, A1<collector_type>> c2(a2);
    auto c2 = std::make_shared<udho::activities::combinator<A2<collector_type>, A1<collector_type>>>(a2);
//     udho::activities::combinator<A3<collector_type>, A1<collector_type>> c3(a3);
    auto c3 = std::make_shared<udho::activities::combinator<A3<collector_type>, A1<collector_type>>>(a3);
//     A1<collector_type> a1(collector, ctx.aux()._io);
    auto a1 = std::make_shared<A1<collector_type>>(collector, ctx.aux()._io);
    a1->done(c2);
    a1->done(c3);
    
    (*a1)();
}

int main(){
    boost::asio::io_service io;
    udho::servers::ostreamed::stateless server(io, std::cout);

    auto urls = udho::router() | "/planet/(\\w+)"  >> udho::get(&planet).deferred();

    server.serve(urls, 9198);

    io.run();
    return 0;
}


