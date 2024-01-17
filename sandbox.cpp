#include <iostream>
#include <udho/view/shortcode_parser.h>

int main(){

    udho::view::detail::trie_node* node = new udho::view::detail::trie_node;

    std::string ab = "ab",
                abc = "abc",
                abcdef = "abcdef",
                xycdef = "xycdef";

    node->add(ab);
    node->add(abc);
    node->add(abcdef);
    node->add(xycdef);

    std::string subject = "hello ab I am a string abcd abcdef and then abcxycdef";
    auto begin = subject.begin();
    auto end = subject.end();

    auto pos = begin;
    while(pos != end){
        auto it = node->forward(node, pos, end);
        pos = it.first;
        std::cout << it.second << std::endl;
    }

    std::cout << "hello world" << std::endl;
}
