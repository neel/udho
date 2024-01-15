#include <iostream>
#include <typeinfo>
#include <udho/url/detail/function.h>
#include <udho/url/url.h>
// #include <udho/url/list.h>
#include <udho/hazo/string/basic.h>
#include <udho/hazo/seq/seq.h>
#include <scn/scn.h>
#include <scn/tuple_return.h>

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

    X x;
    // const X& y=x;
    //
    // {
    //     auto f = udho::url::detail::encapsulate_mem_function(&X::f3, &y);
    //     std::cout << f(decltype(f)::decayed_arguments_type()) << std::endl;
    //     std::cout << f(decltype(f)::arguments_type(0, str, d, false)) << std::endl;
    // }

    {
        using namespace udho::hazo::string::literals;

        auto chain =
            udho::url::slot("f0"_h, &f0)         << udho::url::regx(udho::url::verb::get, "f0", "/f0")                                           |
            udho::url::slot("f1"_h, &f1)         << udho::url::regx(udho::url::verb::get, "f1/(\\w+)/(\\w+)/(\\d+)/(\\d+)", "/f1/{}/{}/{}")      |
            udho::url::slot("f2"_h, &f2)         << udho::url::regx(udho::url::verb::get, "f2", "/f2")                                           |
            udho::url::slot("xf1"_h, &X::f1, &x) << udho::url::regx(udho::url::verb::get, "xf1", "/x/f1");

        std::cout << chain << std::endl;

        auto f1_ = chain["f1"_h];
        f1_(args.begin(), args.end());
        // auto results = f1_.match("f1");
        // std::cout << "results.matched: " << results.matched() << std::endl;
        bool found = f1_.invoke(std::string("f1/23/325/23/1"));
        std::cout << "found: " << found << std::endl;
        std::cout << f1_.fill(std::make_tuple(24,"world", 2.4, 0)) << std::endl;
    }{
        std::cout << "--------" << std::endl;

        using namespace udho::hazo::string::literals;

        auto chain =
            udho::url::slot("f0"_h, &f0)         << udho::url::scan(udho::url::verb::get, "f0", "/f0")                         |
            udho::url::slot("f1"_h, &f1)         << udho::url::scan(udho::url::verb::get, "f1/{}/{}/{}/{}",  "/f1/{}/{}/{}")   |
            udho::url::slot("f2"_h, &f2)         << udho::url::scan(udho::url::verb::get, "f2", "/f2")                         |
            udho::url::slot("xf1"_h, &X::f1, &x) << udho::url::scan(udho::url::verb::get, "xf1", "/x/f1");

        std::cout << chain << std::endl;

        auto f1_ = chain["f1"_h];
        f1_(args.begin(), args.end());
        // auto results = f1_.match("f1");
        // std::cout << "results.matched: " << results.matched() << std::endl;
        bool found = f1_.invoke(std::string("f1/23/hello/24/1"));
        std::cout << "found: " << found << std::endl;
        std::cout << f1_.fill(std::make_tuple(24, "world", 2.4, 0)) << std::endl;
    }

    // udho::hazo::seq_d<int, nodef, std::string> tt(2, nodef(2), "Hello");

    // {
    //     std::uint32_t post_id, user_id;
    //     auto result = scn::scan("/posts/623635/user/42/view", "/posts/{}/user/{}/view", post_id, user_id);
    //     std::cout << "result:  " << (bool) result << std::endl;
    //     std::cout << "post_id: " << post_id << std::endl;
    //     std::cout << "user_id: " << user_id << std::endl;
    // }
    //
    // {
    //     auto [result, post_id, user_id] = scn::scan_tuple<std::uint32_t, std::uint32_t>("/posts/623635/user/42/view", "/posts/{}/user/{}/view");
    //     std::cout << "result:  " << (bool) result << std::endl;
    //     std::cout << "post_id: " << post_id << std::endl;
    //     std::cout << "user_id: " << user_id << std::endl;
    // }
    //
    // {
    //     std::tuple<std::uint32_t, std::string, std::uint32_t> tuple;
    //     auto result = scn::make_result("/posts/623635/user/neel/view/23");
    //
    //     std::tuple<decltype(result.range()), std::string> subject_format(result.range(), "/posts/{}/user/{}/view/{}");
    //     auto args = std::tuple_cat(subject_format, tuple);
    //     result = std::apply(scn::scan<decltype(result.range()), std::string, std::uint32_t, std::string, std::uint32_t>, args);
    //
    //     std::cout << "result:  " << std::boolalpha << (bool) result << std::endl;
    //     std::cout << "post_id: " << std::get<2>(args) << std::endl;
    //     std::cout << "user_id: " << std::get<3>(args) << std::endl;
    // }

    std::tuple<std::uint32_t, std::string, std::uint32_t> tuple;
    std::string format  = "path/{}/{}/{}";
    std::string subject = "path/23/hello/8";
    auto res = udho::url::pattern::detail::scan_helper::apply(subject, format, tuple);
    std::cout << "res: " << (bool) res << std::endl;
    std::cout << "0: " << std::get<0>(tuple) << std::endl;
    std::cout << "1: " << std::get<1>(tuple) << std::endl;
    std::cout << "2: " << std::get<2>(tuple) << std::endl;
    return 0;
}
