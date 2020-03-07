#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "udho Unit Test (udho::router)"
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <udho/router.h>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <udho/server.h>
#include <iostream>

typedef udho::servers::quiet::stateless server_type;
typedef udho::contexts::stateless context_type;

template <typename F>
struct checker{
    F _check_lambda;
    
    checker(F lambda): _check_lambda(lambda){}
    
    template<bool isRequest, class Body, class Fields>
    void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg) const {
        _check_lambda(msg.body());
    }
    void operator()(boost::beast::http::response<boost::beast::http::file_body>&& res) const {
        boost::beast::http::file_body::value_type body = std::move(res.body());
        _check_lambda(boost::lexical_cast<std::string>(body.size()));
    }
};

template <typename F>
checker<F> generate_checker(F f){
    return checker<F>(f);
}

std::string hello(context_type ctx){
    return "Hello World";
}

std::string data(context_type ctx){
    return "{id: 2, name: 'udho'}";
}

int add(context_type ctx, int a, int b){
    return a + b;
}

boost::beast::http::response<boost::beast::http::file_body> file(context_type ctx){
    std::string path("/etc/passwd");
    boost::beast::error_code err;
    boost::beast::http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, err);
    auto const size = body.size();
    boost::beast::http::response<boost::beast::http::file_body> res{std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(boost::beast::http::status::ok, ctx.request().version())};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.content_length(size);
    res.keep_alive(ctx.request().keep_alive());
    return res;
}

boost::beast::http::response<boost::beast::http::string_body> page(context_type ctx){
    boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::ok, ctx.request().version()};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, "text/plain");
    res.body() = std::string("nothing");
    res.keep_alive(ctx.request().keep_alive());
    return res;
}

BOOST_AUTO_TEST_SUITE(router)

BOOST_AUTO_TEST_CASE(mapping){
    auto router = udho::router()
        | (udho::get(&page).raw() = "^/page")
        | (udho::get(&file).raw() = "^/file")
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$");
        
    context_type::request_type req;
    server_type::attachment_type attachment;
    context_type ctx(req, attachment);
    router.serve(ctx, boost::beast::http::verb::get, "/hello", generate_checker([](const std::string& res){
        BOOST_CHECK(res == "Hello World");
    }));
    router.serve(ctx, boost::beast::http::verb::get, "/data", generate_checker([](const std::string& res){
        BOOST_CHECK(res == "{id: 2, name: 'udho'}");
    }));
    router.serve(ctx, boost::beast::http::verb::get, "/add/2/3", generate_checker([](const std::string& res){
        BOOST_CHECK(res == "5");
    }));
    router.serve(ctx, boost::beast::http::verb::get, "/page", generate_checker([](const std::string& res){
        BOOST_CHECK(res == "nothing");
    }));
    router.serve(ctx, boost::beast::http::verb::get, "/file", generate_checker([](const std::string& size){
        BOOST_CHECK(boost::lexical_cast<unsigned long>(size) > 0);
    }));
}

BOOST_AUTO_TEST_SUITE_END()
