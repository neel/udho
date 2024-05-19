#ifndef UDHO_VIEW_SECTIONS_H
#define UDHO_VIEW_SECTIONS_H

#include <istream>
#include <memory>
#include <map>
#include <cstdint>
#include <iterator>
#include <random>
#include <functional>
#include <algorithm>
#include <boost/iostreams/device/mapped_file.hpp>
#include <udho/view/trie.h>
#include <boost/algorithm/string/trim_all.hpp>

namespace udho{
namespace view{

namespace sections{

struct section {
    enum types {
        none     = 0,
        root     = 100,
        meta     = 101,
        echo     = 102,
        eval     = 103,
        embed    = 104,

        comment  = 201,
        verbatim = 202,

        text     = 300
    };

    inline explicit section(types t): _type(t) {}
    inline section(types t, const std::string& content): _type(t), _content(content) {
        if(_type != text && _type != verbatim){
            boost::algorithm::trim_all(_content);
        }
    }
    template <typename InputIt>
    inline section(types t, InputIt begin, InputIt end): _type(t), _content(begin, end) {
        if(_type != text && _type != verbatim){
            boost::algorithm::trim_all(_content);
        }
    }
    inline types type() const { return _type; }
    inline void content(const std::string& c) { _content = c; }
    inline const std::string& content() const { return _content; }
    inline const std::size_t size() const { return _content.size(); }
    inline static std::string name(const section::types type){
        switch(type){
            case section::meta:     return "meta";
            case section::echo:     return "echo";
            case section::eval:     return "eval";
            case section::embed:    return "embed";
            case section::comment:  return "comment";
            case section::text:     return "text";
            case section::verbatim: return "verbatim";
            case section::root:     return "root";
            case section::none:     return "none";
        }
        return "none";
    }

    friend std::ostream& operator<<(std::ostream& stream, const section& s){
        stream << "section " << section::name(s._type) << " >" << s._content << "< " << std::endl;
        return stream;
    }

    private:
        std::string random_string(std::size_t len) {
            static constexpr const char charset[] = {   '0','1','2','3','4',
                                                        '5','6','7','8','9',
                                                        'A','B','C','D','E','F',
                                                        'G','H','I','J','K',
                                                        'L','M','N','O','P',
                                                        'Q','R','S','T','U',
                                                        'V','W','X','Y','Z',
                                                        'a','b','c','d','e','f',
                                                        'g','h','i','j','k',
                                                        'l','m','n','o','p',
                                                        'q','r','s','t','u',
                                                        'v','w','x','y','z'
                                                    };

            static std::default_random_engine rng(std::random_device{}());
            static std::uniform_int_distribution<> dist(0, sizeof(charset)-1);
            static auto lambda = []() -> char{ return charset[dist(rng)]; };

            std::string str(len, 0);
            std::generate_n(str.begin(), len, lambda);
            return str;
        }
    private:
        types _type;
        std::string _content;
};

struct parser{
    static constexpr std::uint32_t tag_close           = 401;
    static constexpr std::uint32_t tag_close_comment   = 402;
    static constexpr std::uint32_t tag_close_verbatim  = 403;

    inline explicit parser() {
        meta("<?!");
        echo("<?=");
        eval("<?");
        embed("<?:");

        verbatim("<@");
        comment("<#");

        close("?>");
        close_comment("#>");
        close_verbatim("@>");
    }

    // Setter and getter for _meta
    inline parser& meta(const std::string& tag) { _meta = tag; return *this; }
    inline const std::string& meta() const { return _meta; }

    // Setter and getter for _echo
    inline parser& echo(const std::string& tag) { _echo = tag; return *this; }
    inline const std::string& echo() const { return _echo; }

    // Setter and getter for _eval
    inline parser& eval(const std::string& tag) { _eval = tag; return *this; }
    inline const std::string& eval() const { return _eval; }

    // Setter and getter for _comment
    inline parser& comment(const std::string& tag) { _comment = tag; return *this; }
    inline const std::string& comment() const { return _comment; }

    inline parser& verbatim(const std::string& tag) { _verbatim = tag; return *this; }
    inline const std::string& verbatim() const { return _verbatim; }

    // Setter and getter for _embed
    inline parser& embed(const std::string& tag) { _embed = tag; return *this; }
    inline const std::string& embed() const { return _embed; }

    // Setter and getter for _close
    inline parser& close(const std::string& tag) { _close = tag; return *this; }
    inline const std::string& close() const { return _close; }

    inline parser& close_comment(const std::string& tag) { _close_comment = tag; return *this; }
    inline const std::string& close_comment() const { return _close_comment; }

    inline parser& close_verbatim(const std::string& tag) { _close_verbatim = tag; return *this; }
    inline const std::string& close_verbatim() const { return _close_verbatim; }

    template <typename Function>
    inline void parse(const std::string& file, Function&& fptr) const {
        std::string buffer;
        boost::iostreams::mapped_file mmap(file, boost::iostreams::mapped_file::readonly);
        auto begin = mmap.const_data();
        auto end   = begin + mmap.size();
        parse(begin, end, std::forward<Function>(fptr));
    }

    template <typename InputIt, typename Function>
    inline void parse(InputIt begin, InputIt end, Function&& fptr) const {
        detail::trie trie = make_trie();

        InputIt last_open = begin, last_close = begin;
        std::uint32_t block_open = 0;

        std::uint32_t last_tag = sections::section::text;

        auto pos = begin;
        while(pos != end){
            auto it = trie.next(pos, end);
            pos = it.first;
            if(pos == end) {
                auto scope_begin = (last_tag == sections::section::text) ? last_close : last_open;
                auto scope_end   = (it.second == 0) ? end : (pos-trie[it.second].size());
                fptr(section{type(last_tag), scope_begin, scope_end});
                break;
            }

            std::uint32_t id = it.second;
            if(id > 100 && id < 300){   // open tag
                if(block_open == 0){
                    auto scope_end = pos - trie[it.second].size();
                    if((scope_end - last_close) > 0){
                        fptr(section{section::text, last_close, scope_end});
                    }
                    last_open = pos;
                    last_tag  = id;
                    block_open = 1;
                }
            }

            if(id > 400 && id < 500){
                if(block_open == 1){
                    if (
                        (id == tag_close          && (last_tag > 100 && last_tag  < 200))     ||
                        (id == tag_close_comment  && last_tag == sections::section::comment)  ||
                        (id == tag_close_verbatim && last_tag == sections::section::verbatim)
                    ){
                        fptr(section{type(last_tag), last_open, pos-trie[it.second].size()});
                        last_close = pos;
                        last_tag   = sections::section::text;
                        block_open = 0;
                    }
                }
            }
        }
    }

    private:
        detail::trie make_trie() const {
            detail::trie trie;
            trie.add(_meta,     sections::section::meta);
            trie.add(_echo,     sections::section::echo);
            trie.add(_eval,     sections::section::eval);
            trie.add(_embed,    sections::section::embed);
            trie.add(_verbatim, sections::section::verbatim);
            trie.add(_comment,  sections::section::comment);

            trie.add(_close,            tag_close);
            trie.add(_close_comment,    tag_close_comment);
            trie.add(_close_verbatim,   tag_close_verbatim);

            return trie;
        }
        section::types type(std::uint32_t id) const{
            return static_cast<section::types>(id);
        }
    private:
        std::string _meta, _echo, _eval, _comment, _verbatim, _embed;
        std::string _close, _close_comment, _close_verbatim;
};

}
}
}

#endif // UDHO_VIEW_SHORTCODE_PARSER_H
