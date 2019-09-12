# Udho (উধো)
udho is a tiny http library based on [`Boost.Beast`](https://www.boost.org/doc/libs/1_71_0/libs/beast/doc/html/index.html). 

`Boost.Beast` is based on [`Boost.Asio`](https://www.boost.org/doc/libs/1_71_0/doc/html/boost_asio.html) which provides low level asynchronous I/O.  udho was originally created to add HTTP functionality to an existing application that is using `Boost.Asio`. The library does not depend on any JSON library, however any JSON library[^1] that `parse` and `dump` `std::string` can be used  along with it. 

[^1]: udho has been used with [`nlohmann`](https://github.com/nlohmann/json) json library and [`inja`](https://github.com/pantor/inja) template library 

 # Dependencies:
* Boost.Filesystem
* Boost.Regex
* Threads
  
# Example

```c++
std::string doc_root(WWW_PATH); // path to static content

int threads = 2;
boost::asio::io_service io{threads};

auto router = udho::router()
    | (udho::post(&input).json()           = "^/input$")
    | (udho::get(&students_list).json()    = "^/students/highlights$")
    | (udho::get(&student, conn).html()    = "^/student/(\\w+)$"); // conn might be a database connection handle injected to the function as the first parameter
router.listen(io, 9198, doc_root);

std::vector<std::thread> v;
v.reserve(threads - 1);
for(auto i = threads - 1; i > 0; --i){
    v.emplace_back([&io]{
        io.run();
    });
}
io.run();
```
