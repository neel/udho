#ifndef UDHO_VIEW_META_PARSER_H
#define UDHO_VIEW_META_PARSER_H

/**
 * @addtogroup DoxyG_view_tmpl_data
 * @{
 */

#include <map>
#include <string>
#include <tao/pegtl.hpp>
#if TAO_PEGTL_VERSION_MAJOR >= 4
#include <tao/pegtl/extra/parse_tree.hpp>
#else
#include <tao/pegtl/contrib/parse_tree.hpp>
#endif
#include <udho/utils/format.h>
#include <iostream>
#include <udho/logging/macros.h>

namespace udho{
namespace view{
namespace data{
namespace meta{

namespace detail{

namespace pegtl = tao::pegtl;

/**
 * @brief abstract syntax tree representation for the meta syntax
 *
 * Grammar
 * ------------
 * whitespace      ::= *space
 * boolean         ::= 'true' | 'false' | 'on' | 'off'
 * integer         ::= [minus], +digit
 * real            ::= [minus], +digit, '.', +digit
 * duration        ::= (real | integer), ('s' | 'm' | 'h' | 'd')
 * quoted_string   ::= dquoted_string | squoted_string
 * dquoted_string  ::= '"', {any character except '"'}, '"'
 * squoted_string  ::= ''', {any character except ''''}, '''
 * value           ::= quoted_string | reference | duration | real | integer | boolean
 * grammar         ::= whitespace, {statement, {whitespace, [semicolon], whitespace}}
 * statement       ::= key, *index
 * key             ::= alpha, *(alnum | '_')
 * index           ::= ('.' key) | index_seq | call
 * index_seq       ::= '[', at, ']'
 * at              ::= +digit
 * call            ::= whitespace, '(', values, ')'
 * values          ::= value, {whitespace, ',', whitespace, value}
 *
 * Examples
 * ---------
 * Following are all valid syntax according to the above mentioned grammar
 *
 * ```
 * x.y.z_a('v1', 'v_2',:keyword);
 * hello[24]("world", "pluto");
 * hello.hi[23]('pla_net')
 * feature.value[1].bit(false);
 * cache.expire(-42.24)[0];
 * ```
 *
 * Usage
 * ------
 *
 * This class is not supposed to be used directly. Rather though the functions in `udho::view::data::meta` namespace.
 * However, following example parses the syntax and prints the AST
 *
 * @code
 * std::string input = "..."; // syntax as mentioned above
 * udho::view::data::meta::detail::ast ast{input};
 * udho::view::data::meta::detail::ast::print(std::cout, ast.root());
 * const udho::view::data::meta::detail::ast::node_ptr_type& root = ast.root();
 * @endcode
 */
struct ast{
    #if TAO_PEGTL_VERSION_MAJOR >= 4
        using input_type = pegtl::text_view_input<pegtl::default_eol, char, std::string>;

        // Provide the PEGTL 3.x node interface for the existing visitor code.
        struct node : pegtl::parse_tree::basic_node<node, input_type::error_position_t> {
            bool has_content() const noexcept {
                return this->data.data() != nullptr;
            }

            std::string_view string_view() const noexcept {
                assert(has_content());
                return this->data;
            }
        };
    #else
        using input_type = pegtl::memory_input<>;
        using node = pegtl::parse_tree::node;
    #endif
    using node_ptr_type = std::unique_ptr<node>;

    struct at:              pegtl::plus<pegtl::digit> {};
    struct index_seq:       pegtl::seq<pegtl::one<'['>, at, pegtl::one<']'>> {};
    struct key:             pegtl::seq<pegtl::alpha, pegtl::star<pegtl::sor<pegtl::alnum, pegtl::one<'_'>>>> {};
    struct dquoted_string:  pegtl::seq<pegtl::one<'"'>, pegtl::until<pegtl::one<'"'>>> {};
    struct squoted_string:  pegtl::seq<pegtl::one<'\''>, pegtl::until<pegtl::one<'\''>>> {};
    struct quoted_string:   pegtl::sor<dquoted_string, squoted_string> {};
    struct reference:       pegtl::seq<pegtl::one<':'>, key> {};
    struct integer:         pegtl::seq<pegtl::opt<pegtl::one<'-'>>, pegtl::plus<pegtl::digit>> {};
    struct real:            pegtl::seq<pegtl::opt<pegtl::one<'-'>>, pegtl::plus<pegtl::digit>, pegtl::one<'.'>, pegtl::plus<pegtl::digit>> {};
    struct duration:        pegtl::seq<pegtl::sor<real, integer>, pegtl::one<'s', 'm', 'h', 'd'>> {};
    struct boolean:         pegtl::sor<TAO_PEGTL_ISTRING("true"), TAO_PEGTL_ISTRING("false"), TAO_PEGTL_ISTRING("on"), TAO_PEGTL_ISTRING("off")> {};
    struct value:           pegtl::sor<quoted_string, reference, duration, real, integer, boolean> {};
    struct whitespace:      pegtl::star<pegtl::space> {};
    template <typename Rule>
    using spaced =          pegtl::seq<whitespace, Rule, whitespace>;
    struct values:          pegtl::list<value, spaced<pegtl::one<','>> > {};
    struct call:            pegtl::seq<whitespace, pegtl::one<'('>, values, pegtl::one<')'>> {};
    struct statement;
    struct index:           pegtl::sor<
                                pegtl::seq<pegtl::one<'.'>, key>,
                                index_seq,
                                call
                            > {};
    struct statement:       pegtl::seq<key, pegtl::star<index>> {};
    struct grammar:         pegtl::must<   pegtl::seq<whitespace, pegtl::list<  statement, pegtl::seq<spaced<pegtl::opt<pegtl::one<';'>>>>  >>   > {};

    template<typename Rule>
    struct selector:        pegtl::parse_tree::selector<Rule,
        pegtl::parse_tree::store_content::on<
                grammar,
                statement,
                key,
                call,
                values,
                quoted_string,
                integer,
                real,
                duration,
                boolean,
                reference,
                at,
                index
            >
        > {};

    inline ast(const std::string& id_str)
        : _str(id_str)
#if TAO_PEGTL_VERSION_MAJOR >= 4
        , _input("input", _str)
#else
        , _input(_str, "input")
#endif
    {
        try {
            _root = pegtl::parse_tree::parse<grammar, node, selector>(_input);
        }
#if TAO_PEGTL_VERSION_MAJOR >= 4
        catch (const input_type::parse_error_t& e) {
           const auto& pos = e.position_object();
#else
        catch (const pegtl::parse_error& e) {
            const auto& pos = e.positions().front();
#endif
            namespace p = udho::logging::params;
            UDHO_LOG_ERROR("udho::view::data::meta::parser", udho::utils::format("Failed to parse view metadata at line {}, column {}", pos.line, pos.column), p::exception(e.what()));
        }
    }

    const node_ptr_type& root() const { return _root; }

    inline static std::string get_node_name(const node_ptr_type& n) {
        if (n->template is_type<grammar>())         return "grammar";
        if (n->template is_type<key>())             return "key";
        if (n->template is_type<call>())            return "call";
        if (n->template is_type<at>())              return "at";
        if (n->template is_type<index_seq>())       return "index_seq";
        if (n->template is_type<index>())           return "index";
        if (n->template is_type<statement>())       return "statement";
        if (n->template is_type<quoted_string>())   return "quoted_string";
        if (n->template is_type<integer>())         return "integer";
        if (n->template is_type<real>())            return "real";
        if (n->template is_type<duration>())        return "duration";
        if (n->template is_type<boolean>())         return "boolean";
        if (n->template is_type<reference>())       return "reference";
        if (n->template is_type<value>())           return "value";
        if (n->template is_type<values>())          return "values";
        return "unknown";
    }

    inline static void print(std::ostream& stream, const node_ptr_type& n, const std::string& indent = "") {
        if (n->is_root()) {
            stream << indent << "Root" << std::endl;
        } else {
            stream << indent << get_node_name(n) << " : \"" << n->string_view() << "\"" << std::endl;
        }

        for (const auto& child : n->children) {
            print(stream, child, indent + "  ");
        }
    }

    std::string _str;
    input_type _input;
    node_ptr_type _root;
};

}

}
}
}
}

/** @} */

#endif // UDHO_VIEW_META_PARSER_H
