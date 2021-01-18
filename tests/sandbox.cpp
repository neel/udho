#include <iostream>
#include <udho/hazo/seq/proxy.h>
#include <udho/hazo/seq.h>
#include <udho/hazo/map.h>
#include <udho/hazo/map/hana.h>

#include <boost/hana.hpp>
using namespace boost;
using namespace udho::util::hazo;

/**
 * seq<seq<A, B, C>, D, E>
 *  : node<monoid<seq<A, B, C>>::head, seq<monoid<seq<A, B, C>>::rest, D, E>>
 * -> node<A, seq<seq<B, C>, D, E>>
 * 
 * seq<seq<B, C>, D, E>
 *  : node<monoid<seq<B, C>>::head, seq<monoid<seq<B, C>>::rest, D, E>>
 * -> node<B, seq<C, D, E>>
 */

// template <typename H>
// struct monoid_helper{
//     using head = H;
//     template <typename Policy, typename... V>
//     using extend = seq<Policy, V...>;
// };
// 
// template <typename Policy, typename H, typename... T>
// struct monoid_helper<seq<Policy, H, T...>>{
//     using head = typename monoid_helper<H>::head;
//     template <typename UnusedPolicy, typename... V>
//     using extend = typename monoid_helper<H>::template extend<UnusedPolicy, T..., V...>;
//     using rest = extend<Policy>;
// };

// template <typename Policy, typename H, typename T>
// struct monoid_helper<seq<Policy, H, T>>{
//     using head = typename monoid_helper<H>::head;
//     template <typename UnusedPolicy, typename... V>
//     using extend = typename monoid_helper<H>::template extend<Policy, T, V...>;
//     using rest = extend<Policy>;
// };

HAZO_ELEMENT(first_name, std::string)
HAZO_ELEMENT(last_name, std::string)
HAZO_ELEMENT(age, std::size_t)

int main(){
    typedef seq_v<int, std::string, seq_v<double, int>, double, int> seq_v_type;
    
    seq_v_type seq1(1, "Neel", 3.14, 4, 12.45, 5);
    
    std::cout << seq1.value<int, 2>() << std::endl;
    
    std::cout << seq1.accumulate([](auto val, int res = 0){
        std::cout << "("<< val << ")" << " " << res << std::endl;
        return res +1;
    }) << std::endl;
    
    typedef seq_v<char, double> seq_type1;
    typedef seq_v<seq_type1, int> seq_type2;
    typedef seq_v<seq_type2, seq_type2> seq_type3;
    typedef seq_v<seq_type3> seq_type4;

//     seq_type4::types::data_at<0>::xyz();
//     seq_type4::types::data_at<1>::xyz();
//     seq_type4::types::data_at<2>::xyz();
//     seq_type4::types::data_at<3>::xyz();
//     seq_type4::types::data_at<4>::xyz();
//     seq_type4::types::data_at<5>::xyz();
    
    seq_type4 seq4('a', 2.4f, 24, 'b', 4.8f, 48);
    
    std::cout << seq4.accumulate([](auto val, int res = 0){
        std::cout << "("<< val << ")" << " " << res << std::endl;
        return res +1;
    }) << std::endl;
    
    std::cout << seq4.value<char, 0>() << std::endl;
    
    seq4.visit([](auto val){
        std::cout << "("<< val << ")" << std::endl;
    });
    
    return 0;
}

