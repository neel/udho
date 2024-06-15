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
    struct whitespace:      pegtl::star<pegtl::space> {};
    template <typename Rule>
    using spaced = pegtl::seq<whitespace, Rule, whitespace>;

    struct key:             pegtl::seq<pegtl::alpha, pegtl::star<pegtl::sor<pegtl::alnum, pegtl::one<'.', '_'>>>> {};
    struct dquoted_string:  pegtl::seq<pegtl::one<'"'>, pegtl::until<pegtl::one<'"'>>> {};
    struct squoted_string:  pegtl::seq<pegtl::one<'\''>, pegtl::until<pegtl::one<'\''>>> {};
    struct quoted_string:   pegtl::sor<dquoted_string, squoted_string> {};
    struct reference:       pegtl::seq<pegtl::one<':'>, key> {};
    struct integer:         pegtl::seq<pegtl::opt<pegtl::one<'-'>>, pegtl::plus<pegtl::digit>> {};
    struct real:            pegtl::seq<pegtl::opt<pegtl::one<'-'>>, pegtl::plus<pegtl::digit>, pegtl::one<'.'>, pegtl::plus<pegtl::digit>> {};
    struct duration:        pegtl::seq<pegtl::sor<real, integer>, pegtl::one<'s', 'm', 'h', 'd'>> {};
    struct boolean:         pegtl::sor<TAO_PEGTL_ISTRING("true"), TAO_PEGTL_ISTRING("false"), TAO_PEGTL_ISTRING("on"), TAO_PEGTL_ISTRING("off")> {};
    struct unquoted_string: pegtl::plus<
                                    pegtl::not_one<'(', ')', ',', '\'', '"', ';', ' ', '\n', '\r', '\t', '\v', '\f'>
                            >{};
    struct value:           pegtl::sor<quoted_string, reference, duration, real, integer, boolean, unquoted_string> {};

    struct values:          pegtl::list<
                                value,
                                spaced<pegtl::one<','>>
                            > {};
    struct statement:       pegtl::seq<
                                key,
                                spaced<pegtl::one<'('>>, values, spaced<pegtl::one<')'>>
                            > {};
    struct grammar:         pegtl::must<
                                pegtl::list<
                                    statement,
                                    pegtl::seq<spaced<pegtl::one<';'>>>
                                >
                            > {};

    // Define which rules should create nodes in the parse tree
    template<typename Rule>
    struct selector : pegtl::parse_tree::selector<
        Rule,
        pegtl::parse_tree::store_content::on<
            key,
            quoted_string,
            unquoted_string,
            integer,
            real,
            duration,
            boolean,
            reference,
            statement
        >> {};

    // Helper function to get human-readable names for the node types
    template <typename NodeT>
    static std::string get_node_name(const NodeT& n) {
        if (n->template is_type<key>()) return "key";
        if (n->template is_type<quoted_string>()) return "quoted_string";
        if (n->template is_type<unquoted_string>()) return "unquoted_string";
        if (n->template is_type<integer>()) return "integer";
        if (n->template is_type<real>()) return "real";
        if (n->template is_type<duration>()) return "duration";
        if (n->template is_type<boolean>()) return "boolean";
        if (n->template is_type<reference>()) return "reference";
        if (n->template is_type<value>()) return "value";
        if (n->template is_type<values>()) return "values";
        if (n->template is_type<statement>()) return "statement";
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

    enum class datatype {
        string, integer, real, duration, boolean
    };
    template <datatype Type> struct cpp_type;
    template <> struct cpp_type<datatype::string>   { using type = std::string; };
    template <> struct cpp_type<datatype::integer>  { using type = std::int64_t; };
    template <> struct cpp_type<datatype::real>     { using type = double; };
    template <> struct cpp_type<datatype::boolean>  { using type = bool; };
    template <> struct cpp_type<datatype::duration> { using type = std::chrono::duration<double>; };

    template <datatype Type, bool Required>
    struct arg{
        using type = typename cpp_type<Type>::type;

        explicit arg() = default;
        explicit arg(const arg&) = default;
        explicit arg(const type& v): _value(v) {}

        type value() const { return _value; }

        private:
            type _value;
    };

    template <datatype Type>
    using required = arg<Type, true>;
    template <datatype Type>
    using optional = arg<Type, true>;

    template <typename... Args>
    struct rhs{
        using args = std::tuple<Args...>;
    };

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
