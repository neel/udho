# Udho (উধো)
udho is a tiny http library based on [`Boost.Beast`](https://www.boost.org/doc/libs/1_71_0/libs/beast/doc/html/index.html). 

`Boost.Beast` is based on [`Boost.Asio`](https://www.boost.org/doc/libs/1_71_0/doc/html/boost_asio.html) which provides low level asynchronous I/O.  udho was originally created to add HTTP functionality to an existing application that is using `Boost.Asio`. The library does not depend on any JSON library, however any JSON library[^1] that `parse` and `dump` `std::string` can be used  along with it. 

[^1]: udho has been used with [`nlohmann`](https://github.com/nlohmann/json) json library and [`inja`](https://github.com/pantor/inja) template library 

 # Dependencies:
* Boost.Filesystem
* Boost.Regex
* ICU::uc
* Threads
  
# Example

```cpp
#include <string>
#include <udho/router.h>
#include <boost/asio.hpp>

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
