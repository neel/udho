#ifndef UDHO_VIEW_PARSER_META_H
#define UDHO_VIEW_PARSER_META_H

#include <map>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <udho/view/data/fwd.h>
#include <udho/view/data/nvp.h>
#include <udho/view/data/detail.h>
#include <udho/url/detail/format.h>
#include <iostream>
#include <chrono>
#include <boost/algorithm/string/case_conv.hpp>

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
        struct call:            pegtl::seq<key, whitespace, pegtl::one<'('>, values, pegtl::one<')'>> {};
        struct lookup:          pegtl::sor<call, key> {};
        struct statement;
        struct index:           pegtl::sor<pegtl::seq<pegtl::one<'.'>, statement>, index_seq> {};
        struct statement:       pegtl::seq<lookup, pegtl::star<index>> {};
        struct grammar:         pegtl::must<   pegtl::list<  statement, pegtl::seq<spaced<pegtl::opt<pegtl::one<';'>>>>  >   > {};

        template<typename Rule>
        struct selector:        pegtl::parse_tree::selector<Rule,
            pegtl::parse_tree::store_content::on<
                    grammar,
                    statement,
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
            if (n->template is_type<lookup>())          return "lookup";
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

        inline static void print(const node_ptr_type& n, const std::string& indent = "") {
            if (n->is_root()) {
                std::cout << indent << "Root" << std::endl;
            } else {
                std::cout << indent << get_node_name(n) << " : \"" << n->string_view() << "\"" << std::endl;
            }
            for (const auto& child : n->children) {
                print(child, indent + "  ");
            }
        }

        std::string _str;
        pegtl::memory_input<> _input;
        node_ptr_type _root;
    };

    template <typename Ret>
    struct value_reader{
        value_reader(Ret& ret): _ret(ret), _assigned(false) {}

        template <typename T, typename std::enable_if_t<std::is_assignable_v<Ret&, T>>* = nullptr>
        void operator()(const T& v){
            _ret = v;
            _assigned = true;
        }
        template <typename T, typename std::enable_if_t<!std::is_assignable_v<Ret&, T>>* = nullptr>
        void operator()(const T&){}
        bool assigned() const { return _assigned; }

        private:
            Ret& _ret;
            bool _assigned;
    };

    template <typename Value>
    struct value_manipulator{
        value_manipulator(const Value& value): _value(value), _assigned(false) {}

        template <typename T, typename std::enable_if_t<std::is_assignable_v<T&, Value>>* = nullptr>
        void operator()(T& target){
            target = _value;
            _assigned = true;
        }
        template <typename T, typename std::enable_if_t<!std::is_assignable_v<T&, Value>>* = nullptr>
        void operator()(T&){ }
        bool assigned() const { return _assigned; }

        private:
            const Value& _value;
            bool _assigned;
    };

    struct value_noop{
        template <typename T>
        void operator()(const T&){}
    };

    template <typename DataT, typename Function>
    struct visitor{
        visitor(const ast::node_ptr_type& id, DataT& data, Function& function): _id(id), _data(data), _function(function), _found(false) {}

        template <typename PolicyT, typename KeyT, typename ValueT>
        bool operator()(udho::view::data::nvp<PolicyT, KeyT, ValueT>& nvp){
            // An id node must have or more children
            // The first child of an id node has to be a lookup node

            assert(_id->template is_type<ast::statement>());
            assert(_id->has_content());
            assert(_id->children.size() > 0);
            const ast::node_ptr_type& lookup_node = _id->children[0];
            assert(lookup_node->template is_type<ast::lookup>());
            assert(lookup_node->has_content());

            bool success = match_lookup(lookup_node, 0, nvp);
            if(!_found){
                _found = success;
            }
            return success;
        }

        template <typename KeyT, typename ValueT>
        bool match_lookup(const ast::node_ptr_type& lookup_node, std::size_t i, udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp){
            assert(lookup_node->template is_type<ast::lookup>());
            assert(lookup_node->children.size() > 0);

            const ast::node_ptr_type& child = lookup_node->children[0];

            if(child->template is_type<ast::key>()){
                const ast::node_ptr_type& key_node = child;
                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                }else{
                    using result_type  = typename ValueT::result_type;
                    using result_ref_t = std::add_lvalue_reference_t<result_type>;
                    result_ref_t res   = _get(nvp);

                    if(_id->children.size() > i+1){
                        // child under id implies index queries
                        const ast::node_ptr_type& index_node = _id->children[i+1];
                        assert(index_node->template is_type<ast::index>());
                        assert(index_node->has_content());
                        return match_index(index_node, i+1, res);
                    } else {
                        // No further subscript to follow
                        // manipulator allowed
                        _function(res);
                        return true;
                    }
                }
            } else if(child->template is_type<ast::call>()){
                // setter intended
                const ast::node_ptr_type& call_node = child;
                assert(call_node->children.size() > 1);
                const ast::node_ptr_type& key_node = call_node->children[0];
                assert(key_node->template is_type<ast::key>());
                assert(key_node->has_content());

                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                } else {
                    const ast::node_ptr_type& values_node = call_node->children[1];

                    assert(values_node->template is_type<ast::values>());
                    assert(values_node->has_content());
                    assert(values_node->children.size() == 1); // expecting single argument only as it is treated as an assignment operation not a function call
                    assert(values_node->children[0]->has_content());

                    std::vector<std::string> provided_args;
                    std::size_t args_extracted = _list_args(values_node, provided_args);
                    assert(args_extracted == provided_args.size());
                    assert(1 == provided_args.size());

                    std::string arg_str = provided_args[0];

                    bool res = _set_str(nvp, arg_str);
                    // manipulator not allowed
                    _function(res);

                    return true;
                }
            }  else {
                return false;
            }
        }

        template <typename KeyT, typename ValueT>
        bool match_lookup(const ast::node_ptr_type& lookup_node, std::size_t i, udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, ValueT>& nvp){
            assert(lookup_node->template is_type<ast::lookup>());
            assert(lookup_node->children.size() > 0);

            const ast::node_ptr_type& child = lookup_node->children[0];

            if(child->template is_type<ast::key>()){
                const ast::node_ptr_type& key_node = child;
                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                }else{
                    using result_type  = typename ValueT::result_type;
                    using result_ref_t = std::add_lvalue_reference_t<result_type>;
                    result_ref_t res   = _get(nvp);

                    if(_id->children.size() > i+1){
                        // child under id implies index queries
                        const ast::node_ptr_type& index_node = _id->children[i+1];
                        assert(index_node->template is_type<ast::index>());
                        assert(index_node->has_content());
                        return match_index(index_node, i+1, res);
                    } else {
                        // No further subscript to follow
                        // manipulator not allowed
                        _function(res);
                        return true;
                    }
                }
            } else {
                return false;
            }
        }

        template <typename KeyT, typename ValueT>
        bool match_lookup(const ast::node_ptr_type& lookup_node, std::size_t i, udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp){
            assert(lookup_node->template is_type<ast::lookup>());
            assert(lookup_node->children.size() > 0);

            const ast::node_ptr_type& child = lookup_node->children[0];

            if(child->template is_type<ast::key>()){
                // getter intended
                const ast::node_ptr_type& key_node = child;
                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                }else{
                    using result_type = typename ValueT::result_type;
                    result_type res   = _get(nvp);

                    if(_id->children.size() > i+1){
                        // child under id implies index queries
                        const ast::node_ptr_type& index_node = _id->children[i+1];
                        assert(index_node->template is_type<ast::index>());
                        assert(index_node->has_content());
                        return match_index(index_node, i+1, res);
                    } else {
                        // No further subscript to follow
                        // manipulator allowed
                        result_type previous = res;
                        _function(res);

                        // although manipulator is allowed the nvp is functional
                        // modification to res does not reflect to the nvp
                        if(previous != res){
                            _set(nvp, res);
                        }
                        return true;
                    }
                }
            } else if(child->template is_type<ast::call>()){
                // setter intended
                const ast::node_ptr_type& call_node = child;
                assert(call_node->children.size() > 1);
                const ast::node_ptr_type& key_node = call_node->children[0];
                assert(key_node->template is_type<ast::key>());
                assert(key_node->has_content());

                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                } else {
                    const ast::node_ptr_type& values_node = call_node->children[1];

                    assert(values_node->template is_type<ast::values>());
                    assert(values_node->has_content());
                    assert(values_node->children.size() == 1); // expecting single argument only as it is treated as an assignment operation not a function call
                    assert(values_node->children[0]->has_content());

                    std::vector<std::string> provided_args;
                    std::size_t args_extracted = _list_args(values_node, provided_args);
                    assert(args_extracted == provided_args.size());
                    assert(1 == provided_args.size());

                    std::string arg_str = provided_args[0];

                    bool res = _set_str(nvp, arg_str);
                    // manipulator not allowed
                    _function(res);

                    return true;
                }
            } else {
                return false;
            }
        }

        template <typename KeyT, typename ValueT>
        bool match_lookup(const ast::node_ptr_type& lookup_node, std::size_t i, udho::view::data::nvp<udho::view::data::policies::function, KeyT, ValueT>& nvp){
            assert(lookup_node->template is_type<ast::lookup>());
            assert(lookup_node->children.size() > 0);

            const ast::node_ptr_type& child = lookup_node->children[0];

            if(child->template is_type<ast::call>()){
                const ast::node_ptr_type& call_node = child;
                assert(call_node->children.size() > 1);
                const ast::node_ptr_type& key_node = call_node->children[0];
                assert(key_node->template is_type<ast::key>());
                assert(key_node->has_content());

                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                } else {
                    const ast::node_ptr_type& values_node = call_node->children[1];

                    assert(values_node->template is_type<ast::values>());
                    assert(values_node->has_content());

                    std::vector<std::string> provided_args;
                    std::size_t args_extracted = _list_args(values_node, provided_args);
                    assert(args_extracted == provided_args.size());

                    typename ValueT::result_type res = _call(nvp, provided_args);

                    if(_id->children.size() > i+1){
                        // child under id implies index queries
                        const ast::node_ptr_type& index_node = _id->children[i+1];
                        assert(index_node->template is_type<ast::index>());
                        assert(index_node->has_content());
                        return match_index(index_node, i+1, res);
                    } else {
                        // manipulator not allowed
                        _function(res);
                        return true;
                    }
                }
            } else {
                return false;
            }
        }

        template <typename X>
        bool match_index(const ast::node_ptr_type& index_node, std::size_t i, X& val){
            assert(index_node->template is_type<ast::index>());
            assert(index_node->children.size() == 1);
            const ast::node_ptr_type& child = index_node->children[0];
            if(child->template is_type<ast::at>()){
                // index_at query
                return match_at(child, i, val);
            } else if (child->template is_type<ast::statement>()) {
                // dot query
                return match_object(child, i, val);
            }
            return false;
        }

        template <typename X, typename std::enable_if<udho::view::data::detail::has_subscript_operator_v<X>, int>::type* = nullptr>
        bool match_at(const ast::node_ptr_type& at_node, std::size_t i, X& vec){
            if(at_node->template is_type<ast::at>()){
                assert(at_node->has_content());
                std::string at_str = at_node->string();
                int at_i = std::stoi(at_str);
                auto& v = vec[at_i];

                // data item correspondng to the at query retrieved
                // now traverse to the next sibling
                if(_id->children.size() > i+1){
                    const ast::node_ptr_type& index_node = _id->children[i+1];
                    assert(index_node->template is_type<ast::index>());
                    assert(index_node->has_content());
                    return match_index(index_node, i+1, v); // Need to pass the sibling
                } else {
                    // There are no other subscripts to follow
                    // manipulator not allowed
                    _function(v);
                    return true;
                }
            }
            return false;
        }

        template <typename X, typename std::enable_if<!udho::view::data::detail::has_subscript_operator_v<X>, int>::type* = nullptr>
        bool match_at(const ast::node_ptr_type&, std::size_t, X&){ return false; }

        template <typename X, typename std::enable_if<udho::view::data::has_prototype<X>::value, int>::type* = nullptr>
        bool match_object(const ast::node_ptr_type& id_node, std::size_t i, X& obj){
            assert(id_node->template is_type<ast::statement>());
            assert(id_node->has_content());

            std::string id_str = id_node->string();

            visitor<X, Function> finder{id_node, obj, _function};
            auto meta = prototype(udho::view::data::type<X>{});

            meta.members().apply_(finder);

            return finder.found();
        }

        template <typename X, typename std::enable_if<!udho::view::data::has_prototype<X>::value, int>::type* = nullptr>
        bool match_object(const ast::node_ptr_type&, std::size_t, X&){ return false; }

        template <typename PolicyT, typename KeyT, typename ValueT, typename std::enable_if<std::is_same_v<PolicyT, udho::view::data::policies::readonly> || std::is_same_v<PolicyT, udho::view::data::policies::writable>, int>::type* = nullptr>
        std::add_lvalue_reference_t<typename ValueT::result_type> _get(udho::view::data::nvp<udho::view::data::policies::property<PolicyT>, KeyT, ValueT>& nvp){ return nvp.value().get(_data); }

        template <typename KeyT, typename ValueT>
        typename ValueT::result_type _get(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp){ return nvp.value().get(_data); }

        template <typename PolicyT, typename KeyT, typename ValueT, typename V, typename std::enable_if<std::is_same_v<PolicyT, udho::view::data::policies::functional> || std::is_same_v<PolicyT, udho::view::data::policies::writable>, int>::type* = nullptr>
        bool _set(udho::view::data::nvp<udho::view::data::policies::property<PolicyT>, KeyT, ValueT>& nvp, const V& v){ return nvp.value().set(_data, v); }

        template <typename KeyT, typename ValueT>
        bool _set_str(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp, const std::string& v){
            bool okay = false;
            std::decay_t<typename ValueT::arg_type> input = udho::url::detail::convert_str_to_type<std::decay_t<typename ValueT::arg_type>>::apply(v, &okay);
            if(okay){
                bool res = _set(nvp, input);
                return res;
            }else{
                return okay;
            }
        }

        template <typename KeyT, typename ValueT>
        bool _set_str(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp, const std::string& v){
            bool okay = false;
            std::decay_t<typename ValueT::result_type> input = udho::url::detail::convert_str_to_type<std::decay_t<typename ValueT::result_type>>::apply(v, &okay);
            if(okay){
                bool res = _set(nvp, input);
                return res;
            }else{
                return okay;
            }
        }

        template <typename KeyT, typename ValueT>
        typename ValueT::result_type _call(udho::view::data::nvp<udho::view::data::policies::function, KeyT, ValueT>& nvp, std::vector<std::string> provided_args){
            using required_arguments_type = typename ValueT::function::arguments_type;
            using result_type             = typename ValueT::result_type;

            constexpr std::size_t required_args_count = std::tuple_size<required_arguments_type>::value;

            assert(required_args_count >= provided_args.size());

            required_arguments_type required_args;
            udho::url::detail::arguments_to_tuple(required_args, provided_args.begin(), provided_args.end());

            return nvp.value().call(_data, required_args);
        }

        std::size_t _list_args(const ast::node_ptr_type& values_node, std::vector<std::string>& provided_args){
            assert(values_node->template is_type<ast::values>());
            assert(values_node->has_content());

            std::size_t counter = 0;
            std::transform(values_node->children.begin(), values_node->children.end(), std::back_inserter(provided_args), [&counter](const ast::node_ptr_type& c){
                if(c->template is_type<ast::integer>() || c->template is_type<ast::real>() || c->template is_type<ast::duration>()){
                    ++counter;
                    return c->string();
                } else if (c->template is_type<ast::quoted_string>()) {
                    std::string q_str = c->string();
                    q_str.erase(0, 1);
                    q_str.erase(q_str.size()-1);
                    ++counter;
                    return q_str;
                } else if (c->template is_type<ast::boolean>()) {
                    std::string q_str = c->string();
                    boost::algorithm::to_lower(q_str);
                    if(q_str == "on"  || q_str == "true")  return std::string{"1"};
                    if(q_str == "off" || q_str == "false") return std::string{"0"};
                    return std::string{};
                } else {
                    return std::string{};
                }
            });
            return counter;
        }

        bool found() const { return _found; }

        const ast::node_ptr_type& _id;
        DataT&                                          _data;
        Function&                                       _function;
        bool                                            _found;
    };

    template <typename DataT, typename Function, bool ReEntrant>
    struct basic_executor{
        using function_type = Function;
        using finder_type   = visitor<DataT, function_type>;
        using meta_type     = decltype(prototype(std::declval<udho::view::data::type<DataT>>()));
        using node_ptr_type = ast::node_ptr_type;

        basic_executor(const std::string& syntax, DataT& data, function_type& function): _syntax(syntax), _ast(_syntax), _data(data), _meta(prototype(udho::view::data::type<DataT>{})), _function(function), _grammar(_ast.root()->children[0]) {
            assert(_grammar->template is_type<ast::grammar>());
            assert(_grammar->children.size() > 0);
        }

        void apply(){
            std::size_t num_statements = _grammar->children.size();
            if(!ReEntrant && num_statements > 1){
                throw std::domain_error{udho::url::format("Meta syntax `{}` executed using NOT ReEntrant callback, hence expecting exactly 1 statement, but got {} statements ", _syntax, num_statements)};
            }

            for(const node_ptr_type& child: _grammar->children){
                assert(child->template is_type<ast::statement>());

                const node_ptr_type& statement = child;
                assert(statement->has_content());

                finder_type finder{statement, _data, _function};
                _meta.members().apply_(finder);

                bool success = finder.found();

                if(!success){
                    throw std::out_of_range{udho::url::format("Failed to find corresponding NVP in data for statement {}", statement->string())};
                }
            }
        }

        void operator()() { apply(); }

        private:
            std::string          _syntax;
            ast               _ast;
            DataT&               _data;
            meta_type            _meta;
            function_type&       _function;
            const node_ptr_type& _grammar;
    };

    template <typename DataT>
    using executor = basic_executor<DataT, value_noop, true>;
    template <typename DataT, typename ValueT>
    using reader   = basic_executor<DataT, value_reader<ValueT>, false>;
    template <typename DataT, typename ValueT>
    using writer   = basic_executor<DataT, value_manipulator<ValueT>, false>;
}

template <typename DataT>
void exec(DataT& data, const std::string& syntax){
    using executor_type = detail::executor<DataT>;
    using function_type = typename executor_type::function_type;

    function_type function;
    executor_type executor{syntax, data, function};
    executor();
}

template <typename DataT, typename ValueT>
bool get(DataT& data, const std::string& syntax, ValueT& value){
    using executor_type = detail::reader<DataT, ValueT>;
    using function_type = typename executor_type::function_type;

    function_type function{value};
    executor_type executor{syntax, data, function};
    executor();

    return function.assigned();
}

template <typename DataT, typename ValueT>
bool set(DataT& data, const std::string& syntax, const ValueT& value){
    using executor_type = detail::writer<DataT, ValueT>;
    using function_type = typename executor_type::function_type;

    function_type function{value};
    executor_type executor{syntax, data, function};
    executor();

    return function.assigned();
}

}
}
}
}

#endif // UDHO_VIEW_PARSER_META_H
