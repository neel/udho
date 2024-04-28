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

int foo(){ return 42; }

struct X{
    friend auto reflect(X& x){
        using namespace udho::hazo::string::literals;

        // reflect("name", name);
        // reflect("age", age);
    }

    std::string name;
    std::uint32_t age;
};

template <typename T>
void bar(T&& v){
    // T::x;
    auto flags = std::cout.flags();
    std::cout << std::boolalpha << v << std::endl;
    std::cout << "is_plain_v:    " << udho::view::data::traits::is_plain_v<T> << std::endl;
    std::cout << "is_string_v:   " << udho::view::data::traits::is_string_v<T> << std::endl;
    std::cout << "is_linked_v:   " << udho::view::data::traits::is_linked_v<T> << std::endl;
    std::cout << "is_function_v: " << udho::view::data::traits::is_function_v<T> << std::endl;
    std::cout << "is_mutable_v:  " << udho::view::data::traits::is_mutable_v<T> << std::endl;
    std::cout.flags(flags);
}

template <typename T>
void baz(T&& w){
    std::cout << "assignable " << w.assignable << std::endl;
    std::cout << "getter     " << w.getter << std::endl;
    std::cout << "setter     " << w.setter << std::endl;
}

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



    // using namespace udho::hazo::string::literals;
    // std::string name = "hello";
    // auto name_v = udho::view::data::make_nvp("name", name);
    // std::cout << name_v << std::endl;
    // const_check(name_v);
    // *name_v = "Something Else";
    // std::cout << name_v << std::endl;
    // const_check(name_v);
    //
    // auto ll = udho::view::data::make_nvp("name", "ll");
    // auto xyz = *ll;
    // std::cout << xyz << std::endl;
    //
    std::uint32_t val = 42;
    // auto number_v = udho::view::data::make_nvp("number"_h, 42);
    // const_check(number_v);
    //
    // std::complex<double> complex;
    std::uint32_t dbl;
    //
    // udho::view::data::reflect(dbl);
    //
    // auto assoc = udho::view::data::associative(
    //     udho::view::data::make_nvp("one", 1),
    //     udho::view::data::make_nvp("two", 2),
    //     udho::view::data::make_nvp("tree", 3)
    // );
    //
    // std::cout << assoc.apply([](auto nvp){
    //     std::cout << nvp << std::endl;
    // }) << std::endl;
    //
    //
    // auto assoc_singular = udho::view::data::associative(
    //     udho::view::data::make_nvp("one", 10),
    //     udho::view::data::make_nvp("two", &foo)
    // );
    //
    // std::cout << assoc_singular.apply([](auto nvp){
    //     std::cout << nvp << std::endl;
    // }) << std::endl;
    //
    const char* str = "hello";
    auto astr = "hello";
    //
    // bar(43);
    // bar("hello");
    // bar(astr);
    // bar(str);
    // bar(val);
    //
    // // std::cout << std::is_const_v<std::remove_pointer_t<std::remove_reference_t<const char*&>>> << std::endl;
    //
    // baz(udho::view::data::wrap(str));

    using namespace udho::view::data;

    wrap(str);
    make_nvp(policies::property{}, "name", str);

    assoc(
        make_nvp(policies::property{}, "name", str),
        make_nvp(policies::property{}, "name", astr),
        make_nvp(policies::property{}, "number", 42),
        make_nvp(policies::property{}, "value", val)
    );
}
