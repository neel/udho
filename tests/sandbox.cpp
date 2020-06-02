#include <set>
#include <map>
#include <stack>
#include <udho/scope.h>
#include <udho/access.h>
#include <udho/parser.h>
#include <udho/configuration.h>
#include <boost/filesystem.hpp>

std::string longest_common_prefix(const std::string& left, const std::string& right){
    std::string::const_iterator lit = left.cbegin(), rit = right.cbegin();
    std::string common;
    while(lit != left.cend() && rit != right.cend()){
        if(*lit == *rit){
            common.push_back(*lit);
        }else{
            break;
        }
        ++lit;
        ++rit;
    }
    return common;
}

int main(){
    boost::filesystem::path base = "/home/neel/Projects/";
    boost::filesystem::path sub  = "/udho/examples/www/udho.png";
    boost::filesystem::path sub_bad  = "../udho/examples/www/udho.png";
    boost::system::error_code ec;
    boost::filesystem::path canonical_base = boost::filesystem::canonical(base);
    boost::filesystem::path path = canonical_base / sub.make_preferred();
    boost::filesystem::path path_bad = canonical_base / sub_bad.make_preferred();
    boost::filesystem::path canonical_path = boost::filesystem::weakly_canonical(path);
    boost::filesystem::path canonical_path_bad = boost::filesystem::weakly_canonical(path_bad);

    std::cout << ec << std::endl;
    std::cout << longest_common_prefix(canonical_path.string(), canonical_base.string()) << std::endl;
    std::cout << longest_common_prefix(canonical_path_bad.string(), canonical_base.string()) << std::endl;
    std::cout << canonical_path << std::endl;
    std::cout << canonical_path_bad << std::endl;
    
    return 0;
}

