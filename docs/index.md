# Udho (উধো)
udho is a minimalistic http library based on [`Boost.Beast`](https://www.boost.org/doc/libs/1_71_0/libs/beast/doc/html/index.html). 

[Code](https://gitlab.com/neel.basu/udho) |
[Build](build) |
[Quickstart](quickstart) |
[Router](router) |
[Server](server) |
[Logging](logging) |
[Form](form) |
[Session](session) |
[Contribute](contributing) |
[Issues](https://gitlab.com/neel.basu/udho/issues)


[![pipeline status develop](https://gitlab.com/neel.basu/udho/badges/develop/pipeline.svg)](https://gitlab.com/neel.basu/udho/commits/develop) 
[![pipeline status master](https://gitlab.com/neel.basu/udho/badges/master/pipeline.svg)](https://gitlab.com/neel.basu/udho/commits/master) 
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/20093f1597cd490ba923fc5401ada672)](https://www.codacy.com/manual/neel.basu.z/udho?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=neel/udho&amp;utm_campaign=Badge_Grade)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/neel/udho.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/neel/udho/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/neel/udho.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/neel/udho/context:cpp)

```cpp
std::string world(udho::contexts::stateless ctx){
    return "{'planet': 'Earth'}";
}
std::string planet(udho::contexts::stateless ctx, std::string name){
    return "Hello "+name;
}
int main(){
    boost::asio::io_service io;
    udho::servers::ostreamed::stateless server(io, std::cout);

    auto urls = udho::router() | "/world"          >> udho::get(&world).json() 
                               | "/planet/(\\w+)"  >> udho::get(&planet).plain();

    server.serve(urls, 9198, "/path/to/static/files");

    io.run();
    return 0;
}
```

The philosophy is simple. `udho::router` comprises of mapping between url patterns (as regex) and the corresponding callables (function pointers, function objects). Multiple such mappings are combined at compile time using pipe (`|`) operator. The url mappings are passed to the server along with the listening port and document root. The request methods (GET, POST, HEAD, PUT, etc..) and the response content type are attached with the callable on compile time. Whenever an HTTP request appears to the server its path is matched against the url patterns and the matching callable is called. The values captured from the url patterns are converted (using `boost::lexical_cast`) and passed as arguments to the callables. 

The server can be logging or quiet. The example above uses an ostreamed logger that logs on `std::cout`. A server can be `stateless` or `stateful` depending on the choice of `session`. If the server uses HTTP session then it has to be stateful, and the states comprising the session has to be declared at the compile time. The above example uses a stateless server (no HTTP session cookie). Callables in a stateful server can be stateless. 
 

# Dependencies:
udho depend on boost. As boost-beast is only available on boost >= 1.66, udho requires a boost version at least 1.66. udho may optionally use ICU library for unicode regex functionality. In that case ICU library may be required.

* boost > 1.66
* icu [optional]

# Features:

* regular expression based url routing to callables (functions / function objects)
* compile time binding of routing rule with callables
* compile time travarsable url router
* response mime type specification in routing rule
* any default constructible can be used as callable argument type that can be parsed from std::string
* any ostreamable can be used as return type of callables
* automatic type coersion for url based method calling
* throwable http error messages
* serving static content from disk document root if no rule matched
* urlencoded/multipart form parsing
* on memory session for stateful web applications (no globals) and no session for stateless applications
* strictly typed on memory session storage
* compile time pluggable & customizable logging
  
# Example 

```cpp
std::string login(udho::contexts::stateful<user> ctx){ /// < strictly typed stateful context
    const static username = "derp";
    const static password = "derp123";

    if(ctx.session().exists<user>()){
        user data;
        ctx.session() >> data;  /// < extract session data
        return "already logged in";
    }else{
        if(ctx.form().has("user") && ctx.form().has("pass")){
            std::string usr = ctx.form().field<std::string>("user"); /// < form field value from post request
            std::string psw = ctx.form().field<std::string>("pass"); /// < form field value from post request
            if(usr == username && psw == password){
                ctx.session() << user(usr); /// < put data in session
                return "successful";
            }
        }
    }
    return "failed";
}

std::string echo(udho::contexts::stateful<user> ctx, int num){
    if(ctx.session().exists<user>()){
        user data;
        ctx.session() >> data;
        return boost::format("{'name': '%1%', 'num': %2%}") % data.name % num;
    }
    return "{}";
}

int main(){
    std::string doc_root("/path/to/static/document/root");
    
    boost::asio::io_service io;
    udho::servers::ostreamed::stateful<user> server(io, std::cout);

    auto router = udho::router()
        | (udho::post(&login).plain() = "^/login$")
        | (udho::get(&echo).json()    = "^/echo/(\\d+)$");

    server.serve(router, 9198, doc_root);
        
    io.run();
    
    return 0;
}
```
