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

namespace tmpl{

/**
 * @struct parser
 * @brief Handles the parsing of template files into sections based on predefined tags and constructs.
 *
 * This parser identifies different types of sections such as meta, echo, eval, embed, and more, based on tags defined at construction or via setter methods.
 */
struct parser{
    static constexpr std::uint32_t tag_close           = 401;
    static constexpr std::uint32_t tag_close_comment   = 402;
    static constexpr std::uint32_t tag_close_verbatim  = 403;

    /**
     * @brief Constructs the parser and initializes it with default tags for different sections.
     */
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

    /**
     * @brief Sets the tag for meta sections. Defaults to <?!
     * @param tag String representing the opening tag for meta sections.
     * @return A reference to this parser.
     */
    inline parser& meta(const std::string& tag) { _meta = tag; return *this; }
    /**
     * @brief Retrieves the tag for meta sections.
     * @return The tag used for meta sections.
     */
    inline const std::string& meta() const { return _meta; }

    /**
     * @brief Sets the tag for echo sections. Defaults to <?=.
     * @param tag String representing the opening tag for echo sections.
     * @return A reference to this parser.
     */
    inline parser& echo(const std::string& tag) { _echo = tag; return *this; }
    /**
     * @brief Retrieves the tag for echo sections.
     * @return The tag used for echo sections.
     */
    inline const std::string& echo() const { return _echo; }

    /**
     * @brief Sets the tag for eval sections. Defaults to <?.
     * @param tag String representing the opening tag for eval sections.
     * @return A reference to this parser.
     */
    inline parser& eval(const std::string& tag) { _eval = tag; return *this; }
    /**
     * @brief Retrieves the tag for eval sections.
     * @return The tag used for eval sections.
     */
    inline const std::string& eval() const { return _eval; }

    /**
     * @brief Sets the tag for comment sections. Defaults to <#.
     * @param tag String representing the opening tag for comment sections.
     * @return A reference to this parser.
     */
    inline parser& comment(const std::string& tag) { _comment = tag; return *this; }
    /**
     * @brief Retrieves the tag for comment sections.
     * @return The tag used for comment sections.
     */
    inline const std::string& comment() const { return _comment; }

    /**
     * @brief Sets the tag for verbatim sections. Defaults to <\@.
     * @param tag String representing the opening tag for verbatim sections.
     * @return A reference to this parser.
     */
    inline parser& verbatim(const std::string& tag) { _verbatim = tag; return *this; }
    /**
     * @brief Retrieves the tag for verbatim sections.
     * @return The tag used for verbatim sections.
     */
    inline const std::string& verbatim() const { return _verbatim; }

    /**
     * @brief Sets the tag for embed sections. Defaults to <?:.
     * @param tag String representing the opening tag for embed sections.
     * @return A reference to this parser.
     */
    inline parser& embed(const std::string& tag) { _embed = tag; return *this; }
    /**
     * @brief Retrieves the tag for embed sections.
     * @return The tag used for embed sections.
     */
    inline const std::string& embed() const { return _embed; }

    /**
     * @brief Sets the closing tag for general sections. Defaults to ?>.
     * @param tag String representing the closing tag for general sections.
     * @return A reference to this parser.
     */
    inline parser& close(const std::string& tag) { _close = tag; return *this; }
    /**
     * @brief Retrieves the closing tag for general sections.
     * @return The closing tag used for general sections.
     */
    inline const std::string& close() const { return _close; }

    /**
     * @brief Sets the closing tag for comment sections. Defaults to #>.
     * @param tag String representing the closing tag for comment sections.
     * @return A reference to this parser.
     */
    inline parser& close_comment(const std::string& tag) { _close_comment = tag; return *this; }
    /**
     * @brief Retrieves the closing tag for comment sections.
     * @return The closing tag used for comment sections.
     */
    inline const std::string& close_comment() const { return _close_comment; }

    /**
     * @brief Sets the closing tag for verbatim sections. Defaults to @>.
     * @param tag String representing the closing tag for verbatim sections.
     * @return A reference to this parser.
     */
    inline parser& close_verbatim(const std::string& tag) { _close_verbatim = tag; return *this; }
    /**
     * @brief Retrieves the closing tag for verbatim sections.
     * @return The closing tag used for verbatim sections.
     */
    inline const std::string& close_verbatim() const { return _close_verbatim; }

    /**
     * @brief Parses the given range of input iterators and applies the specified function to each parsed section.
     * @details The parse function uses a trie structure for efficient matching of tags to sections. It distinguishes between different types of sections and invokes the given function with a section object for each detected section.
     * @param begin Iterator pointing to the beginning of the template data.
     * @param end Iterator pointing to the end of the template data.
     * @param fptr Function or functor to call with each parsed section.
     * @tparam InputIt Type of the input iterator.
     * @tparam Function Type of the function or functor.
     */
    template <typename InputIt, typename Function>
    inline void parse(InputIt begin, InputIt end, Function&& fptr) const {
        detail::trie trie = make_trie();

        InputIt last_open = begin, last_close = begin;
        std::uint32_t block_open = 0;

        std::uint32_t last_tag = tmpl::section::text;

        auto pos = begin;
        while(pos != end){
            auto tpos = trie.next(pos, end);
            // auto token_len = trie[tpos.token_id].size();
            pos = tpos.token_end;
            if(pos == end) {
                auto scope_begin = (last_tag == tmpl::section::text) ? last_close : last_open;
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
                        (id == tag_close_comment  && last_tag == tmpl::section::comment)  ||
                        (id == tag_close_verbatim && last_tag == tmpl::section::verbatim)
                    ){
                        fptr(section{type(last_tag), last_open, tpos.token_begin});
                        last_close = pos;
                        last_tag   = tmpl::section::text;
                        block_open = 0;
                    }
                }
            }
        }
    }

    private:
        detail::trie make_trie() const {
            detail::trie trie;
            trie.add(_meta,     tmpl::section::meta);
            trie.add(_echo,     tmpl::section::echo);
            trie.add(_eval,     tmpl::section::eval);
            trie.add(_embed,    tmpl::section::embed);
            trie.add(_verbatim, tmpl::section::verbatim);
            trie.add(_comment,  tmpl::section::comment);

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

