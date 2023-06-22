#include <iostream>
#include <typeinfo>
#include <udho/hazo/detail/function.h>

void f0(int a, const std::string& b, const double& c, bool d){
    return;
}

int f1(int a, const std::string& b, const double& c, bool d){
    std::cout << a << " " << b << " " << c << " " << d << std::endl;
    return 42;
}

std::string f2(int a, const std::string& b, const double& c, bool d){
    return "42";
}

struct X{
    void f0(int a, const std::string& b, const double& c, bool d){
        return;
    }

    int f1(int a, const std::string& b, const double& c, bool d){
        return 42;
    }

    std::string f2(int a, const std::string& b, const double& c, bool d){
        return "42";
    }

    int f3(int a, const std::string& b, const double& c, bool d) const{
        return 84;
    }
};

int main(){
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature( f0))::return_type, void>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature(&f0))::return_type, void>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature( f1))::return_type, int>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature(&f1))::return_type, int>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature( f2))::return_type, std::string>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature(&f2))::return_type, std::string>);

    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature( f0))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature(&f0))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature( f1))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature(&f1))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature( f2))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::hazo::detail::function_signature(&f2))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);

    std::string str = "hello";
    double d = 2.4;

    {
        std::vector<std::string> args;
        args.push_back("24");
        args.push_back("world");
        args.push_back("2.42");
        args.push_back("0");

        auto f = udho::hazo::detail::encapsulate_function(f1);
        // std::cout << f(decltype(f)::decayed_arguments_type()) << std::endl;
        // std::cout << f(decltype(f)::arguments_type(0, str, d, false)) << std::endl;

        f(args.begin(), args.end());
    }

    // X x;
    // const X& y=x;
    //
    // {
    //     auto f = udho::hazo::detail::encapsulate_mem_function(&X::f3, &y);
    //     std::cout << f(decltype(f)::decayed_arguments_type()) << std::endl;
    //     std::cout << f(decltype(f)::arguments_type(0, str, d, false)) << std::endl;
    // }

    return 0;
}
