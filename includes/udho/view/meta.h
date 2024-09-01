#ifndef UDHO_VIEW_PARSER_META_H
#define UDHO_VIEW_PARSER_META_H

#include <map>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <udho/view/data/fwd.h>
#include <udho/view/data/nvp.h>
#include <udho/view/data/detail.h>
#include <udho/view/data/metatype.h>
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
        bool operator()(const T& v){
            _ret = v;
            _assigned = true;
            return false;
        }
        template <typename T, typename std::enable_if_t<!std::is_assignable_v<Ret&, T>>* = nullptr>
        bool operator()(const T&){ return false; }
        bool assigned() const { return _assigned; }

        private:
            Ret& _ret;
            bool _assigned;
    };

    template <typename Value>
    struct value_manipulator{
        value_manipulator(const Value& value): _value(value), _assigned(false) {}

        template <typename T, typename std::enable_if_t<std::is_assignable_v<T&, Value>>* = nullptr>
        bool operator()(T& target){
            target = _value;
            _assigned = true;
            return true;
        }
        template <typename T, typename std::enable_if_t<!std::is_assignable_v<T&, Value>>* = nullptr>
        bool operator()(T&){ return false; }
        bool assigned() const { return _assigned; }

        private:
            const Value& _value;
            bool _assigned;
    };

    struct value_noop{
        template <typename T>
        bool operator()(const T&){ return false; }
    };

    template <typename DataT, typename Function>
    struct visitor_key;

    template <typename DataT, typename Function>
    struct visitor_common{
        using data_type     = DataT;
        using function_type = Function;

        visitor_common(data_type& data, function_type& function, const ast::node_ptr_type& statement, std::size_t idx): _data(data), _function(function), _statement(statement), _idx(idx) {
            assert(_statement->template is_type<ast::statement>());
            assert(_statement->has_content());
            assert(_statement->children.size() >= 1);
        }

        std::size_t idx() const { return _idx; }
        const ast::node_ptr_type& statement() const { return _statement; }
        data_type& data() { return _data; }

        protected:
            function_type& function() { return _function; }
            template <typename T>
            bool pass(T& value){
                return _function(value);
            }
        protected:
            bool has_next() const {
                // Initially index is 0
                // _statement->children.size() has to be more than 1 (including the first key node)
                // Hence indexes_next tells how many sibling indexes (e.g. [], ., ()) to follow
                // if indexes_next is 0 then there is nothing to folow, it was a simple call to the key
                // if indexes_next is 1 then it is key1.key2 or key[10] or key(1) hence needs further processing through an index visitor

                std::size_t indexes_next = _statement->children.size() -1;
                return indexes_next > _idx;
            }
            const ast::node_ptr_type& next() {
                _idx = _idx +1;
                return _statement->children[_idx];
            }
        protected:
            template <typename PolicyT, typename KeyT, typename ValueT, std::enable_if_t<data::policies::is_readable_property_v<PolicyT>, int >* = nullptr >
            typename ValueT::value_type extract(udho::view::data::nvp<PolicyT, KeyT, ValueT>& nvp){
                return nvp.value().get(_data);
            }
            // template <typename PolicyT, typename KeyT, typename ValueT, std::enable_if_t<std::is_same_v<PolicyT, data::policies::function>, int >* = nullptr >
            // typename ValueT::value_type extract(udho::view::data::nvp<PolicyT, KeyT, ValueT>& nvp){ }
        protected:
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

            template <typename KeyT, typename ValueT>
            typename ValueT::result_type _call(udho::view::data::nvp<udho::view::data::policies::function, KeyT, ValueT>& nvp, std::vector<std::string> provided_args){
                using required_arguments_type = typename ValueT::function::arguments_type;
                using result_type             = typename ValueT::result_type;

                constexpr std::size_t required_args_count = std::tuple_size<required_arguments_type>::value;

                assert(required_args_count >= provided_args.size());

                required_arguments_type required_args;
                udho::url::detail::arguments_to_tuple(required_args, provided_args.begin(), provided_args.end());

                return nvp.value().call(data(), required_args);
            }

            template <typename KeyT, typename ValueT>
            typename ValueT::result_type _call(udho::view::data::nvp<udho::view::data::policies::function, KeyT, ValueT>& nvp, const ast::node_ptr_type& values_node){
                assert(values_node->template is_type<ast::values>());
                assert(values_node->has_content());
                assert(values_node->children.size() > 0); // expecting single argument only as it is treated as an assignment operation not a function call
                assert(values_node->children[0]->has_content());

                std::vector<std::string> provided_args;
                std::size_t args_extracted = _list_args(values_node, provided_args);
                assert(args_extracted == provided_args.size());

                typename ValueT::result_type res = _call(nvp, provided_args);

                return res;
            }
            template <typename P, typename KeyT, typename ValueT, std::enable_if_t<data::policies::is_writable_property_v<P>, int>* = nullptr>
            bool _set_str(udho::view::data::nvp<P, KeyT, ValueT>& nvp, const std::string& v){
                bool okay = false;
                std::decay_t<typename ValueT::result_type> input = udho::url::detail::convert_str_to_type<std::decay_t<typename ValueT::result_type>>::apply(v, &okay);
                if(okay){
                    bool res = _set(nvp, input);
                    return res;
                }else{
                    return okay;
                }
            }

            template <typename P, typename KeyT, typename ValueT, std::enable_if_t<!data::policies::is_writable_property_v<P>, int>* = nullptr>
            bool _set_str(udho::view::data::nvp<P, KeyT, ValueT>& nvp, const std::string& v){ return false; }

            template <typename PolicyT, typename KeyT, typename ValueT, typename V, std::enable_if_t<data::policies::is_writable_property_v<PolicyT>, int>* = nullptr>
            bool _set(udho::view::data::nvp<PolicyT, KeyT, ValueT>& nvp, const V& v){ return nvp.value().set(_data, v); }

            template <typename PolicyT, typename KeyT, typename ValueT, typename V, std::enable_if_t<!data::policies::is_writable_property_v<PolicyT>, int>* = nullptr>
            bool _set(udho::view::data::nvp<PolicyT, KeyT, ValueT>& nvp, const V& v){ return false; }

            template <typename ValueT, typename std::enable_if<udho::view::data::has_prototype<ValueT>::value, int>::type* = nullptr>
            bool _assign_str(ValueT& target, std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end){
                std::size_t assignment_count = udho::view::data::assign(target, begin, end);
                assert(assignment_count == std::distance(begin, end));
                return true;
            }

            template <typename ValueT, typename std::enable_if<!udho::view::data::has_prototype<ValueT>::value, int>::type* = nullptr>
            bool _assign_str(ValueT& target, std::vector<std::string>::const_iterator begin, std::vector<std::string>::const_iterator end){ return false; }
        private:
            data_type&                _data;
            function_type&            _function;
            const ast::node_ptr_type& _statement;
            std::size_t               _idx;

    };

    template <typename DataT, typename Function>
    struct visitor_index;

    template <typename DataT, typename Function>
    struct visitor_key: private visitor_common<DataT, Function>{
        using function_type = Function;
        using data_type     = DataT;
        using base          = visitor_common<DataT, Function>;

        visitor_key(data_type& data, function_type& function, const ast::node_ptr_type& statement, std::size_t idx, const ast::node_ptr_type& key): base(data, function, statement, idx), _key(key) {
            assert(_key->template is_type<ast::key>());
            assert(_key->has_content());

            _name = _key->string();

            ast::print(_key);
        }

        template <typename P, typename K, typename V>
        bool operator()(udho::view::data::nvp<udho::view::data::policies::property<P>, K, V>& nvp){
            if(_name != nvp.name()){
                return false;
            }
            typename V::value_type value = base::extract(nvp);
            if(base::has_next()){
                const ast::node_ptr_type& index_node = base::next();
                assert(index_node->template is_type<ast::index>());
                assert(index_node->has_content());
                assert(index_node->children.size() > 0);

                const ast::node_ptr_type& child_node = index_node->children[0];
                if(child_node->template is_type<ast::call>()){
                    const ast::node_ptr_type& call_node = child_node;
                    const ast::node_ptr_type& values_node = call_node->children[0];;
                    // assign
                    std::vector<std::string> provided_args;
                    std::size_t args_count = base::_list_args(values_node, provided_args);

                    assert(provided_args.size() == args_count);

                    if(args_count == 1){
                        // singular assignment
                        return base::_set_str(nvp, provided_args[0]);
                    } else {
                        // multi assign requested
                        // fetch prototype of the requested object (value)
                        return base::_assign_str(value, provided_args.begin(), provided_args.end());
                    }
                } else {
                    return visit(index_node, value);
                }
            } else {
                typename V::value_type previous = value;
                bool modified = base::pass(value);
                if(modified){
                    base::_set(nvp, value);
                }
                return true;
            }
        }
        template <typename K, typename V>
        bool operator()(udho::view::data::nvp<udho::view::data::policies::function, K, V>& nvp){
            if(_name != nvp.name()){
                return false;
            }
            // matched
            // hence expect index > call syntax next
            // no need to extract value from nvp
            if(base::has_next()){
                const ast::node_ptr_type& index_node = base::next();
                assert(index_node->template is_type<ast::index>());
                assert(index_node->has_content());
                assert(index_node->children.size() > 0);

                const ast::node_ptr_type& call_node = index_node->children[0];
                assert(call_node->template is_type<ast::call>());
                assert(call_node->children.size() > 0);

                const ast::node_ptr_type& values_node = call_node->children[0];
                typename V::result_type res = base::_call(nvp, values_node);

                if(base::has_next()){
                    const ast::node_ptr_type& next_index_node = base::next();
                    return visit(next_index_node, res);
                } else {
                    base::pass(res);
                    return true;
                }
            } else {
                // TODO throw exception
                return false;
            }
        }

        private:
            template <typename ValueT, typename std::enable_if<udho::view::data::detail::has_subscript_operator_v<ValueT>, int>::type* = nullptr>
            bool visit(const ast::node_ptr_type& index_node, ValueT& value){
                assert(index_node->template is_type<ast::index>());
                assert(index_node->has_content());
                assert(index_node->children.size() > 0);

                ast::print(index_node);

                const ast::node_ptr_type& child_node = index_node->children[0];

                if(child_node->template is_type<ast::at>()){
                    return at(child_node, value);
                }

                return false;
            }

            template <typename ValueT, typename std::enable_if<!udho::view::data::detail::has_subscript_operator_v<ValueT>, int>::type* = nullptr>
            bool visit(const ast::node_ptr_type& index_node, ValueT& value){
                assert(index_node->template is_type<ast::index>());
                assert(index_node->has_content());
                assert(index_node->children.size() > 0);

                ast::print(index_node);

                const ast::node_ptr_type& child_node = index_node->children[0];

                if(child_node->template is_type<ast::key>()){
                    using key_visitor_type = visitor_key<ValueT, function_type>;
                    key_visitor_type visitor{value, base::function(), base::statement(), base::idx(), child_node};

                    return apply<ValueT>(visitor);
                } else if (child_node->template is_type<ast::call>()) {
                    // throw std::runtime_error{"Not Implemented"};

                    const ast::node_ptr_type& call_node = child_node;
                    const ast::node_ptr_type& values_node = call_node->children[0];;
                    // assign
                    std::vector<std::string> provided_args;
                    std::size_t args_count = base::_list_args(values_node, provided_args);

                    assert(provided_args.size() == args_count);

                    // multi assign requested
                    // fetch prototype of the requested object (value)

                    return base::_assign_str(value, provided_args.begin(), provided_args.end());
                }

                return false;

            }

            template <typename ValueT>
            bool at(const ast::node_ptr_type& at_node, ValueT& value){
                assert(at_node->has_content());
                std::string at_str = at_node->string();
                int at_i = std::stoi(at_str);
                auto& v = value[at_i];

                using v_type = std::decay_t<decltype(v)>;

                if(base::has_next()){
                    const ast::node_ptr_type& index_node = base::next();
                    assert(index_node->template is_type<ast::index>());
                    assert(index_node->has_content());
                    assert(index_node->children.size() > 0);

                    return visit(index_node, v);
                } else {
                    base::pass(v);
                    return true;
                }
            }

            // template <typename ValueT, typename std::enable_if<!udho::view::data::detail::has_subscript_operator_v<ValueT>, int>::type* = nullptr>
            // bool at(const ast::node_ptr_type& at_node, ValueT& value){ return false; }

            template <typename ValueT, typename VisitorT, typename std::enable_if<udho::view::data::has_prototype<ValueT>::value, int>::type* = nullptr>
            bool apply(VisitorT& visitor){
                auto meta = prototype(udho::view::data::type<ValueT>{});
                meta.members().apply_(visitor);

                return true;
            }

            template <typename ValueT, typename VisitorT, typename std::enable_if<!udho::view::data::has_prototype<ValueT>::value, int>::type* = nullptr>
            bool apply(VisitorT&){ return false; }

        private:
            const ast::node_ptr_type& _key;
            std::string               _name;
    };

    template <typename DataT, typename Function>
    struct visitor{
        visitor(const ast::node_ptr_type& id, DataT& data, Function& function): _id(id), _data(data), _function(function), _found(false), _key_visitor(0x0) {
            assert(_id->template is_type<ast::statement>());
            assert(_id->has_content());
            assert(_id->children.size() > 0);
            const ast::node_ptr_type& key_node = _id->children[0];
            assert(key_node->template is_type<ast::key>());
            assert(key_node->has_content());

            _key_visitor = new visitor_key<DataT, Function>{_data, _function, _id, 0, key_node};
        }

        ~visitor(){
            if(_key_visitor)
                delete _key_visitor;
        }

        template <typename PolicyT, typename KeyT, typename ValueT>
        bool operator()(udho::view::data::nvp<PolicyT, KeyT, ValueT>& nvp){
            bool success = (*_key_visitor)(nvp);
            if(!_found){
                _found = success;
            }
            return success;
        }


        bool found() const { return _found; }

        const ast::node_ptr_type& _id;
        DataT&                                          _data;
        Function&                                       _function;
        bool                                            _found;
        visitor_key<DataT, Function>*                   _key_visitor;
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

            ast::print(_ast.root());
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
            ast                  _ast;
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

template <typename ValueT, typename DataT>
ValueT get(DataT& data, const std::string& syntax){
    using executor_type = detail::reader<DataT, ValueT>;
    using function_type = typename executor_type::function_type;

    ValueT value;

    function_type function{value};
    executor_type executor{syntax, data, function};
    executor();

    if(!function.assigned()){
        throw std::runtime_error{udho::url::format("Failed to assign value while retrieving `{}`", syntax)};
    }

    return value;
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
