#ifndef UDHO_VIEW_TRIE_H
#define UDHO_VIEW_TRIE_H

#include <istream>
#include <memory>
#include <map>
#include <cstdint>
#include <iterator>


namespace udho{
namespace view{

namespace detail{

struct trie_node{
    template <typename IteratorT>
    struct token_pos{
        IteratorT token_end;
        IteratorT token_begin;
        std::uint32_t token_id;

        token_pos(IteratorT end, IteratorT begin, std::uint32_t id): token_end(end), token_begin(begin), token_id(id) {}
    };

    inline explicit trie_node(): _id(0) {}
    inline ~trie_node() {
        for(auto& p: _next){
            delete p.second;
            p.second = 0;
        }
    }

    inline bool token() const{ return _id != 0; }
    inline bool terminal() const { return _next.size() == 0; }
    template <typename InputIt>
    void add(InputIt begin, InputIt pos, InputIt end, std::uint32_t id){
        if(pos != end){
            trie_node* child = 0x0;
            auto it = _next.find(*pos);
            if(it != _next.end()){
                child = it->second;
            }else{
                child = new trie_node;
                _next.insert(std::make_pair(*pos, child));
            }
            child->add(begin, pos+1, end, id);
        }else{
            _id = id;
        }
    }
    template <typename InputIt>
    void add(InputIt begin, InputIt end, std::uint32_t id){
        add(begin, begin, end, id);
    }
    template <typename CharT>
    void add(const std::basic_string<CharT>& str, std::uint32_t id){
        add(str.begin(), str.end(), id);
    }
    /**
     * Forwards iterator till it finds a token end of end of input
     */
    template <typename InputIt>
    token_pos<InputIt> forward(trie_node* root, std::pair<InputIt, trie_node*> last, InputIt begin, InputIt end, InputIt token_begin){
        using tpos = token_pos<InputIt>;
        if(terminal() && token()){
            return tpos(begin, token_begin, _id);
        }
        if(begin != end){
            // char current_char = *begin;
            auto it = _next.find(*begin);
            if(it == _next.end()){                                          // end of a token
                auto tbegin = token_begin;
                token_begin = begin;
                if(token()){                                                // landed on a token node
                    return tpos(begin, tbegin, _id);                        // return token_end position and the token_id
                }else if(last.first != begin && last.second != root){       // landed on a non-token node, But previously landed on a token node
                    return tpos(last.first, tbegin, last.second->_id);      // return the previous token's token_end and token_id
                }
                auto b = begin;
                std::advance(b, 1);
                return root->forward(root, b, end);                         // unfinished token, start following from the root
            }else{                                                          // begin or intermediate stage of a token
                auto b = begin;
                std::advance(b, 1);                                         // advance the iterator
                if(token()){                                                // landed on token node but there is more to follow
                    return it->second->forward(root, std::make_pair(begin, this), b, end, token_begin); // follow while remembering the last captured node
                }else{
                    return it->second->forward(root, last, b, end, token_begin);         // follow next
                }
            }
        }else{
            return tpos(end, end, 0);
        }
    }
    template <typename InputIt>
    token_pos<InputIt> forward(trie_node* root, InputIt begin, InputIt end){
        return forward(root, std::make_pair(begin, root), begin, end, begin);
    }
    inline std::size_t count() const {
        std::size_t sum = token() ? 1 : 0;
        for(const auto& p: _next){
            sum += p.second->count();
        }
        return sum;
    }
    private:
        std::uint32_t _id;
        std::map<char, trie_node*> _next;
};

class trie{
    trie_node _root;
    std::map<std::uint32_t, std::string> _tokens;
    public:
        inline void add(const std::string& token, std::uint32_t id){
            _root.add(token, id);
            _tokens[id] = token;
        }
        template <typename InputIt>
        trie_node::token_pos<InputIt> next(InputIt begin, InputIt end){
            return _root.forward(&_root, begin, end);
        }
        inline const std::string& at(std::uint32_t id) const { return _tokens.at(id); }
        inline const std::size_t size(std::uint32_t id) const { return at(id).size(); }
        inline const std::string& operator[](std::uint32_t id) const { return at(id); }
        inline const std::size_t size() const { return _tokens.size(); }
};

}
}
}

#endif // UDHO_VIEW_TRIE_H
