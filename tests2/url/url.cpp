#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <string>
#include <udho/url/action.h>
#include <type_traits>

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


TEST_CASE("url common functionalities", "[url]") {
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

    using namespace udho::hazo::string::literals;

    auto slot_f0 = udho::url::slot("f0"_h, &f0);

    CHECK(slot_f0.args == 4);
    CHECK(slot_f0.key() == "f0"_h);
}
