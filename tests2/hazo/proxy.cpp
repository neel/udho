#include "udho/hazo/node/fwd.h"
#include <type_traits>
#define CATCH_CONFIG_MAIN
#if WITH_CATCH_VERSION_2
#include <catch2/catch.hpp>
#else
#include <catch2/catch_all.hpp>
#endif
#include <udho/hazo/node/node.h>
#include <udho/hazo/node/proxy.h>
#include <string>

namespace h = udho::hazo;


TEST_CASE( "transparent node proxy", "[hazo]" ) {

    GIVEN( "A node chain" ) {
        h::node<int, std::string, double, char, std::string, int> chain(42, "Fourty Two", 4.2, '!', "Twenty Four", 24);
        h::node<int, std::string, double, char, std::string, int> copy = chain;

        CHECK(chain.data<0>() == copy.data<0>());
        CHECK(chain.data<1>() == copy.data<1>());
        CHECK(chain.data<2>() == copy.data<2>());
        CHECK(chain.data<3>() == copy.data<3>());
        CHECK(chain.data<4>() == copy.data<4>());
        CHECK(chain.data<5>() == copy.data<5>());

        WHEN ( "proxy has same items as the original node chain" ) {
            h::proxy<int, std::string, double, char, std::string, int> proxy(chain);
            
            THEN( "same data can be retrieved before any update" ) {
                CHECK(chain.data<0>() == proxy.data<0>());
                CHECK(chain.data<1>() == proxy.data<1>());
                CHECK(chain.data<2>() == proxy.data<2>());
                CHECK(chain.data<3>() == proxy.data<3>());
                CHECK(chain.data<4>() == proxy.data<4>());
                CHECK(chain.data<5>() == proxy.data<5>());
                CHECK(chain.data<int, 0>() == proxy.data<int, 0>());
                CHECK(chain.data<int, 1>() == proxy.data<int, 1>());
            }

            THEN( "same value can be retrieved before any update" ) {
                CHECK(chain.value<0>() == proxy.value<0>());
                CHECK(chain.value<1>() == proxy.value<1>());
                CHECK(chain.value<2>() == proxy.value<2>());
                CHECK(chain.value<3>() == proxy.value<3>());
                CHECK(chain.value<4>() == proxy.value<4>());
                CHECK(chain.value<5>() == proxy.value<5>());
                CHECK(chain.value<int, 0>() == proxy.value<int, 0>());
                CHECK(chain.value<int, 1>() == proxy.value<int, 1>());
            }
        }

        WHEN ( "updation of data in chain" ) {
            h::proxy<int, std::string, double, char, std::string, int> proxy(chain);
            
            chain.data<0>() = 24;
            chain.data<1>() = "Two Fourty";

            THEN( "same data can be retrieved after the original chain is updated" ) {
                CHECK(chain.data<0>() == proxy.data<0>());
                CHECK(chain.data<1>() == proxy.data<1>());
            }
        }

        WHEN ( "updation of data through proxy" ) {
            h::proxy<int, std::string, double, char, std::string, int> proxy(chain);
            
            proxy.data<0>() = 42;
            proxy.data<1>() = "Fourty Two";

            THEN( "same data can be retrieved after the original chain is updated" ) {
                CHECK(chain.data<0>() == proxy.data<0>());
                CHECK(chain.data<1>() == proxy.data<1>());
            }
        }

        WHEN( "a proxy with smaller projection in same order can be constructed" ) {
            h::proxy<int, std::string, int> proxy(chain);

            THEN( "both the proxy correctly points to the appropriate item in original chain" ) {
                CHECK(proxy.data<0>() == chain.data<0>());
                CHECK(proxy.data<1>() == chain.data<1>());
                CHECK(proxy.data<2>() == chain.data<5>());
            }

            THEN( "a secondary proxy can be constructed using smaller projection" ) {
                h::proxy<int, std::string> proxy2(proxy);

                CHECK(proxy2.data<0>() == proxy.data<0>());
                CHECK(proxy2.data<1>() == proxy.data<1>());
            }
        }

        WHEN( "a proxy with smaller projection in different order can be constructed" ) {
            h::proxy<int, std::string, int, std::string> proxy(chain);

            THEN( "both the proxy correctly points to the appropriate item in original chain" ) {
                CHECK(proxy.data<0>() == chain.data<0>());
                CHECK(proxy.data<1>() == chain.data<1>());
                CHECK(proxy.data<2>() == chain.data<5>());
                CHECK(proxy.data<3>() == chain.data<4>());
            }

            THEN( "a secondary proxy can be constructed using smaller projection" ) {
                h::proxy<std::string, std::string> proxy2(proxy);

                CHECK(proxy2.data<0>() == proxy.data<1>());
                CHECK(proxy2.data<1>() == proxy.data<3>());
            }
        }
    }
}
