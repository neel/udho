#include <iostream>
#include <udho/view/shortcode_parser.h>

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
}
