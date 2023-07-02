#include <iostream>
#include <typeinfo>
#include <udho/url/detail/function.h>
#include <udho/url/action.h>
// #include <udho/url/list.h>
#include <udho/hazo/string/basic.h>
#include <udho/hazo/seq/seq.h>

struct nodef{
    nodef() = delete;
    nodef(int) {}
};

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
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f0))::return_type, void>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f0))::return_type, void>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f1))::return_type, int>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f1))::return_type, int>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f2))::return_type, std::string>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f2))::return_type, std::string>);

    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f0))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f0))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f1))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f1))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature( f2))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);
    static_assert(std::is_same_v<decltype(udho::url::detail::function_signature(&f2))::arguments_type, std::tuple<int, const std::string&, const double&, bool>>);

    std::string str = "hello";
    double d = 2.4;

    // {
        std::vector<std::string> args;
        args.push_back("24");
        args.push_back("world");
        args.push_back("2.42");
        args.push_back("0");

        auto f = udho::url::detail::encapsulate_function(f1);
        // std::cout << f(decltype(f)::decayed_arguments_type()) << std::endl;
        // std::cout << f(decltype(f)::arguments_type(0, str, d, false)) << std::endl;

        std::cout << f.args << std::endl;

        f(args.begin(), args.end());
    // }

    // X x;
    // const X& y=x;
    //
    // {
    //     auto f = udho::url::detail::encapsulate_mem_function(&X::f3, &y);
    //     std::cout << f(decltype(f)::decayed_arguments_type()) << std::endl;
    //     std::cout << f(decltype(f)::arguments_type(0, str, d, false)) << std::endl;
    // }

    using namespace udho::hazo::string::literals;

    auto chain = udho::hazo::make_seq_d(
        udho::url::action(udho::url::detail::encapsulate_function(f0), std::regex("f0"), "f0"_h),
        udho::url::action(udho::url::detail::encapsulate_function(f1), std::regex("f1"), "f1"_h),
        udho::url::action(udho::url::detail::encapsulate_function(f2), std::regex("f2"), "f2"_h)
    );
    auto f1_ = chain["f1"_h];
    std::cout << "---" << std::endl;
    f1_(args.begin(), args.end());

    // udho::hazo::seq_d<int, nodef, std::string> tt(2, nodef(2), "Hello");

    return 0;
}
