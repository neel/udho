#include <string>
#include <boost/asio.hpp>
#include <udho/router.h>
#include <udho/logging.h>
#include <udho/server.h>
#include <udho/contexts.h>
#include <iostream>
#include <udho/configuration.h>
#include <udho/activities.h>

struct A1SData{
    int value;
};

struct A1FData{
    int reason;
};

template <typename CollectorT>
struct A1: udho::activities::result<A1SData, A1FData>{
    typedef udho::activities::result<A1SData, A1FData> base;
    
    A1(CollectorT& c): base(c){}
    
    void operator()(){
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
struct A2: udho::activities::result<A2SData, A2FData>{
    typedef udho::activities::result<A2SData, A2FData> base;
    
    A2(CollectorT& c): base(c){}
    
    void operator()(){
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
struct A3: udho::activities::result<A3SData, A3FData>{
    typedef udho::activities::result<A3SData, A3FData> base;
    
    A3(CollectorT& c): base(c){}
    
    void operator()(){
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
struct A4: udho::activities::result<A4SData, A4FData>{
    typedef udho::activities::result<A4SData, A4FData> base;
    
    udho::contexts::stateless& _ctx;
    std::string _planet;
    
    A4(CollectorT& c, udho::contexts::stateless& ctx, std::string planet): base(c), _ctx(ctx), _planet(planet){}
    
    void operator()(){
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
    
    collector_type collector(ctx.aux().config(), "A");
    
    A4<collector_type> a4(collector, ctx, name);
    udho::activities::combinator<A4<collector_type>, A2<collector_type>, A3<collector_type>> c4(a4);
    A2<collector_type> a2(collector);
    a2.done(c4);
    A3<collector_type> a3(collector);
    a3.done(c4);
    udho::activities::combinator<A2<collector_type>, A1<collector_type>> c2(a2);
    udho::activities::combinator<A3<collector_type>, A1<collector_type>> c3(a3);
    A1<collector_type> a1(collector);
    a1.done(c2);
    a1.done(c3);
    
    a1();
}

int main(){
    boost::asio::io_service io;
    udho::servers::ostreamed::stateless server(io, std::cout);

    auto urls = udho::router() | "/planet/(\\w+)"  >> udho::get(&planet).deferred();

    server.serve(urls, 9198);

    io.run();
    return 0;
}


