#ifndef UDHO_VIEW_SHORTCODE_PARSER_H
#define UDHO_VIEW_SHORTCODE_PARSER_H

#include <istream>
#include <memory>
#include <map>
#include <iterator>
#include <boost/iostreams/device/mapped_file.hpp>

namespace udho{
namespace view{

namespace detail{

struct trie_node{
    std::string _past;
    std::map<char, trie_node*> _next;

    bool token() const{
        return _past.size() != 0;
    }
    bool terminal() const {
        return _next.size() == 0;
    }
    bool root() const {
        return _past.size() == 0;
    }
    template <typename InputIt>
    void add(InputIt begin, InputIt pos, InputIt end){
        if(pos != end){
            trie_node* child = 0x0;
            auto it = _next.find(*pos);
            if(it != _next.end()){
                child = it->second;
            }else{
                child = new trie_node;
                _next.insert(std::make_pair(*pos, child));
            }
            child->add(begin, pos+1, end);
        }else{
            _past = std::string(begin, end);
        }
    }
    template <typename InputIt>
    void add(InputIt begin, InputIt end){
        add(begin, begin, end);
    }
    void add(const std::string& str){
        add(str.begin(), str.end());
    }
    /**
     * Forwards iterator till it finds a token end of end of input
     */
    template <typename InputIt>
    std::pair<InputIt, std::string> forward(trie_node* root, std::pair<InputIt, trie_node*> last, InputIt begin, InputIt end){
        if(terminal() && token()){
            return std::make_pair(begin, _past);
        }
        if(begin != end){
            auto it = _next.find(*begin);
            if(it == _next.end()){
                if(token()){
                    return std::make_pair(begin, _past);
                }else if(last.first != begin && last.second != root){
                    return std::make_pair(last.first, last.second->_past);
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
            return std::make_pair(end, "");
        }
    }
    template <typename InputIt>
    std::pair<InputIt, std::string> forward(trie_node* root, InputIt begin, InputIt end){
        return forward(root, std::make_pair(begin, root), begin, end);
    }
};

struct trie{};

}

namespace sections{

struct section {
    enum types { none, root, code, text };

    inline explicit section(types t): _type(t) {}
    inline section(types t, const std::string& content): _type(t), _content(content) {}

    inline types type() const { return _type; }
    inline void content(const std::string& c) { _content = c; }
    inline const std::string& content() const { return _content; }

    private:
        types _type;
        std::string _content;
};

struct parser{
    inline explicit parser() {}
    inline parser& open(const std::string& tag) { _open = tag; return *this; }
    inline const std::string& open() const { return _open; }
    inline parser& close(const std::string& tag) { _close = tag; return *this; }
    inline const std::string& close() const { return _close; }

    template <typename OutputIt>
    inline void parse(const std::string& file, OutputIt out) const {
        std::string buffer;
        boost::iostreams::mapped_file mmap(file, boost::iostreams::mapped_file::readonly);
        auto begin = mmap.const_data();
        auto end   = begin + mmap.size();

    }
    std::string _open, _close;
};

}
}
}

#endif // UDHO_VIEW_SHORTCODE_PARSER_H
