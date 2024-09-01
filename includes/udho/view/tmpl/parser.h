#ifndef UDHO_VIEW_PARSER_H
#define UDHO_VIEW_PARSER_H

#include <istream>
#include <memory>
#include <map>
#include <cstdint>
#include <iterator>
#include <random>
#include <functional>
#include <algorithm>
#include <udho/view/tmpl/detail/trie.h>
#include <udho/view/tmpl/sections.h>

namespace udho{
namespace view{

namespace sections{

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

    template <typename InputIt, typename Function>
    inline void parse(InputIt begin, InputIt end, Function&& fptr) const {
        detail::trie trie = make_trie();

        InputIt last_open = begin, last_close = begin;
        std::uint32_t block_open = 0;

        std::uint32_t last_tag = sections::section::text;

        auto pos = begin;
        while(pos != end){
            auto tpos = trie.next(pos, end);
            // auto token_len = trie[tpos.token_id].size();
            pos = tpos.token_end;
            if(pos == end) {
                auto scope_begin = (last_tag == sections::section::text) ? last_close : last_open;
                auto scope_end   = (tpos.token_id == 0) ? end : tpos.token_begin;
                fptr(section{type(last_tag), scope_begin, scope_end});
                break;
            }

            std::uint32_t id = tpos.token_id;
            if(id > 100 && id < 300){   // open tag
                if(block_open == 0){
                    auto scope_end = tpos.token_begin;
                    if(std::distance(last_close, scope_end) > 0){
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
                        fptr(section{type(last_tag), last_open, tpos.token_begin});
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

#endif // UDHO_VIEW_PARSER_H

