# Udho (উধো)
udho is a tiny http library based on [`Boost.Beast`](https://www.boost.org/doc/libs/1_71_0/libs/beast/doc/html/index.html). 

`Boost.Beast` is based on [`Boost.Asio`](https://www.boost.org/doc/libs/1_71_0/doc/html/boost_asio.html) which provides low level asynchronous I/O.  udho was originally created to add HTTP functionality to an existing application that is using `Boost.Asio`. The library does not depend on any JSON library, however any JSON library[^1] that `parse` and `dump` `std::string` can be used  along with it. 

[![pipeline status develop](https://gitlab.com/neel.basu/udho/badges/develop/pipeline.svg)](https://gitlab.com/neel.basu/udho/commits/develop) 
[![pipeline status master](https://gitlab.com/neel.basu/udho/badges/master/pipeline.svg)](https://gitlab.com/neel.basu/udho/commits/master) 
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/20093f1597cd490ba923fc5401ada672)](https://www.codacy.com/manual/neel.basu.z/udho?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=neel/udho&amp;utm_campaign=Badge_Grade)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/neel/udho.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/neel/udho/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/neel/udho.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/neel/udho/context:cpp)


[^1]: udho has been used with [`nlohmann`](https://github.com/nlohmann/json) json library and [`inja`](https://github.com/pantor/inja) template library 

 # Dependencies:
* Boost.Filesystem
* Boost.Regex
* ICU::uc
* Threads
  
# Example (Simple Function)

```cpp
#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>

// udho::request_type is typedef of boost::beast::http::request<boost::beast::http::string_body>

std::string hello(udho::request_type req){
    return "Hello World";
}

std::string data(udho::request_type req){
    return "{id: 2, name: 'udho'}";
}

int add(udho::request_type req, int a, int b){
    return a + b;
}

int main(){
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    boost::asio::io_service io;

    auto router = udho::router()
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$");
    router.listen(io, 9198, doc_root);
        
    io.run();
    
    return 0;
}

```

# Example (Function Object)

```cpp
#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>

struct simple{
    int operator()(udho::request_type req, int a, int b){
        return a+b;
    }
    std::string operator()(udho::request_type req){
        return "Hello World";
    }
};

int main(){
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    boost::asio::io_service io;
    
    simple s;
    boost::function<int (udho::request_type, int, int)> add(s);
    boost::function<std::string (udho::request_type)> hello(s);

    auto router = udho::router()
        | (udho::get(add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::get(hello).plain() = "^/hello$");
    router.listen(io, 9198, doc_root);
        
    io.run();
    
    return 0;
}

```

# Example (Application)

```cpp
#include <string>
#include <functional>
#include <udho/router.h>
#include <boost/asio.hpp>
#include <udho/application.h>

std::string hello(udho::request_type req){
    return "Hello World";
}

std::string data(udho::request_type req){
    return "{id: 2, name: 'udho'}";
}

int add(udho::request_type req, int a, int b){
    return a + b;
}

struct my_app: public udho::application<my_app>{
    typedef udho::application<my_app> base;
    
    my_app();
    int add(udho::request_type req, int a, int b);
    int mul(udho::request_type req, int a, int b);
    
    template <typename RouterT>
    auto route(RouterT& router){
        return router | (get(&my_app::add).plain() = "^/add/(\\d+)/(\\d+)$")
                      | (get(&my_app::mul).plain() = "^/mul/(\\d+)/(\\d+)$");
    }
};

my_app::my_app(): base("my_app"){}
int my_app::add(udho::request_type req, int a, int b){
    return a + b;
}
int my_app::mul(udho::request_type req, int a, int b){
    return a * b;
}

int main(){
    std::string doc_root("/home/neel/Projects/udho"); // path to static content
    boost::asio::io_service io;

    auto router = udho::router()
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$")
        | (udho::app<my_app>()       = "^/my_app");
    router.listen(io, 9198, doc_root);
          
    io.run();
    
    return 0;
}

```
