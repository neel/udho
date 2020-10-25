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

struct A4: udho::activities::aggregated<A4, A3, A2, A1>{
    typedef udho::activities::aggregated<A4, A3, A2, A1> base;
    
    udho::contexts::stateless _ctx;
    std::string _planet;
    
    A4(const udho::contexts::stateless& ctx, std::string planet): base(ctx, "A"), _ctx(ctx), _planet(planet){}
    
    void operator()(){
        if(data()->template exists<udho::activities::result_data<A3SData, A3FData>>()){
            udho::activities::result_data<A3SData, A3FData> d;
            *data() >> d;
            std::cout << "A3 " << d.success_data().value << std::endl;
        }
        
        if(data()->template exists<udho::activities::result_data<A2SData, A2FData>>()){
            udho::activities::result_data<A2SData, A2FData> d;
            *data() >> d;
            std::cout << "A2 " << d.success_data().value << std::endl;
        }
        
        if(data()->template exists<udho::activities::result_data<A1SData, A1FData>>()){
            udho::activities::result_data<A1SData, A1FData> d;
            *data() >> d;
            std::cout << "A1 " << d.success_data().value << std::endl;
        }
        
        std::cout << "A4 begin" << std::endl;
        _ctx.respond(_planet, "text/plain");
        std::cout << "A4 end" << std::endl;
    }
};

void planet(udho::contexts::stateless ctx, std::string name){
    auto& io = ctx.aux()._io;
    
    auto t4 = udho::activities::task<A4, A2, A3>::with(ctx, name);
    auto t2 = udho::activities::task<A2, A1>::with(t4.activity()->data(), io).done(t4);
    auto t3 = udho::activities::task<A3, A1>::with(t4.activity()->data(), io).done(t4);
    auto t1 = udho::activities::task<A1>::with(t4.activity()->data(), io).done(t2).done(t3);
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


