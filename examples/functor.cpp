#include <string>
#include <functional>
#include <udho/router.h>
#include <udho/server.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <udho/defs.h>
#include <udho/visitor.h>

struct simple{
    int add(int a, int b){
        return a+b;
    }
    int operator()(udho::defs::request_type req, int a, int b){
        return a+b;
    }
    std::string operator()(udho::contexts::stateless ctx){
        return "Hello World\n";
    }
};

void hello(const boost::system::error_code& e){
    std::cout << "hello handler " << e << std::endl;
}

struct delayed{
    boost::asio::deadline_timer _timer;
    bool _started;
    
    delayed(boost::asio::io_service& io): _timer(io), _started(false){}
    void triggered(udho::contexts::stateless ctx, const boost::system::error_code& e){
        std::string content = "Hello Mars\n";
        ctx.respond(content, "text/plain");
        std::cout << "timer error: " << e << std::endl;
        _started = false;
    }
    void operator()(udho::contexts::stateless ctx){
        if(!_started){
            _started = true;
            _timer.expires_from_now(boost::posix_time::seconds(15));
            _timer.async_wait(boost::bind(&delayed::triggered, this, ctx, boost::asio::placeholders::error));
            std::cout << "timer setup" << std::endl;
        }
    }
    
};

int main(){
    boost::asio::io_service io;
    
    simple s;
    boost::function<int (udho::defs::request_type, int, int)> add(s);
    boost::function<std::string (udho::contexts::stateless)> hello(s);
    
    delayed d1(io);
    boost::function<void (udho::contexts::stateless)> delayed_5(boost::ref(d1));

    udho::servers::ostreamed::stateless server(io, std::cout);
    server[udho::configs::server::template_root] = TMPL_PATH;
    server[udho::configs::server::document_root] = WWW_PATH;
    
    auto router = udho::router()
        | (udho::get(add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::get(delayed_5).deferred() = "^/delayed")
        | (udho::get(hello).plain() = "^/hello$");
         
    router /= udho::visitors::print<udho::visitors::visitable::both, std::ostream>(std::cout);
    
    server.serve(router, 9198);
    
    io.run();

    
    return 0;
}
