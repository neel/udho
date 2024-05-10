#include <iostream>
#include <udho/view/trie.h>
#include <udho/view/sections.h>
#include <udho/view/scope.h>
#include <udho/view/resources.h>
#include <udho/view/bridges.h>
#include <udho/hazo/string/basic.h>
#include <stdio.h>
#include <complex>
#include <string.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <udho/view/resources.h>

struct info{
    std::string name;
    double      value;
    std::uint32_t _x;

    inline double x() const { return _x; }
    inline void setx(const std::uint32_t& v) { _x = v; }

    inline info() {
        name = "Hello";
        value = 42;
        _x = 43;
    }

    void print(){
        std::cout << "name: " << name << " value: " << value  << std::endl;
    }

    friend auto prototype(udho::view::data::type<info>){
        using namespace udho::view::data;

        return assoc(
            make_nvp(policies::property<policies::writable>{}, "name",  &info::name),
            make_nvp(policies::property<policies::readonly>{}, "value", &info::value),
            make_nvp(policies::property<policies::functional>{}, "x", &info::x, &info::setx),
            make_nvp(policies::function{}, "print", &info::print)
        ).as("info");
    }
};

static char buffer[] = R"TEMPLATE(
<?! register "views.user.badge"; lang "lua" ?>

<? d = udho.view() ?>

Hello <?= d.world ?>

<?:score udho.view() ?>

<# Some comments that will be ignored #>

<@ verbatim block @>

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

    // FILE* fptr = fmemopen(buffer, strlen (buffer), "r");
    // int posix_handle = fileno(fptr);
    // boost::iostreams::file_descriptor_source descriptor(posix_handle, boost::iostreams::close_handle);
    // boost::iostreams::stream stream(descriptor);

    std::vector<udho::view::sections::section> sections;
    udho::view::sections::parser parser;
    parser.parse(buffer, buffer+sizeof(buffer), std::back_inserter(sections));
    for(const udho::view::sections::section& section: sections){
        std::cout  << section << std::endl;
    }


    udho::view::data::bridges::lua lua;
    lua.init();
    lua.bind(udho::view::data::type<info>{});
    // lua.shell();

    udho::view::data::bridges::chai chai;
    chai.init();
    chai.bind(udho::view::data::type<info>{});
    // lua.shell();

    udho::view::data::resources resources;
}
