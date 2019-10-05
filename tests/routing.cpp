#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "udho Unit Test (udho::router)"
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <udho/router.h>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <iostream>

template <typename F>
struct checker{
    F _check_lambda;
    
    checker(F lambda): _check_lambda(lambda){}
    
    template<bool isRequest, class Body, class Fields>
    void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg) const {
        _check_lambda(msg.body());
    }
};

template <typename F>
checker<F> generate_checker(F f){
    return checker<F>(f);
}

std::string hello(udho::request_type req){
    return "Hello World";
}

std::string data(udho::request_type req){
    return "{id: 2, name: 'udho'}";
}

int add(udho::request_type req, int a, int b){
    return a + b;
}

BOOST_AUTO_TEST_SUITE(router)

BOOST_AUTO_TEST_CASE(mapping){
    auto router = udho::router()
        | (udho::get(&hello).plain() = "^/hello$")
        | (udho::get(&data).json()   = "^/data$")
        | (udho::get(&add).plain()   = "^/add/(\\d+)/(\\d+)$");
    router.serve(udho::request_type(), boost::beast::http::verb::get, "/hello", generate_checker([](const std::string& res){
        BOOST_CHECK(res == "Hello World");
    }));
    router.serve(udho::request_type(), boost::beast::http::verb::get, "/data", generate_checker([](const std::string& res){
        BOOST_CHECK(res == "{id: 2, name: 'udho'}");
    }));
    router.serve(udho::request_type(), boost::beast::http::verb::get, "/add/2/3", generate_checker([](const std::string& res){
        BOOST_CHECK(res == "5");
    }));
}

BOOST_AUTO_TEST_SUITE_END()
