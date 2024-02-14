#include <iostream>
#include <udho/view/shortcode_parser.h>
#include <stdio.h>
#include <string.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

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

}
