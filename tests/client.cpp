#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "udho Unit Test (udho::client)"

#include <boost/test/unit_test.hpp>
#include <udho/server.h>
#include <udho/contexts.h>
#include <udho/client.h>
#include <iostream>

typedef udho::servers::quiet::stateless server_type;
typedef udho::contexts::stateless context_type;

BOOST_AUTO_TEST_SUITE(client)

BOOST_AUTO_TEST_CASE(https){
    boost::asio::io_service io;
    context_type::request_type req;
    server_type::attachment_type attachment(io);
    context_type ctx(attachment.aux(), req, attachment);
    
    ctx.client().get("http://google.com")
        .done([ctx](boost::beast::http::status status, const std::string& body) mutable {
            BOOST_CHECK(status == boost::beast::http::status::moved_permanently);
        }).error([ctx](const boost::beast::error_code& ec) mutable {
            BOOST_CHECK_MESSAGE(false, "test request to http://google.com failed");
        })
        .option(udho::client_options::follow_redirect, false)
        .option(udho::client_options::verify_certificate, true);
    
    ctx.client().get("http://google.com")
        .done([ctx](boost::beast::http::status status, const std::string& body) mutable {
            BOOST_CHECK(status == boost::beast::http::status::ok);
        }).error([ctx](const boost::beast::error_code& ec) mutable {
            BOOST_CHECK_MESSAGE(false, "test request to http://google.com failed");
        })
        .option(udho::client_options::follow_redirect, true)
        .option(udho::client_options::verify_certificate, true);
        
    ctx.client().get("http://tls-v1-2.badssl.com")
        .done([ctx](boost::beast::http::status status, const std::string& body) mutable {
            BOOST_CHECK(status == boost::beast::http::status::ok);
        }).error([ctx](const boost::beast::error_code& ec) mutable {
            BOOST_CHECK_MESSAGE(false, "test request to http://google.com failed");
        })
        .option(udho::client_options::follow_redirect, true)
        .option(udho::client_options::verify_certificate, true);
        
    ctx.client().get("http://expired.badssl.com")
        .done([ctx](boost::beast::http::status status, const std::string& body) mutable {
            BOOST_CHECK_MESSAGE(false, "SSL verification succeeded with http://expired.badssl.com which is unexpected");
        }).error([ctx](const boost::beast::error_code& ec) mutable {
            BOOST_CHECK_MESSAGE(ec.value() == 337047686 || ec.value() == 167772294, "expecting SSL varification failure, but got somethign else");
        })
        .option(udho::client_options::follow_redirect, true)
        .option(udho::client_options::verify_certificate, true);
        
    ctx.client().get("http://expired.badssl.com")
        .done([ctx](boost::beast::http::status status, const std::string& body) mutable {
            BOOST_CHECK(status == boost::beast::http::status::ok);
        }).error([ctx](const boost::beast::error_code& ec) mutable {
            BOOST_CHECK_MESSAGE(false, "unexpected error whiling ignoring ssl verification");
        })
        .option(udho::client_options::follow_redirect, true)
        .option(udho::client_options::verify_certificate, false);
        
    udho::url url1 = udho::url::build("https://postman-echo.com/get", {
        {"foo", "bar"},
        {"key", "value"}
    });
    
    BOOST_CHECK_MESSAGE(url1.stringify() == "https://postman-echo.com:443/get?foo=bar&key=value", "unexpected " << url1.stringify());
    
    udho::url url2 = udho::url::build("https://postman-echo.com/get?id=42", {
        {"foo", "bar"},
        {"key", "value"}
    });
    
    BOOST_CHECK_MESSAGE(url2.stringify() == "https://postman-echo.com:443/get?id=42&foo=bar&key=value", "unexpected " << url2.stringify());
        
    io.run();
}

BOOST_AUTO_TEST_SUITE_END()
