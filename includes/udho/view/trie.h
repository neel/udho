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
    std::pair<InputIt, std::uint32_t> forward(trie_node* root, std::pair<InputIt, trie_node*> last, InputIt begin, InputIt end){
        if(terminal() && token()){
            return std::make_pair(begin, _id);
        }
        if(begin != end){
            auto it = _next.find(*begin);
            if(it == _next.end()){
                if(token()){
                    return std::make_pair(begin, _id);
                }else if(last.first != begin && last.second != root){
                    return std::make_pair(last.first, last.second->_id);
                }
                return root->forward(root, begin+1, end);
            }else{
                if(token()){
                    return it->second->forward(root, std::make_pair(begin, this), begin+1, end);
                }else{
                    return it->second->forward(root, last, begin+1, end);
                }
            }
        }else{
            return std::make_pair(end, 0);
        }
    }
    template <typename InputIt>
    std::pair<InputIt, std::uint32_t> forward(trie_node* root, InputIt begin, InputIt end){
        return forward(root, std::make_pair(begin, root), begin, end);
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
        std::pair<InputIt, std::uint32_t> next(InputIt begin, InputIt end){
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
