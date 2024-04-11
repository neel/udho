#include <iostream>
#include <udho/view/shortcode_parser.h>
#include <udho/view/scope.h>
#include <udho/view/reflect.h>
#include <udho/hazo/string/basic.h>
#include <stdio.h>
#include <complex>
#include <string.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

template <typename K, typename T>
void const_check(const udho::view::data::nvp<K, T>& nvp){
    std::cout << nvp << std::endl;
}

struct X{
    friend auto reflect(X& x){
        using namespace udho::hazo::string::literals;

        // reflect("name", name);
        // reflect("age", age);
    }

    std::string name;
    std::uint32_t age;
};

static char buffer[] = R"TEMPLATE(
<? codode 1 ?>Once upon a time there was a
time when there was no time at all.<? code 2

?>
A quick brown dinosaur jumped over a lazy unicorn.
)TEMPLATE";

int main(){

    udho::view::detail::trie trie;

    std::string ab = "ab",
                abc = "abc",
                abcdef = "abcdef",
                xycdef = "xycdef";

    trie.add(ab, 101);
    trie.add(abc, 102);
    trie.add(abcdef, 103);
    trie.add(xycdef, 104);

    std::string subject = "hello ab I am a string abcd abcdef and then abcxycdef";
    auto begin = subject.begin();
    auto end = subject.end();

    auto pos = begin;
    while(pos != end){
        auto it = trie.next(pos, end);
        pos = it.first;
        std::cout << trie[it.second] << std::endl;
    }

    std::cout << "hello world" << std::endl;

    FILE* fptr = fmemopen(buffer, strlen (buffer), "r");
    int posix_handle = fileno(fptr);
    boost::iostreams::file_descriptor_source descriptor(posix_handle, boost::iostreams::close_handle);
    boost::iostreams::stream stream(descriptor);

    udho::view::sections::parser parser;
    parser.open("<?").close("?>");



    using namespace udho::hazo::string::literals;
    std::string name = "hello";
    auto name_v = udho::view::data::make_nvp("name", name);
    std::cout << name_v << std::endl;
    const_check(name_v);
    *name_v = "Something Else";
    std::cout << name_v << std::endl;
    const_check(name_v);

    auto ll = udho::view::data::make_nvp("name", "ll");
    auto xyz = *ll;
    std::cout << xyz << std::endl;

    std::uint32_t val = 42;
    auto number_v = udho::view::data::make_nvp("number"_h, 42);
    const_check(number_v);

    std::complex<double> complex;
    std::uint32_t dbl;

    udho::view::data::reflect(dbl);
}
