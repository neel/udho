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

struct A1: udho::activity<A1, A1SData, A1FData>{
    typedef udho::activity<A1, A1SData, A1FData> base;
    
    boost::asio::deadline_timer _timer;
    
    template <typename CollectorT>
    A1(CollectorT c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(5));
        _timer.async_wait(boost::bind(&A1::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& e){
        std::cout << "A1 begin" << std::endl;
        A1SData data;
        data.value = 42;
        success(data);
//         A1FData data;
//         data.reason = 42;
//         failure(data);
        std::cout << "A1 end" << std::endl;
    }
};

struct A2SData{
    int value;
};

struct A2FData{
    int reason;
};

// struct A2: udho::activity<A2, A2SData, A2FData>{
//     typedef udho::activity<A2, A2SData, A2FData> base;
//     
//     boost::asio::deadline_timer _timer;
//     udho::accessor<A1> _accessor;
//     
//     template <typename CollectorT>
//     A2(CollectorT c, boost::asio::io_context& io): base(c), _timer(io), _accessor(c){}
//     
//     void operator()(){
//         _timer.expires_from_now(boost::posix_time::seconds(10));
//         _timer.async_wait(boost::bind(&A2::finished, self(), boost::asio::placeholders::error));
//     }
//     
//     void finished(const boost::system::error_code& err){
//         std::cout << "A2 begin" << std::endl;
//         if(!err && !_accessor.failed<A1>()){
//             A1SData pre = _accessor.success<A1>();
//             A2SData data;
//             data.value = pre.value + 2;
//             success(data);
//         }
//         std::cout << "A2 end" << std::endl;
//     }
// };

struct A2i: udho::activity<A2i, A2SData, A2FData>{
    typedef udho::activity<A2i, A2SData, A2FData> base;
    
    int prevalue;
    boost::asio::deadline_timer _timer;
    
    template <typename CollectorT>
    A2i(CollectorT c, boost::asio::io_context& io, int p): base(c), prevalue(p), _timer(io){}
    
    template <typename CollectorT>
    A2i(CollectorT c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(10));
        _timer.async_wait(boost::bind(&A2i::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& err){
        std::cout << "A2i begin" << std::endl;
        if(!err){
            A2SData data;
            data.value = prevalue + 2;
            success(data);
        }
        std::cout << "A2i end" << std::endl;
    }
};

struct A3SData{
    int value;
};

struct A3FData{
    int reason;
};


// struct A3: udho::activity<A3, A3SData, A3FData>{
//     typedef udho::activity<A3, A3SData, A3FData> base;
//     
//     boost::asio::deadline_timer _timer;
//     udho::accessor<A1> _accessor;
//     
//     template <typename CollectorT>
//     A3(CollectorT c, boost::asio::io_context& io): base(c), _timer(io), _accessor(c){}
//     
//     void operator()(){
//         _timer.expires_from_now(boost::posix_time::seconds(5));
//         _timer.async_wait(boost::bind(&A3::finished, self(), boost::asio::placeholders::error));
//     }
//     
//     void finished(const boost::system::error_code& err){
//         std::cout << "A3 begin" << std::endl;
//         if(!err && !_accessor.failed<A1>()){
//             A1SData pre = _accessor.success<A1>();
//             A3SData data;
//             data.value = pre.value * 2;
//             success(data);
//         }
//         std::cout << "A3 end" << std::endl;
//     }
// };

struct A3i: udho::activity<A3i, A3SData, A3FData>{
    typedef udho::activity<A3i, A3SData, A3FData> base;
    
    boost::asio::deadline_timer _timer;
    int prevalue;
    
    template <typename CollectorT>
    A3i(CollectorT c, boost::asio::io_context& io): base(c), _timer(io){}
    
    void operator()(){
        _timer.expires_from_now(boost::posix_time::seconds(5));
        _timer.async_wait(boost::bind(&A3i::finished, self(), boost::asio::placeholders::error));
    }
    
    void finished(const boost::system::error_code& err){
        std::cout << "A3i begin" << std::endl;
        if(!err){
            A3SData data;
            data.value = prevalue * 2;
            success(data);
        }
        std::cout << "A3i end" << std::endl;
    }
};

void planet(udho::contexts::stateless ctx, std::string name){
    auto& io = ctx.io();
    
    auto data = udho::collect<A1, A2i, A3i>(ctx, "A");
    
    auto t1 = udho::perform<A1>::with(data, io).required(false);
    auto t2 = udho::perform<A2i>::require<A1>::with(data, io).after(t1).prepare([data](A2i& a2i){
        udho::accessor<A1> a1_access(data);
        A1SData pre = a1_access.success<A1>();
        a2i.prevalue = pre.value;
        std::cout << "preparing A2i" << std::endl;
    });
    auto t3 = udho::perform<A3i>::require<A1>::with(data, io).after(t1).prepare([data](A3i& a3i){
        udho::accessor<A1> a1_access(data);
        A1SData pre = a1_access.success<A1>();
        a3i.prevalue = pre.value;
        std::cout << "preparing A3i" << std::endl;
    });
        
    udho::require<A2i, A3i>::with(data).exec([ctx, name](const udho::accessor<A1, A2i, A3i>& d) mutable{
        std::cout << "Final begin" << std::endl;
        
        int sum = 0;
        
        if(!d.failed<A2i>()){
            A2SData pre = d.success<A2i>();
            sum += pre.value;
            std::cout << "A2i " << pre.value << std::endl;
        }
        
        if(!d.failed<A3i>()){
            A3SData pre = d.success<A3i>();
            sum += pre.value;
            std::cout << "A3i " << pre.value << std::endl;
        }
        
        ctx.respond(boost::lexical_cast<std::string>(sum), "text/plain");
        
        std::cout << "Final end" << std::endl;
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


