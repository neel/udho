#include "counter.h"
#include <string>

simple::actions::counter::counter(int initial): _value(initial) {}

void simple::actions::counter::home(udho::www::context<> context){
    context << "Counter Example\n\n";
    context << "Current value: " << std::to_string(_value.load()) << "\n\n";
    context << "Available URLs:\n";
    context << "  http://localhost:9999/counter/\n";
    context << "  http://localhost:9999/counter/increment\n";
    context << "  http://localhost:9999/counter/reset\n";
    context.finish();
}

void simple::actions::counter::increment(udho::www::context<> context){
    int value = ++_value;
    context << "Counter incremented to " << std::to_string(value);
    context.finish();
}

void simple::actions::counter::reset(udho::www::context<> context){
    _value.store(0);
    context << "Counter reset to 0";
    context.finish();
}
