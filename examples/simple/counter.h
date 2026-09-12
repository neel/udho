#ifndef EXAMPLE_COUNTER_H
#define EXAMPLE_COUNTER_H

#include <udho/url/url.h>
#include <udho/www/www.h>
#include <atomic>

namespace simple{
namespace actions{

class counter{
    std::atomic<int> _value;

public:
    explicit counter(int initial = 0);

    BOOST_SYMBOL_EXPORT void home(udho::www::context<> context);
    BOOST_SYMBOL_EXPORT void increment(udho::www::context<> context);
    BOOST_SYMBOL_EXPORT void reset(udho::www::context<> context);

    auto route() {
        using namespace udho::hazo::string::literals;

        auto actions = udho::url::slot("home"_h,      &actions::counter::home,      this) << udho::url::home (udho::url::verb::get)
                     | udho::url::slot("increment"_h, &actions::counter::increment, this) << udho::url::fixed(udho::url::verb::get, "/increment", "/increment")
                     | udho::url::slot("reset"_h,     &actions::counter::reset,     this) << udho::url::fixed(udho::url::verb::get, "/reset", "/reset")
        ;

        return actions;
    }
};

}
}

#endif // EXAMPLE_COUNTER_H