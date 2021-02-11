#include <typeinfo>
#include <iostream>

template <typename H, typename... T>
struct container;

template <typename ContainerT, typename U>
struct eliminate;

template <typename ContainerT, typename... T>
struct extend;

template <typename H, typename... T>
struct first_of{
    using type = H;
};

template <template <typename...> class ContainerT, typename H, typename... T>
struct rest_of{
    using type = ContainerT<T...>;
};

template <template <typename...> class ContainerT, typename... H, typename... T>
struct rest_of<ContainerT, ContainerT<H...>, T...>{
    using type = typename rest_of<ContainerT, H..., T...>::type;
};

// monoid {

template <template <typename...> class ContainerT, typename... X>
struct monoid{
    using first = typename first_of<X...>::type;
    using rest  = typename rest_of<ContainerT, X...>::type;
};

// } monoid

// container {

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
struct container{
    using mono = monoid<container, H, T...>;
    using head = typename mono::first;
    using tail = typename mono::rest;
    using base = node<head, tail>;
    
    static const char* pretty(){
        return __PRETTY_FUNCTION__;
    }
};

// monoid {{

template <typename... H, typename... T>
struct first_of<container<H...>, T...>{
    using type = typename first_of<H...>::type;
};

// }} monoid

// } container 

// { operations

template <typename... X, typename... T>
struct extend<container<X...>, T...>{
    using type = container<X..., T...>;
};

template <typename... T>
struct extend<container<void>, T...>{
    using type = container<T...>;
};

template <typename H, typename... X, typename U>
struct eliminate<container<H, X...>, U>{
    enum { 
        matched = std::is_same<H, U>::value
    };
    using tail = container<X...>;
    using type = typename std::conditional<matched, 
        tail,
        typename extend<typename eliminate<tail, U>::type, H>::type
    >::type;
};

template <typename H, typename U>
struct eliminate<container<H>, U>{
    enum { 
        matched = std::is_same<H, U>::value
    };
    using type = typename std::conditional<matched, 
        container<void>,
        container<H>
    >::type;
};

// } operations 

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
//     
//     std::cout << c::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     std::cout << c::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::tail::base::pretty() << std::endl;
//     
//     std::cout << std::endl;
//     
//     std::cout << eliminate<c, T1>::type::base::pretty() << std::endl;
//     std::cout << eliminate<c, T2>::type::base::pretty() << std::endl;
//     std::cout << eliminate<c, T3>::type::base::pretty() << std::endl;
//     std::cout << eliminate<c, T4>::type::base::pretty() << std::endl;
//     std::cout << eliminate<c, T5>::type::base::pretty() << std::endl;
//     std::cout << eliminate<c, T6>::type::base::pretty() << std::endl;
//     std::cout << eliminate<c, T7>::type::base::pretty() << std::endl;
    
    return 0;
}
