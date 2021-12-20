#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>

namespace activities = udho::activities;

struct A{
    struct result_type {
        std::string _value;
        inline result_type() = default;
        inline explicit result_type(const std::string& v): _value(v){}
    };
};
struct B{
    struct result_type {
        int _value;
        inline result_type() = default;
        inline explicit result_type(int v): _value(v){}
    };
};
struct C{
    struct result_type {
        double _value;
        inline result_type() = default;
        inline explicit result_type(double v): _value(v){}
    };
};
struct D{
    typedef C::result_type result_type;
};
struct E{
    typedef int result_type;
};

TEST_CASE( "activity data", "[activity]" ) {
    boost::asio::io_service io;
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);

    GIVEN( "a collector<A, B, C, D>" ) {
        WHEN( "some data has been inserted into it in the ABCD order" ) {
            auto collector = activities::collect<A, B, C, D>(ctx);
            *collector  << activities::detail::labeled<A, A::result_type>(A::result_type{"Hello World"}) 
                        << activities::detail::labeled<B, B::result_type>(B::result_type{42})
                        << activities::detail::labeled<C, C::result_type>(C::result_type{3.14})
                        << activities::detail::labeled<D, D::result_type>(D::result_type{2.718});

            THEN( "the same data can be retrieved in the same order" ) {
                activities::detail::labeled<A, A::result_type> a;
                activities::detail::labeled<B, B::result_type> b;
                activities::detail::labeled<C, C::result_type> c;
                activities::detail::labeled<D, D::result_type> d;

                *collector >> a >> b >> c >> d;

                CHECK(a.get()._value == "Hello World");
                CHECK(b.get()._value == 42);
                CHECK(c.get()._value == 3.14);
                CHECK(d.get()._value == 2.718);
            }

            THEN( "the same data can be retrieved in opposite order" ) {
                activities::detail::labeled<A, A::result_type> a;
                activities::detail::labeled<B, B::result_type> b;
                activities::detail::labeled<C, C::result_type> c;
                activities::detail::labeled<D, D::result_type> d;

                *collector >> d >> c >> b >> a;

                CHECK(a.get()._value == "Hello World");
                CHECK(b.get()._value == 42);
                CHECK(c.get()._value == 3.14);
                CHECK(d.get()._value == 2.718);
            }
        }

        WHEN( "some data has been inserted into it in the CBA order" ) {
            auto collector = activities::collect<A, B, C, D>(ctx);
            *collector  << activities::detail::labeled<D, D::result_type>(D::result_type{2.718}) 
                        << activities::detail::labeled<C, C::result_type>(C::result_type{3.14})
                        << activities::detail::labeled<B, B::result_type>(B::result_type{42})
                        << activities::detail::labeled<A, A::result_type>(A::result_type{"Hello World"});

            THEN( "the same data can be retrieved in the same order" ) {
                activities::detail::labeled<A, A::result_type> a;
                activities::detail::labeled<B, B::result_type> b;
                activities::detail::labeled<C, C::result_type> c;
                activities::detail::labeled<D, D::result_type> d;

                *collector >> a >> b >> c >> d;

                CHECK(a.get()._value == "Hello World");
                CHECK(b.get()._value == 42);
                CHECK(c.get()._value == 3.14);
                CHECK(d.get()._value == 2.718);
            }

            THEN( "the same data can be retrieved in opposite order" ) {
                activities::detail::labeled<A, A::result_type> a;
                activities::detail::labeled<B, B::result_type> b;
                activities::detail::labeled<C, C::result_type> c;
                activities::detail::labeled<D, D::result_type> d;

                *collector >> d >> c >> b >> a;

                CHECK(a.get()._value == "Hello World");
                CHECK(b.get()._value == 42);
                CHECK(c.get()._value == 3.14);
                CHECK(d.get()._value == 2.718);
            }

        }
    }

    GIVEN( "a collector<A, B, C, D, E> where no value for E is set" ) {
        auto collector = activities::collect<A, B, C, D, E>(ctx);
        *collector  << activities::detail::labeled<A, A::result_type>(A::result_type{"Hello World"}) 
                    << activities::detail::labeled<B, B::result_type>(B::result_type{42})
                    << activities::detail::labeled<C, C::result_type>(C::result_type{3.14})
                    << activities::detail::labeled<D, D::result_type>(D::result_type{2.718}) ;

        WHEN( "a complete accessor<A, B, C, D, E> is used to access it") {
            activities::accessor<A, B, C, D, E> accessor(collector);
            THEN( "whatever set into the collector can be accessed by that accessor" ) {
                A::result_type a = accessor.get<A>();
                B::result_type b = accessor.get<B>();
                C::result_type c = accessor.get<C>();
                D::result_type d = accessor.get<D>();

                CHECK(!accessor.exists<E>());
                CHECK(accessor.exists<D>());
                CHECK(a._value == "Hello World");
                CHECK(b._value == 42);
                CHECK(c._value == 3.14);
                CHECK(d._value == 2.718);
            }
        }

        WHEN( "a partial accessor<C, A, D> is used to access it") {
            activities::accessor<C, A, D> accessor(collector);
            THEN( "whatever set into the collector can be accessed by that accessor" ) {
                A::result_type a = accessor.get<A>();
                C::result_type c = accessor.get<C>();
                D::result_type d = accessor.get<D>();

                CHECK(a._value == "Hello World");
                CHECK(c._value == 3.14);
                CHECK(d._value == 2.718);
            }
        }

        WHEN( "a partial accessor<C, A, D> is used to modify it") {
            activities::accessor<C, A, D> accessor(collector);
            THEN( "whatever set into the collector can be accessed by that accessor" ) {
                accessor.set<A>(A::result_type{"Hello Mars"});
                accessor.set<C>(C::result_type{4.2});

                activities::detail::labeled<A, A::result_type> a;
                activities::detail::labeled<C, C::result_type> c;
                activities::detail::labeled<D, D::result_type> d;

                *collector >> c >> a >> d;

                CHECK(a.get()._value == "Hello Mars");
                CHECK(c.get()._value == 4.2);
                CHECK(d.get()._value == 2.718);
            }
        }
    }
}