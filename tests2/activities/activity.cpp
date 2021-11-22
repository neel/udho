#include "udho/activities/collector.h"
#include "udho/activities/fwd.h"
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

struct MinimalA1: activities::activity<MinimalA1, success_t, failure_t>{
    bool _succeed;
    template <typename CollectorT>
    MinimalA1(CollectorT& collector, bool succeed): activity(collector), _succeed(succeed) {}
    void operator()(){
        if(_succeed)    success(success_t{42});
        else            failure(failure_t{24});
    }
};
struct MinimalA2: activities::activity<MinimalA2, success_t, failure_t>{
    using activity::activity;
    void operator()() {
        success(success_t{42});
    }
};

struct MinimalA3: activities::activity<MinimalA3, success_t, failure_t>{
    using activity::activity;
    void operator()() {
        failure(failure_t{24});
    }
};

TEST_CASE( "activity basic", "[activities]" ) {
    boost::asio::io_service io;
    udho::servers::quiet::stateless::request_type req;
    udho::servers::quiet::stateless::attachment_type attachment(io);
    udho::contexts::stateless ctx(attachment.aux(), req, attachment);

    SECTION( "construction" ) {
        CHECK(std::is_constructible_v<MinimalA1, std::shared_ptr<activities::collector<udho::contexts::stateless, activities::dataset<MinimalA1, MinimalA2>>>&, bool>);
        CHECK(std::is_constructible_v<MinimalA1, std::shared_ptr<activities::collector<udho::contexts::stateless, activities::dataset<MinimalA1>>>&, bool>);
    }

    WHEN("a minimal activity MinimalA1 is constructed using larger collector<MinimalA1, MinimalA2>"){
        auto collector_a1_a2_ptr = activities::collect<MinimalA1, MinimalA2>(ctx);
        MinimalA1 a1_test1(collector_a1_a2_ptr, true);
        activities::accessor<MinimalA1> accessor_a1_test1(collector_a1_a2_ptr);

        THEN("before invocation it is failed but not okay, neither completed nor canceled") {
            CHECK(!accessor_a1_test1.completed<MinimalA1>());
            CHECK(!accessor_a1_test1.canceled<MinimalA1>());
            CHECK(!accessor_a1_test1.okay<MinimalA1>());
            CHECK(accessor_a1_test1.failed<MinimalA1>());
            CHECK(!accessor_a1_test1.exists<MinimalA1>());
        }

        THEN("after successful invocation produces expected results") {
            a1_test1();
            CHECK(accessor_a1_test1.completed<MinimalA1>());
            CHECK(!accessor_a1_test1.failed<MinimalA1>());
            CHECK(accessor_a1_test1.okay<MinimalA1>());
            CHECK(accessor_a1_test1.exists<MinimalA1>());
            CHECK(accessor_a1_test1.success<MinimalA1>()._value == 42);
        }
    }
    WHEN("a minimal activity MinimalA1 is constructed using collector<MinimalA1>") {
        auto collector_a1_ptr = activities::collect<MinimalA1>(ctx);
        MinimalA1 a1_test2(collector_a1_ptr, false);
        activities::accessor<MinimalA1> accessor_a1_test2(collector_a1_ptr);

        THEN("after UNsuccessful invocation produces expected results") {
            a1_test2();
            CHECK(accessor_a1_test2.completed<MinimalA1>());
            CHECK(accessor_a1_test2.failed<MinimalA1>());
            CHECK(!accessor_a1_test2.okay<MinimalA1>());
            CHECK(accessor_a1_test2.exists<MinimalA1>());
            CHECK(accessor_a1_test2.failure<MinimalA1>()._value == 24);
        }
    }

    GIVEN( "two activities are chained through a combinator and collecting data through the same collector" ){
        auto collector_ptr = activities::collect<MinimalA1, MinimalA2>(ctx);
        auto a1_ptr = std::make_shared<MinimalA1>(collector_ptr, false);
        auto a2_ptr = std::make_shared<MinimalA2>(collector_ptr);
        auto combinator = std::make_shared<activities::combinator<MinimalA2, MinimalA1>>(a2_ptr);
        a1_ptr->done(combinator);

        MinimalA1& a1 = *a1_ptr;
        MinimalA2& a2 = *a2_ptr;
        activities::accessor<MinimalA2> accessor_a2(collector_ptr);

        WHEN("the first activity fails") {
            a1();

            THEN("the second activity is canceled") {
                CHECK(accessor_a2.exists<MinimalA2>());
                CHECK(!accessor_a2.completed<MinimalA2>());
                CHECK(accessor_a2.canceled<MinimalA2>());
            }
        }
    }

    GIVEN( "two activities are chained through a combinator and collecting data through the same collector" ){
        auto collector_ptr = activities::collect<MinimalA1, MinimalA2>(ctx);
        auto a1_ptr = std::make_shared<MinimalA1>(collector_ptr, true);
        auto a2_ptr = std::make_shared<MinimalA2>(collector_ptr);
        auto combinator = std::make_shared<activities::combinator<MinimalA2, MinimalA1>>(a2_ptr);
        a1_ptr->done(combinator);

        MinimalA1& a1 = *a1_ptr;
        MinimalA2& a2 = *a2_ptr;
        activities::accessor<MinimalA2> accessor_a2(collector_ptr);

        WHEN("the first activity succeeds") {
            a1();

            THEN("the second activity is completed") {
                CHECK(accessor_a2.exists<MinimalA2>());
                CHECK(accessor_a2.completed<MinimalA2>());
                CHECK(!accessor_a2.canceled<MinimalA2>());
            }
        }
    }

    GIVEN( "two activities are chained through a combinator and collecting data through the same collector" ){
        auto collector_ptr = activities::collect<MinimalA1, MinimalA2>(ctx);
        auto a1_ptr = std::make_shared<MinimalA1>(collector_ptr, false);
        auto a2_ptr = std::make_shared<MinimalA2>(collector_ptr);
        a1_ptr->required(false);
        auto combinator = std::make_shared<activities::combinator<MinimalA2, MinimalA1>>(a2_ptr);
        a1_ptr->done(combinator);

        MinimalA1& a1 = *a1_ptr;
        MinimalA2& a2 = *a2_ptr;
        activities::accessor<MinimalA2> accessor_a2(collector_ptr);

        WHEN("the first activity fails but is optional") {
            a1();

            THEN("the second activity is completed") {
                CHECK(accessor_a2.exists<MinimalA2>());
                CHECK(accessor_a2.completed<MinimalA2>());
                CHECK(!accessor_a2.canceled<MinimalA2>());
            }
        }
    }

    GIVEN( "one activity that depends on two parent activities" ){
        auto collector_ptr = activities::collect<MinimalA1, MinimalA2, MinimalA3>(ctx);
        auto a1_ptr = std::make_shared<MinimalA1>(collector_ptr, false);
        auto a2_ptr = std::make_shared<MinimalA2>(collector_ptr);
        auto a3_ptr = std::make_shared<MinimalA3>(collector_ptr);

        auto combinator = std::make_shared<activities::combinator<MinimalA3, MinimalA1, MinimalA2>>(a3_ptr);
        a1_ptr->done(combinator);
        a2_ptr->done(combinator);

        MinimalA1& a1 = *a1_ptr;
        MinimalA2& a2 = *a2_ptr;
        activities::accessor<MinimalA1> accessor_a1(collector_ptr);
        activities::accessor<MinimalA2> accessor_a2(collector_ptr);
        activities::accessor<MinimalA3> accessor_a3(collector_ptr);

        WHEN("the first parent activity fails") {
            a1();
            a2();

            THEN("the first parent activity is completed and failed") {
                CHECK(accessor_a1.exists<MinimalA1>());
                CHECK(accessor_a1.completed<MinimalA1>());
                CHECK(!accessor_a1.canceled<MinimalA1>());
                CHECK(!accessor_a1.okay<MinimalA1>());
                CHECK(accessor_a1.failed<MinimalA1>());
            }

            THEN("the second parent activity is completed and successful") {
                CHECK(accessor_a2.exists<MinimalA2>());
                CHECK(accessor_a2.completed<MinimalA2>());
                CHECK(!accessor_a2.canceled<MinimalA2>());
                CHECK(accessor_a2.okay<MinimalA2>());
                CHECK(!accessor_a2.failed<MinimalA2>());
            }
            THEN("the third activity is canceled") {
                CHECK(accessor_a3.exists<MinimalA3>());
                CHECK(!accessor_a3.completed<MinimalA3>());
                CHECK(accessor_a3.canceled<MinimalA3>());
                CHECK(!accessor_a3.okay<MinimalA3>());
                CHECK(!accessor_a3.failed<MinimalA3>());
            }
        }
    }

    GIVEN( "one activity that depends on two parent activities" ){
        auto collector_ptr = activities::collect<MinimalA1, MinimalA2, MinimalA3>(ctx);
        auto a1_ptr = std::make_shared<MinimalA1>(collector_ptr, true);
        auto a2_ptr = std::make_shared<MinimalA2>(collector_ptr);
        auto a3_ptr = std::make_shared<MinimalA3>(collector_ptr);

        auto combinator = std::make_shared<activities::combinator<MinimalA3, MinimalA1, MinimalA2>>(a3_ptr);
        a1_ptr->done(combinator);
        a2_ptr->done(combinator);

        MinimalA1& a1 = *a1_ptr;
        MinimalA2& a2 = *a2_ptr;
        activities::accessor<MinimalA1> accessor_a1(collector_ptr);
        activities::accessor<MinimalA2> accessor_a2(collector_ptr);
        activities::accessor<MinimalA3> accessor_a3(collector_ptr);

        WHEN("the both parent activity succeed") {
            a1();
            a2();

            THEN("the first parent activity is completed") {
                CHECK(accessor_a1.exists<MinimalA1>());
                CHECK(accessor_a1.completed<MinimalA1>());
                CHECK(!accessor_a1.canceled<MinimalA1>());
                CHECK(accessor_a1.okay<MinimalA1>());
                CHECK(!accessor_a1.failed<MinimalA1>());
            }
            
            THEN("the second parent activity is completed") {
                CHECK(accessor_a2.exists<MinimalA2>());
                CHECK(accessor_a2.completed<MinimalA2>());
                CHECK(!accessor_a2.canceled<MinimalA2>());
                CHECK(accessor_a2.okay<MinimalA2>());
                CHECK(!accessor_a2.failed<MinimalA2>());
            }
            THEN("the third activity is completed too") {
                CHECK(accessor_a3.exists<MinimalA3>());
                CHECK(accessor_a3.completed<MinimalA3>());
                CHECK(!accessor_a3.canceled<MinimalA3>());
                CHECK(!accessor_a3.okay<MinimalA3>());
                CHECK(accessor_a3.failed<MinimalA3>());
            }
        }
    }

    GIVEN( "an activity fails" ) {
        THEN( "the if_failed callback is called" ) {
            auto collector_ptr = activities::collect<MinimalA1>(ctx);
            MinimalA1 a1(collector_ptr, false);
            activities::accessor<MinimalA1> accessor(collector_ptr);
            int failure_value = 0;
            a1.if_failed([&failure_value](const failure_t& failure){
                failure_value = failure._value;
                return true;
            });
            a1();
            CHECK(failure_value == 24);
            CHECK(accessor.exists<MinimalA1>());
            CHECK(accessor.completed<MinimalA1>());
            CHECK(accessor.failed<MinimalA1>());
            CHECK(!accessor.okay<MinimalA1>());
            CHECK(!accessor.canceled<MinimalA1>());
            CHECK(accessor.failure<MinimalA1>()._value == 24);
        }
        THEN( "then child activities are cancelled if the if_failed callback returns true" ) {
            auto collector_ptr = activities::collect<MinimalA1, MinimalA2>(ctx);
            auto a1_ptr = std::make_shared<MinimalA1>(collector_ptr, false);
            auto a2_ptr = std::make_shared<MinimalA2>(collector_ptr);

            auto combinator = std::make_shared<activities::combinator<MinimalA2, MinimalA1>>(a2_ptr);
            a1_ptr->done(combinator);

            activities::accessor<MinimalA1, MinimalA2> accessor(collector_ptr);

            MinimalA1& a1 = *a1_ptr;

            int failure_value = 0;
            a1.if_failed([&failure_value](const failure_t& failure){
                failure_value = failure._value;
                return true;
            });
            
            a1();
            CHECK(failure_value == 24);

            CHECK(accessor.exists<MinimalA1>());
            CHECK(accessor.completed<MinimalA1>());
            CHECK(accessor.failed<MinimalA1>());
            CHECK(!accessor.okay<MinimalA1>());
            CHECK(!accessor.canceled<MinimalA1>());
            CHECK(accessor.failure<MinimalA1>()._value == 24);

            CHECK(accessor.exists<MinimalA2>());
            CHECK(!accessor.completed<MinimalA2>());
            CHECK(!accessor.failed<MinimalA2>());
            CHECK(!accessor.okay<MinimalA2>());
            CHECK(accessor.canceled<MinimalA2>());
        }
        THEN( "then child activities are not cancelled if the if_failed callback returns false" ) {
            auto collector_ptr = activities::collect<MinimalA1, MinimalA2>(ctx);
            auto a1_ptr = std::make_shared<MinimalA1>(collector_ptr, false);
            auto a2_ptr = std::make_shared<MinimalA2>(collector_ptr);

            auto combinator = std::make_shared<activities::combinator<MinimalA2, MinimalA1>>(a2_ptr);
            a1_ptr->done(combinator);

            activities::accessor<MinimalA1, MinimalA2> accessor(collector_ptr);

            MinimalA1& a1 = *a1_ptr;

            int failure_value = 0;
            a1.if_failed([&failure_value](const failure_t& failure){
                failure_value = failure._value;
                return false;
            });
            
            a1();
            CHECK(failure_value == 24);

            CHECK(accessor.exists<MinimalA1>());
            CHECK(accessor.completed<MinimalA1>());
            CHECK(accessor.failed<MinimalA1>());
            CHECK(!accessor.okay<MinimalA1>());
            CHECK(!accessor.canceled<MinimalA1>());
            CHECK(accessor.failure<MinimalA1>()._value == 24);

            CHECK(accessor.exists<MinimalA2>());
            CHECK(accessor.completed<MinimalA2>());
            CHECK(!accessor.failed<MinimalA2>());
            CHECK(accessor.okay<MinimalA2>());
            CHECK(!accessor.canceled<MinimalA2>());
            CHECK(accessor.success<MinimalA2>()._value == 42);
        }
    }
}
