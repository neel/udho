#include <typeinfo>
#include <iostream>

template <typename H, typename... T>
struct basic_container;

template <typename ContainerT, typename U>
struct eliminate;

template <typename ContainerT, typename T, typename... Rest>
struct eliminate_all;

template <typename ContainerT, typename... T>
struct append;

template <typename ContainerT, typename... T>
struct prepend;

template <typename ContainerT, typename T, typename... Rest>
struct eliminate_all{
    using type = typename eliminate_all<
        typename eliminate<ContainerT, T>::type, 
        Rest...
    >::type;
};

template <typename ContainerT, typename T>
struct eliminate_all<ContainerT, T>{
    using type = typename eliminate<ContainerT, T>::type;
};

template <template <typename...> class ContainerT, typename H, typename... T>
struct first_of{
    using type = H;
};

template <template <typename...> class ContainerT, typename... H, typename... T>
struct first_of<ContainerT, ContainerT<H...>, T...>{
    using type = typename first_of<ContainerT, H...>::type;
};

template <template <typename...> class ContainerT, typename H, typename... T>
struct rest_of{
    using type = ContainerT<T...>;
};

template <template <typename...> class ContainerT, typename... H, typename... T>
struct rest_of<ContainerT, ContainerT<H...>, T...>{
    using type = typename rest_of<ContainerT, H..., T...>::type;
};


// { flatten 

template <template <typename...> class ContainerT, typename InitialT, typename... X>
struct basic_flatten{
    using initial = typename append<InitialT, typename first_of<ContainerT, X...>::type>::type;
    using rest = typename rest_of<ContainerT, X...>::type;
    using type = typename basic_flatten<ContainerT, initial, rest>::type;
};

template <template <typename...> class ContainerT, typename InitialT>
struct basic_flatten<ContainerT, InitialT, ContainerT<void>>{
    using initial = InitialT;
    using rest = void;
    using type = initial;
};

template <template <typename...> class ContainerT, typename... X>
struct flatten{
    using type = typename basic_flatten<ContainerT, ContainerT<>, X...>::type;
};

// } flatten

// { container

template <typename H, typename T>
struct node{
    using head = H;
    using tail = T;
    
    static const char* pretty(){
        return __PRETTY_FUNCTION__;
    }
};

template <typename H>
struct node<H, void>{
    using head = H;
    using tail = void;
    
    static const char* pretty(){
        return __PRETTY_FUNCTION__;
    }
};

template <typename H = void, typename... T>
struct basic_container{
    using head = typename first_of<basic_container, H, T...>::type;
    using tail = typename rest_of<basic_container, H, T...>::type;
    using base = node<head, tail>;
    
    static const char* pretty(){
        return __PRETTY_FUNCTION__;
    }
};

// { operations

template <typename... X, typename... T>
struct append<basic_container<X...>, T...>{
    using type = basic_container<X..., T...>;
};

template <typename... T>
struct append<basic_container<void>, T...>{
    using type = basic_container<T...>;
};

template <typename... X, typename... T>
struct prepend<basic_container<X...>, T...>{
    using type = basic_container<T..., X...>;
};

template <typename... T>
struct prepend<basic_container<void>, T...>{
    using type = basic_container<T...>;
};

template <typename H, typename... X, typename U>
struct eliminate<basic_container<H, X...>, U>{
    enum { 
        matched = std::is_same<H, U>::value
    };
    using tail = basic_container<X...>;
    using type = typename std::conditional<matched, 
        tail,
        typename prepend<typename eliminate<tail, U>::type, H>::type
    >::type;
};

template <typename H, typename U>
struct eliminate<basic_container<H>, U>{
    enum { 
        matched = std::is_same<H, U>::value
    };
    using type = typename std::conditional<matched, 
        basic_container<void>,
        basic_container<H>
    >::type;
};

// } operations 

template <typename... T>
using container = typename flatten<basic_container, T...>::type;

// } container 

struct T1{};
struct T2{};
struct T3{};
struct T4{};
struct T5{};
struct T6{};
struct T7{};
struct T8{};
struct T9{};
struct T10{};
struct T11{};
struct T12{};
struct T13{};

int main(){
    typedef container<container<container<container<T1, T2>, T3>, T4>, T5, T6, T7, container<T8, T9, T10>, T11, container<T12, T13>> c;
    
    std::cout << c::base::pretty() << std::endl;
    std::cout << c::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
    
    std::cout << std::endl;
    
    typedef container<c> cf;
    std::cout << cf::pretty() << std::endl;
    
    std::cout << std::endl;
    
    std::cout << eliminate<c, T1>::type::base::pretty() << std::endl;
    std::cout << eliminate<c, T2>::type::base::pretty() << std::endl;
    std::cout << eliminate<c, T3>::type::base::pretty() << std::endl;
    std::cout << eliminate<c, T4>::type::base::pretty() << std::endl;
    std::cout << eliminate<c, T5>::type::base::pretty() << std::endl;
    std::cout << eliminate<c, T6>::type::base::pretty() << std::endl;
    std::cout << eliminate_all<c, T1, T7, T13, T4>::type::base::pretty() << std::endl;
    
    return 0;
}
