#include "udho/activities/collector.h"
#include "udho/activities/fwd.h"
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <type_traits>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>

namespace activities = udho::activities;

struct success_t{
    int _value;
    inline success_t(): _value(0){}
    inline explicit success_t(int v): _value(v){}
};
struct failure_t{
    int _value;
    inline failure_t(): _value(0){}
    inline explicit failure_t(int v): _value(v){}
};

template <int N>
struct A: activities::activity<A<N>, success_t, failure_t>{
    using activity_type = activities::activity<A<N>, success_t, failure_t>;

    int _value;
    bool _succeed;
    boost::asio::deadline_timer _timer;

    template <typename CollectorT>
    A(CollectorT& collector, int value, bool succeed = true): activity_type(collector), _value(value), _succeed(succeed), _timer(collector->context().io()) {}
    int value() const { return _value; }
    void operator()(){
        _value = 2 * _value;
        _timer.expires_from_now(boost::posix_time::milliseconds(100));
        _timer.async_wait(boost::bind(&A<N>::finished, activity_type::self(), boost::asio::placeholders::error));
    }
    void finished(const boost::system::error_code&){
        _value = 1+ _value;
        if(_succeed)    activity_type::success(success_t{ _value});
        else            activity_type::failure(failure_t{-_value});
    }
};

using A0 = A<0>;
using A1 = A<1>;
using A2 = A<2>;
using A3 = A<3>;
using A4 = A<4>;
using A5 = A<5>;
using A6 = A<6>;
using A7 = A<7>;

TEST_CASE("subtask basic", "[activities]") {
    boost::asio::io_service io;
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);

    auto collector = activities::collect<A0,A1,A2,A3,A4>(ctx);
    auto a0 = activities::perform<A0>::with(collector, 100);
    auto a1 = activities::after(a0).perform<A1>(collector, 101);
    auto a2 = activities::after(a0).perform<A1>(collector, 102);
    auto a3 = activities::after(a1, a2).perform<A3>(collector, 103);
    auto a4 = activities::after(a3).perform<A4>(collector, 104);

    bool test_run = false;

    udho::require<A4>::with(collector).exec([ctx, &test_run](const activities::accessor<A0, A1, A2, A3, A4>& data){
        REQUIRE(data.completed<A0>());
        REQUIRE(data.completed<A1>());
        REQUIRE(data.completed<A2>());
        REQUIRE(data.completed<A3>());
        REQUIRE(data.completed<A4>());

        test_run = true;
    });

    a0();
    REQUIRE(!test_run);

    io.run();
    REQUIRE(test_run);
}