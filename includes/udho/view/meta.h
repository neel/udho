#ifndef UDHO_VIEW_PARSER_META_H
#define UDHO_VIEW_PARSER_META_H

#include <map>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>

#include <iostream>
#include <chrono>

namespace udho{
namespace view{

namespace sections{

namespace pegtl = tao::pegtl;

namespace meta_parser{
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
        struct values:          pegtl::list< value, spaced<pegtl::one<','>> > {};
        struct call:            pegtl::seq<key, whitespace, pegtl::one<'('>, values, pegtl::one<')'>> {};
        struct lookup:          pegtl::sor<call, key> {};
        struct id;
        struct index:           pegtl::sor<pegtl::seq<pegtl::one<'.'>, id>, index_seq> {};
        struct id:              pegtl::seq<lookup, pegtl::star<index>> {};
        struct grammar:         pegtl::must<
                                    pegtl::list<
                                        id,
                                        pegtl::seq<spaced<pegtl::one<';'>>>
                                    >
                                > {};

    template<typename Rule>
    struct selector:        pegtl::parse_tree::selector<Rule,
        pegtl::parse_tree::store_content::on<
                id,
                lookup,
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
                index,
                grammar
            >
        > {};

    // Helper function to get human-readable names for the node types
    template <typename NodeT>
    static std::string get_node_name(const NodeT& n) {
        if (n->template is_type<lookup>()) return "lookup";
        if (n->template is_type<key>()) return "key";
        if (n->template is_type<call>()) return "call";
        if (n->template is_type<at>()) return "at";
        if (n->template is_type<index_seq>()) return "index_seq";
        if (n->template is_type<index>()) return "index";
        if (n->template is_type<id>()) return "id";
        if (n->template is_type<quoted_string>()) return "quoted_string";
        if (n->template is_type<integer>()) return "integer";
        if (n->template is_type<real>()) return "real";
        if (n->template is_type<duration>()) return "duration";
        if (n->template is_type<boolean>()) return "boolean";
        if (n->template is_type<reference>()) return "reference";
        if (n->template is_type<value>()) return "value";
        if (n->template is_type<values>()) return "values";
        if (n->template is_type<grammar>()) return "grammar";
        return "unknown";
    }

    template <typename NodeT>
    static void print_tree(const NodeT& n, const std::string& indent = "") {
        if (n->is_root()) {
            std::cout << indent << "Root" << std::endl;
        } else {
            std::cout << indent << get_node_name(n) << " : \"" << n->string_view() << "\"" << std::endl;
        }
        for (const auto& child : n->children) {
            print_tree(child, indent + "  ");
        }
    }

    static void parse(const std::string& input) {
        pegtl::memory_input in(input, "input");
        try {
            auto root = pegtl::parse_tree::parse<grammar, selector>(in);
            std::cout << "Parsing succeeded!" << std::endl;

            // Print the parse tree for debugging
            if (root) {
                print_tree(root);
            }
        } catch (const pegtl::parse_error &e) {
            const auto p = e.positions().front();
            std::cerr << "Parse error at line " << p.line << ", column " << p.column << ": " << e.what() << std::endl;
        }
    }
};

}

}
}

#endif // UDHO_VIEW_PARSER_META_H
