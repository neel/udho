#ifndef UDHO_VIEW_SECTIONS_H
#define UDHO_VIEW_SECTIONS_H

#include <istream>
#include <memory>
#include <map>
#include <cstdint>
#include <iterator>
#include <boost/iostreams/device/mapped_file.hpp>
#include <udho/view/trie.h>

namespace udho{
namespace view{

namespace sections{

struct section {
    enum types {
        none,
        root,
        meta,
        echo,
        eval,
        comment,
        embed,
        text
    };

    inline explicit section(types t): _type(t) {}
    inline section(types t, const std::string& content): _type(t), _content(content) {}
    template <typename InputIt>
    inline section(types t, InputIt begin, InputIt end): _type(t), _content(begin, end) {}

    inline types type() const { return _type; }
    inline void content(const std::string& c) { _content = c; }
    inline const std::string& content() const { return _content; }

    friend std::ostream& operator<<(std::ostream& stream, const section& s){
        stream << "section " << s._type << " >" << s._content << "< " << std::endl;
        return stream;
    }

    private:
        types _type;
        std::string _content;
};

struct parser{
    static constexpr std::uint32_t tag_meta    = 101;
    static constexpr std::uint32_t tag_echo    = 102;
    static constexpr std::uint32_t tag_eval    = 103;
    static constexpr std::uint32_t tag_comment = 104;
    static constexpr std::uint32_t tag_embed   = 105;

    static constexpr std::uint32_t tag_close   = 301;
    static constexpr std::uint32_t tag_close_comment = 302;

    inline explicit parser() {
        meta("<?!");
        echo("<?=");
        eval("<?");
        embed("<?:");
        comment("<#");

        comment_close("#>");
        close("?>");
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

    // Setter and getter for _embed
    inline parser& embed(const std::string& tag) { _embed = tag; return *this; }
    inline const std::string& embed() const { return _embed; }

    // Setter and getter for _close
    inline parser& close(const std::string& tag) { _close = tag; return *this; }
    inline const std::string& close() const { return _close; }

    inline parser& comment_close(const std::string& tag) { _comment_close = tag; return *this; }
    inline const std::string& comment_close() const { return _close; }

    template <typename OutputIt>
    inline void parse(const std::string& file, OutputIt out) const {
        std::string buffer;
        boost::iostreams::mapped_file mmap(file, boost::iostreams::mapped_file::readonly);
        auto begin = mmap.const_data();
        auto end   = begin + mmap.size();
        parse(begin, end, out);
    }
    template <typename InputIt, typename OutputIt>
    inline void parse(InputIt begin, InputIt end, OutputIt out) const {
        detail::trie trie;
        trie.add(_meta,    tag_meta);
        trie.add(_echo,    tag_echo);
        trie.add(_eval,    tag_eval);
        trie.add(_embed,   tag_embed);
        trie.add(_comment, tag_comment);

        trie.add(_close,          tag_close);
        trie.add(_comment_close,  tag_close_comment);

        InputIt last_open = begin, last_close = begin;
        std::uint32_t nested_open = 0;

        std::uint32_t last_tag = 0;

        auto pos = begin;
        while(pos != end){
            auto it = trie.next(pos, end);
            pos = it.first;
            if(pos == end) {
                out++ = section{section::text, last_close, end};
                break;
            }

            std::uint32_t id = it.second;
            if(id > 100 && id < 200){   // open tag
                if(nested_open == 0){
                    auto scope_end = pos - trie[it.second].size();
                    if((scope_end - last_close) > 0){
                        out++ = section{section::text, last_close, pos-trie[it.second].size()};
                    }
                    last_open = pos;
                    last_tag  = id;
                }
                ++nested_open;
            }

            if(id > 300 && id < 400){
                if(nested_open > 0){
                    --nested_open;
                }

                if(nested_open == 0){
                    out++ = section{type(last_tag), last_open, pos-trie[it.second].size()};
                    last_close = pos;
                    last_tag   = 0;
                }
            }
        }
    }

    private:
        detail::trie make_trie() const {
            detail::trie trie;
            trie.add(_meta,    tag_meta);
            trie.add(_echo,    tag_echo);
            trie.add(_eval,    tag_eval);
            trie.add(_embed,   tag_embed);
            trie.add(_comment, tag_comment);

            trie.add(_close,          tag_close);
            trie.add(_comment_close,  tag_close_comment);

            return trie;
        }
        section::types type(std::uint32_t id) const{
            switch(id){
                case tag_meta:    return section::meta;
                case tag_echo:    return section::echo;
                case tag_eval:    return section::eval;
                case tag_embed:   return section::embed;
                case tag_comment: return section::comment;
            }
            return section::none;
        }
    private:
        std::string _meta, _echo, _eval, _comment, _embed, _close, _comment_close;
};

}
}
}

#endif // UDHO_VIEW_SHORTCODE_PARSER_H
