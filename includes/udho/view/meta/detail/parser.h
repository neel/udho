#ifndef UDHO_VIEW_META_PARSER_H
#define UDHO_VIEW_META_PARSER_H

#include <map>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <iostream>

namespace udho{
namespace view{
namespace data{
namespace meta{

namespace detail{

namespace pegtl = tao::pegtl;

struct ast{
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
    struct grammar:         pegtl::must<   pegtl::list<  statement, pegtl::seq<spaced<pegtl::opt<pegtl::one<';'>>>>  >   > {};

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

    using node_ptr_type = std::unique_ptr<tao::pegtl::parse_tree::node>;

    inline ast(const std::string& id_str): _str(std::move(id_str)), _input(_str, "input") {
        try{
            _root = pegtl::parse_tree::parse<grammar, selector>(_input);
        } catch (const pegtl::parse_error& e) {
            const auto p = e.positions().front();
            std::cerr << e.what() << '\n' << _input.line_at(p) << std::endl;
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
    pegtl::memory_input<> _input;
    node_ptr_type _root;
};

}

}
}
}
}

#endif // UDHO_VIEW_META_PARSER_H
