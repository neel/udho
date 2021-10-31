#include "udho/activities/accessor.h"
#include "udho/activities/collector.h"
#include "udho/activities/fwd.h"
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/thread/thread_time.hpp>
#include <iterator>
#include <type_traits>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <udho/activities.h>
#include <udho/contexts.h>
#include <udho/server.h>
#include <string>
#include <boost/thread.hpp>

namespace activities = udho::activities;

struct success_t{
    int _value;

    inline success_t(): _value(0){}
    inline explicit success_t(int v): _value(v){}

    int value() const { return _value; }
};
struct failure_t{
    int _value;

    inline failure_t(): _value(0){}
    inline explicit failure_t(int v): _value(v){}

    int value() const { return _value; }
};

template <int N>
struct accessor_storage;

template <int N>
struct A: activities::activity<A<N>, success_t, failure_t>, private accessor_storage<N>{
    using activity_type = activities::activity<A<N>, success_t, failure_t>;
    using accessor_storage_type = accessor_storage<N>;
    using storage_type = std::vector<int>;

    template <typename CollectorT>
    A(CollectorT& collector, int value, bool succeed = true): 
        activity_type(collector), 
        accessor_storage_type(collector), 
        _value(value), 
        _succeed(succeed), 
        _timer(collector->context().io()), 
        _time(boost::get_system_time()) 
    {}

    int value() const { return _value; }
    const boost::posix_time::ptime& time() const { return _time; }

    void operator()(){
        _value = 2 * _value;

        auto now = boost::get_system_time();
        auto at  = now + boost::posix_time::milliseconds(100 + (rand() % 2000));

        _timer.expires_at(at);
        _timer.async_wait(boost::bind(&A<N>::finished, activity_type::self(), boost::asio::placeholders::error));

        std::cout << "A<" << N << ">" << " " << now << " -> " << at << std::endl;
    }
    void finished(const boost::system::error_code&){
        _value = 1+ _value;
        _time  = boost::get_system_time();

        accessor_storage_type::populate(_storage);

        if(_succeed)    activity_type::success(success_t{ _value});
        else            activity_type::failure(failure_t{-_value});
    }

    storage_type::const_iterator begin() const { return _storage.cbegin(); }
    storage_type::const_iterator end() const { return _storage.cend(); }
    storage_type::size_type size() const { return _storage.size(); }

    private:
        int _value;
        boost::posix_time::ptime _time;
        bool _succeed;
        boost::asio::deadline_timer _timer;
        std::vector<int> _storage;
};

template <int N>
struct accessor_storage: private accessor_storage<N-1>{
    using previous_accessor_storage = accessor_storage<N-1>;

    template <typename CollectorT>
    accessor_storage(CollectorT collector): accessor_storage<N-1>(collector), _accessor(collector){}

    activities::accessor<A<N-1>> _accessor;

    void populate(std::vector<int>& storage){
        previous_accessor_storage::populate(storage);
        const success_t& success = _accessor.template success<A<N-1>>();
        storage.push_back(success.value());
    }
}; 

template <>
struct accessor_storage<0>{
    template <typename CollectorT>
    accessor_storage(CollectorT){}

    void populate(std::vector<int>&){}
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
    auto a0 = activities::after()       .perform<A0>(collector, 100);
    auto a1 = activities::after(a0)     .perform<A1>(collector, 102);
    auto a2 = activities::after(a0)     .perform<A2>(collector, 104);
    auto a3 = activities::after(a1, a2) .perform<A3>(collector, 106);
    auto a4 = activities::after(a3)     .perform<A4>(collector, 108);

    bool test_run = false;

    activities::after(a4).finish(collector, [ctx, &test_run](const activities::accessor<A0, A1, A2, A3, A4>& data){
        REQUIRE(data.completed<A0>());
        REQUIRE(data.completed<A1>());
        REQUIRE(data.completed<A2>());
        REQUIRE(data.completed<A3>());
        REQUIRE(data.completed<A4>());

        test_run = true;
    });

    a0();
    REQUIRE(!test_run);

    boost::thread_group group;
    for (unsigned i = 0; i < 2; ++i){
        group.create_thread(boost::bind(&boost::asio::io_context::run, &io));
    }

    group.join_all();
    REQUIRE(test_run);

    THEN("all subtasks are executed in proper sequence") {
        REQUIRE(a0->value() == 100*2+1); // 201
        REQUIRE(a1->value() == 102*2+1); // 205
        REQUIRE(a2->value() == 104*2+1); // 209
        REQUIRE(a3->value() == 106*2+1); // 213
        REQUIRE(a4->value() == 108*2+1); // 217
    }

    THEN("all subtasks are executed after all dependent subtasks are done") {
        REQUIRE(a0->time() <= a1->time());
        REQUIRE(a0->time() <= a2->time());
        REQUIRE(a1->time() <= a3->time());
        REQUIRE(a3->time() <= a3->time());
        REQUIRE(a3->time() <= a4->time());
    }

    THEN("all subtasks can access its previous subtasks data through appropriate accessor") {
        std::cout << "a0" << " ";
        std::copy(a0->begin(), a0->end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;

        std::cout << "a1" << " ";
        std::copy(a1->begin(), a1->end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;

        std::cout << "a2" << " ";
        std::copy(a2->begin(), a2->end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;

        std::cout << "a3" << " ";
        std::copy(a3->begin(), a3->end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;

        std::cout << "a4" << " ";
        std::copy(a4->begin(), a4->end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;
    }
}