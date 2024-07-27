/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_VIEW_DATA_DETAIL_H
#define UDHO_VIEW_DATA_DETAIL_H

#include <udho/view/data/fwd.h>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <list>
#include <vector>
#include <boost/algorithm/string/case_conv.hpp>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif

namespace udho{
namespace view{
namespace data{

namespace detail{
    /**
     * @brief Gets the value from an nvp and assignes it to the passed reference
     * \code
     * Person p
     * std:string value;
     * auto getter = getter_f{value, p};
     * getter(nvp_name); // The nvp should be
     * // if successful value should contain the persons name.
     * // Assuming nvp_name is the nvp mapped with the name property of person class
     * \endcode
     */
    template <typename ValueT, typename DataT>
    struct getter_f{
        using value_type  = std::add_lvalue_reference_t<ValueT>;
        using data_type   = std::add_const_t<std::add_lvalue_reference_t<DataT>>;

        enum {
            is_getter = true,
            is_setter = false,
            is_caller = false
        };

        getter_f(value_type ret, data_type d): _ret(ret), _data(d), _success(false) {}

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<value_type, typename V::result_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::writable>, K, V>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, std::ref(_data));
            _ret = function();
            _success = true;
        }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<value_type, typename V::result_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::readonly>, K, V>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, std::ref(_data));
            _ret = function();
            _success = true;
        }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<value_type, typename V::result_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::functional>, K, V>& nvp){
            auto wrapper  = *nvp.value().getter();
            auto function = std::bind(wrapper, std::ref(_data));
            _ret = function();
            _success = true;
        }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<value_type, typename V::result_type> >::type>
        void operator()(nvp<policies::function, K, V>&){ }

        template <typename PolicyT, typename K, typename V, typename = typename std::enable_if< !std::is_assignable_v<value_type, typename V::result_type> >::type>
        void operator()(nvp<PolicyT, K, V>&){ }

        bool success() const { return _success; }
        operator bool () const { return success(); }
        bool operator!() const { return !success(); }

        value_type _ret;
        data_type  _data;
        bool       _success;
    };

    template <typename ValueT, typename DataT>
    struct setter_f{
        using value_type = std::add_const_t<std::add_lvalue_reference_t<ValueT>>;
        using data_type  = std::add_lvalue_reference_t<DataT>;

        enum {
            is_getter = false,
            is_setter = true,
            is_caller = false
        };

        setter_f(value_type v, data_type d): _value(v), _data(d), _success(false) {}

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<std::add_lvalue_reference_t<typename V::result_type>, value_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::writable>, K, V>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, &_data);
            function() = _value;
            _success = true;
        }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<std::add_lvalue_reference_t<typename V::result_type>, value_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::readonly>, K, V>& nvp){ }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<std::add_lvalue_reference_t<typename V::result_type>, value_type> >::type>
        void operator()(nvp<policies::property<udho::view::data::policies::functional>, K, V>& nvp){
            auto wrapper  = *nvp.value().setter();
            auto function = std::bind(wrapper, &_data, _value);
            function();
            _success = true;
        }

        template <typename K, typename V, typename = typename std::enable_if< std::is_assignable_v<std::add_lvalue_reference_t<typename V::result_type>, value_type> >::type>
        void operator()(nvp<policies::function, K, V>&){ }

        template <typename PolicyT, typename K, typename V, typename = typename std::enable_if< !std::is_assignable_v<std::add_lvalue_reference_t<typename V::result_type>, value_type> >::type>
        void operator()(nvp<PolicyT, K, V>&){ }

        bool success() const { return _success; }
        operator bool () const { return success(); }
        bool operator!() const { return !success(); }

        value_type _value;
        data_type        _data;
        bool             _success;
    };

    template <typename Ret, typename DataT, typename... X>
    struct caller_f{
        using provided_args_type = std::tuple<X...>;
        static constexpr std::size_t provided_args_size = std::tuple_size_v<provided_args_type>;

        enum {
            is_getter = false,
            is_setter = false,
            is_caller = true
        };

        caller_f(Ret& ret, DataT& d, X&&... args): _ret(ret), _data(d), _args(std::forward<X>(args)...), _success(false) {}
        caller_f(Ret& ret, DataT& d, std::tuple<X...> args): _ret(ret), _data(d), _args(args), _success(false) {}

        template <typename K, typename V, typename std::enable_if_t<std::is_assignable_v<Ret&, typename V::result_type>, int> = 0>
        void operator()(nvp<policies::function, K, V>& nvp) {
            call(nvp);
        }

        template <typename K, typename V, typename std::enable_if_t<!std::is_assignable_v<Ret&, typename V::result_type>, int> = 0>
        void operator()(nvp<policies::function, K, V>&) { }
        template <typename PropertyPolicy, typename K, typename V>
        void operator()(nvp<policies::property<PropertyPolicy>, K, V>&){}

        template <typename K, typename V, typename std::enable_if_t<provided_args_size <= std::tuple_size_v<typename V::function::arguments_type>, int> = 0>
        void call(nvp<policies::function, K, V>& nvp){
            static_assert(std::is_assignable_v<Ret&, typename V::result_type>);

            using required_arguments_type     = typename V::function::arguments_type;
            constexpr auto required_args_size = std::tuple_size_v<required_arguments_type>;
            static_assert(provided_args_size <= required_args_size);

            required_arguments_type required_args;
            udho::url::detail::tuple_copy(_args, required_args);

            auto wrapper  = *nvp.value();
            _ret = std::apply(wrapper, std::tuple_cat(std::make_tuple(std::ref(_data)), required_args));
            _success = true;
        }

        template <typename K, typename V, typename std::enable_if_t< std::tuple_size_v<typename V::function::arguments_type> < provided_args_size, int> = 0>
        void call(nvp<policies::function, K, V>& nvp){
            static_assert(std::is_assignable_v<Ret&, typename V::result_type>);

            using required_arguments_type     = typename V::function::arguments_type;
            constexpr auto required_args_size = std::tuple_size_v<required_arguments_type>;
            static_assert(provided_args_size  > required_args_size);

            throw std::invalid_argument{udho::url::format("function {} called with more arguments than needed", nvp.name())};
        }

        bool success() const { return _success; }
        operator bool () const { return success(); }
        bool operator!() const { return !success(); }

        Ret&             _ret;
        DataT&           _data;
        std::tuple<X...> _args;
        bool             _success;
    };

    template <typename Function>
    struct extractor_f{
        extractor_f(Function&& f): _f(std::move(f)) {}

        template <typename PolicyT, typename KeyT, typename ValueT>
        decltype(auto) operator()(nvp<PolicyT, KeyT, ValueT>& nvp){
            return _f(nvp);
        }

        Function _f;
    };
    template <typename KeyT, bool Once = false>
    struct match_f{
        match_f(KeyT&& key): _key(std::move(key)), _count(0) {}
        template <typename PolicyT, typename ValueT>
        bool operator()(const nvp<PolicyT, KeyT, ValueT>& nvp){
            if(Once && _count > 1){
                return false;
            }

            bool res = nvp.name() == _key;
            _count = _count + res;
            return res;
        }
        template <typename OtherPolicyT, typename OtherKeyT, typename ValueT>
        bool operator()(const nvp<OtherPolicyT, OtherKeyT, ValueT>& nvp){
            return false;
        }

        KeyT _key;
        std::size_t _count;
    };

    template <typename, typename = std::void_t<>>
    struct has_subscript_operator : std::false_type {};

    // Specialization recognizes types that do have an operator[]
    template <typename T>
    struct has_subscript_operator<T, std::void_t<decltype(std::declval<T>()[std::declval<std::size_t>()])>> : std::true_type {};

    // Helper variable template to simplify usage
    template <typename T>
    constexpr bool has_subscript_operator_v = has_subscript_operator<T>::value;

    namespace pegtl = tao::pegtl;

    struct id_ast{
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
        struct id;
        struct index:           pegtl::sor<pegtl::seq<pegtl::one<'.'>, id>, index_seq> {};
        struct id:              pegtl::seq<lookup, pegtl::star<index>> {};

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
                    index
                >
            > {};

        inline id_ast(const std::string& id_str): _str(std::move(id_str)), _input(_str, "input") {
            try{
                _root = pegtl::parse_tree::parse<id, selector>(_input);
                print_tree(_root);
            } catch (const pegtl::parse_error& e) {
                const auto p = e.positions().front();
                std::cerr << e.what() << '\n' << _input.line_at(p) << std::endl;
            }
        }

        const std::unique_ptr<pegtl::parse_tree::node>& root() const { return _root; }

        inline static std::string get_node_name(const std::unique_ptr<tao::pegtl::parse_tree::node>& n) {
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
            return "unknown";
        }

        inline static void print_tree(const std::unique_ptr<tao::pegtl::parse_tree::node>& n, const std::string& indent = "") {
            if (n->is_root()) {
                std::cout << indent << "Root" << std::endl;
            } else {
                std::cout << indent << get_node_name(n) << " : \"" << n->string_view() << "\"" << std::endl;
            }
            for (const auto& child : n->children) {
                print_tree(child, indent + "  ");
            }
        }

        std::string _str;
        pegtl::memory_input<> _input;
        std::unique_ptr<pegtl::parse_tree::node> _root;
    };

    template <typename Ret>
    struct id_value_extractor{
        id_value_extractor(Ret& ret): _ret(ret), _assigned(false) {}

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
    struct id_value_manipulator{
        id_value_manipulator(const Value& value): _value(value), _assigned(false) {}

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

    template <typename DataT, typename Function>
    struct id_finder{
        id_finder(const std::unique_ptr<pegtl::parse_tree::node>& id, DataT& data, Function& function): _id(id), _data(data), _function(function) {}

        template <typename PolicyT, typename KeyT, typename ValueT>
        bool operator()(nvp<PolicyT, KeyT, ValueT>& nvp){
            // An id node must have or more children
            // The first child of an id node has to be a lookup node

            assert(_id->template is_type<id_ast::id>());
            assert(_id->has_content());
            assert(_id->children.size() > 0);
            const std::unique_ptr<pegtl::parse_tree::node>& lookup_node = _id->children[0];
            assert(lookup_node->template is_type<id_ast::lookup>());
            assert(lookup_node->has_content());

            return match_lookup(lookup_node, 0, nvp);
        }

        template <typename KeyT, typename ValueT>
        bool match_lookup(const std::unique_ptr<pegtl::parse_tree::node>& lookup_node, std::size_t i, nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp){
            assert(lookup_node->template is_type<id_ast::lookup>());
            assert(lookup_node->children.size() > 0);

            const std::unique_ptr<pegtl::parse_tree::node>& child = lookup_node->children[0];

            if(child->template is_type<id_ast::key>()){
                const std::unique_ptr<pegtl::parse_tree::node>& key_node = child;
                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                }else{
                    using result_type  = typename ValueT::result_type;
                    using result_ref_t = std::add_lvalue_reference_t<result_type>;
                    result_ref_t res   = _get(nvp);

                    if(_id->children.size() > i+1){
                        // child under id implies index queries
                        const std::unique_ptr<pegtl::parse_tree::node>& index_node = _id->children[i+1];
                        assert(index_node->template is_type<id_ast::index>());
                        assert(index_node->has_content());
                        return match_index(index_node, i+1, res);
                    } else {
                        // No further subscript to follow
                        // manipulator allowed
                        _function(res);
                        return true;
                    }
                }
            } else if(child->template is_type<id_ast::call>()){
                // setter intended
                const std::unique_ptr<pegtl::parse_tree::node>& call_node = child;
                assert(call_node->children.size() > 1);
                const std::unique_ptr<pegtl::parse_tree::node>& key_node = call_node->children[0];
                assert(key_node->template is_type<id_ast::key>());
                assert(key_node->has_content());

                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                } else {
                    const std::unique_ptr<pegtl::parse_tree::node>& values_node = call_node->children[1];

                    assert(values_node->template is_type<id_ast::values>());
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
        bool match_lookup(const std::unique_ptr<pegtl::parse_tree::node>& lookup_node, std::size_t i, nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, ValueT>& nvp){
            assert(lookup_node->template is_type<id_ast::lookup>());
            assert(lookup_node->children.size() > 0);

            const std::unique_ptr<pegtl::parse_tree::node>& child = lookup_node->children[0];

            if(child->template is_type<id_ast::key>()){
                const std::unique_ptr<pegtl::parse_tree::node>& key_node = child;
                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                }else{
                    using result_type  = typename ValueT::result_type;
                    using result_ref_t = std::add_lvalue_reference_t<result_type>;
                    result_ref_t res   = _get(nvp);

                    if(_id->children.size() > i+1){
                        // child under id implies index queries
                        const std::unique_ptr<pegtl::parse_tree::node>& index_node = _id->children[i+1];
                        assert(index_node->template is_type<id_ast::index>());
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
        bool match_lookup(const std::unique_ptr<pegtl::parse_tree::node>& lookup_node, std::size_t i, nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp){
            assert(lookup_node->template is_type<id_ast::lookup>());
            assert(lookup_node->children.size() > 0);

            const std::unique_ptr<pegtl::parse_tree::node>& child = lookup_node->children[0];

            if(child->template is_type<id_ast::key>()){
                // getter intended
                const std::unique_ptr<pegtl::parse_tree::node>& key_node = child;
                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                }else{
                    using result_type = typename ValueT::result_type;
                    result_type res   = _get(nvp);

                    if(_id->children.size() > i+1){
                        // child under id implies index queries
                        const std::unique_ptr<pegtl::parse_tree::node>& index_node = _id->children[i+1];
                        assert(index_node->template is_type<id_ast::index>());
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
            } else if(child->template is_type<id_ast::call>()){
                // setter intended
                const std::unique_ptr<pegtl::parse_tree::node>& call_node = child;
                assert(call_node->children.size() > 1);
                const std::unique_ptr<pegtl::parse_tree::node>& key_node = call_node->children[0];
                assert(key_node->template is_type<id_ast::key>());
                assert(key_node->has_content());

                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                } else {
                    const std::unique_ptr<pegtl::parse_tree::node>& values_node = call_node->children[1];

                    assert(values_node->template is_type<id_ast::values>());
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
        bool match_lookup(const std::unique_ptr<pegtl::parse_tree::node>& lookup_node, std::size_t i, nvp<policies::function, KeyT, ValueT>& nvp){
            assert(lookup_node->template is_type<id_ast::lookup>());
            assert(lookup_node->children.size() > 0);

            const std::unique_ptr<pegtl::parse_tree::node>& child = lookup_node->children[0];

            if(child->template is_type<id_ast::call>()){
                const std::unique_ptr<pegtl::parse_tree::node>& call_node = child;
                assert(call_node->children.size() > 1);
                const std::unique_ptr<pegtl::parse_tree::node>& key_node = call_node->children[0];
                assert(key_node->template is_type<id_ast::key>());
                assert(key_node->has_content());

                std::string key_name = key_node->string();
                if(key_name != nvp.name()){
                    return false;
                } else {
                    const std::unique_ptr<pegtl::parse_tree::node>& values_node = call_node->children[1];

                    assert(values_node->template is_type<id_ast::values>());
                    assert(values_node->has_content());

                    std::vector<std::string> provided_args;
                    std::size_t args_extracted = _list_args(values_node, provided_args);
                    assert(args_extracted == provided_args.size());

                    typename ValueT::result_type res = _call(nvp, provided_args);

                    if(_id->children.size() > i+1){
                        // child under id implies index queries
                        const std::unique_ptr<pegtl::parse_tree::node>& index_node = _id->children[i+1];
                        assert(index_node->template is_type<id_ast::index>());
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
        bool match_index(const std::unique_ptr<pegtl::parse_tree::node>& index_node, std::size_t i, X& val){
            assert(index_node->template is_type<id_ast::index>());
            assert(index_node->children.size() == 1);
            const std::unique_ptr<pegtl::parse_tree::node>& child = index_node->children[0];
            if(child->template is_type<id_ast::at>()){
                // index_at query
                return match_at(child, i, val);
            } else if (child->template is_type<id_ast::id>()) {
                // dot query
                return match_object(child, i, val);
            }
            return false;
        }

        template <typename X, typename std::enable_if<has_subscript_operator_v<X>, int>::type* = nullptr>
        bool match_at(const std::unique_ptr<pegtl::parse_tree::node>& at_node, std::size_t i, X& vec){
            if(at_node->template is_type<id_ast::at>()){
                assert(at_node->has_content());
                std::string at_str = at_node->string();
                int at_i = std::stoi(at_str);
                auto& v = vec[at_i];

                // data item correspondng to the at query retrieved
                // now traverse to the next sibling
                if(_id->children.size() > i+1){
                    const std::unique_ptr<pegtl::parse_tree::node>& index_node = _id->children[i+1];
                    assert(index_node->template is_type<id_ast::index>());
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

        template <typename X, typename std::enable_if<!has_subscript_operator_v<X>, int>::type* = nullptr>
        bool match_at(const std::unique_ptr<pegtl::parse_tree::node>&, std::size_t, X&){ return false; }

        template <typename X, typename std::enable_if<has_prototype<X>::value, int>::type* = nullptr>
        bool match_object(const std::unique_ptr<pegtl::parse_tree::node>& id_node, std::size_t i, X& obj){
            assert(id_node->template is_type<id_ast::id>());
            assert(id_node->has_content());

            std::string id_str = id_node->string();

            id_finder<X, Function> finder{id_node, obj, _function};
            auto meta = prototype(udho::view::data::type<X>{});

            return meta.members().find_recursive(finder);
        }

        template <typename X, typename std::enable_if<!has_prototype<X>::value, int>::type* = nullptr>
        bool match_object(const std::unique_ptr<pegtl::parse_tree::node>&, std::size_t, X&){ return false; }

        template <typename PolicyT, typename KeyT, typename ValueT, typename std::enable_if<std::is_same_v<PolicyT, udho::view::data::policies::readonly> || std::is_same_v<PolicyT, udho::view::data::policies::writable>, int>::type* = nullptr>
        std::add_lvalue_reference_t<typename ValueT::result_type> _get(nvp<udho::view::data::policies::property<PolicyT>, KeyT, ValueT>& nvp){ return nvp.value().get(_data); }

        template <typename KeyT, typename ValueT>
        typename ValueT::result_type _get(nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp){ return nvp.value().get(_data); }

        template <typename PolicyT, typename KeyT, typename ValueT, typename V, typename std::enable_if<std::is_same_v<PolicyT, udho::view::data::policies::functional> || std::is_same_v<PolicyT, udho::view::data::policies::writable>, int>::type* = nullptr>
        bool _set(nvp<udho::view::data::policies::property<PolicyT>, KeyT, ValueT>& nvp, const V& v){ return nvp.value().set(_data, v); }

        template <typename KeyT, typename ValueT>
        bool _set_str(nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp, const std::string& v){
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
        bool _set_str(nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp, const std::string& v){
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
        typename ValueT::result_type _call(nvp<policies::function, KeyT, ValueT>& nvp, std::vector<std::string> provided_args){
            using required_arguments_type = typename ValueT::function::arguments_type;
            using result_type             = typename ValueT::result_type;

            constexpr std::size_t required_args_count = std::tuple_size<required_arguments_type>::value;

            assert(required_args_count >= provided_args.size());

            required_arguments_type required_args;
            udho::url::detail::arguments_to_tuple(required_args, provided_args.begin(), provided_args.end());

            return nvp.value().call(_data, required_args);
        }

        std::size_t _list_args(const std::unique_ptr<pegtl::parse_tree::node>& values_node, std::vector<std::string>& provided_args){
            assert(values_node->template is_type<id_ast::values>());
            assert(values_node->has_content());

            std::size_t counter = 0;
            std::transform(values_node->children.begin(), values_node->children.end(), std::back_inserter(provided_args), [&counter](const std::unique_ptr<pegtl::parse_tree::node>& c){
                if(c->template is_type<id_ast::integer>() || c->template is_type<id_ast::real>() || c->template is_type<id_ast::duration>()){
                    ++counter;
                    return c->string();
                } else if (c->template is_type<id_ast::quoted_string>()) {
                    std::string q_str = c->string();
                    q_str.erase(0, 1);
                    q_str.erase(q_str.size()-1);
                    ++counter;
                    return q_str;
                } else if (c->template is_type<id_ast::boolean>()) {
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

        const std::unique_ptr<pegtl::parse_tree::node>& _id;
        DataT&                                          _data;
        Function&                                       _function;
    };

    struct match_all{
        template <typename PolicyT, typename KeyT, typename ValueT>
        bool operator()(nvp<PolicyT, KeyT, ValueT>& nvp){
            return true;
        }
    };


#ifdef WITH_JSON_NLOHMANN

    template <typename DataT>
    struct to_json_f{
        to_json_f(nlohmann::json& root, const DataT& data): _root(root), _data(data) {}

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, _data);
            auto value    = function();
            _root.push_back({nvp.name(), udho::view::data::to_json(value)});
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, ValueT>& nvp){
            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, _data);
            _root.push_back({nvp.name(), function()});
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp){
            auto wrapper  = *nvp.value().getter();
            auto function = std::bind(wrapper, _data);
            _root.push_back({nvp.name(), function()});
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<policies::function, KeyT, ValueT>& nvp){
            return false;
        }

        nlohmann::json& _root;
        const DataT& _data;
    };

    template <typename DataT>
    struct from_json_f{
        from_json_f(const nlohmann::json& root, DataT& data): _root(root), _data(data) {}

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, ValueT>& nvp){
            using result_type = typename ValueT::result_type;

            result_type v = _root[nvp.name()].template get<result_type>();

            auto wrapper  = *nvp.value();
            auto function = std::bind(wrapper, &_data);
            function() = v;
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, ValueT>& nvp){
            return false;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, ValueT>& nvp){
            using result_type = typename ValueT::result_type;

            result_type v = _root[nvp.name()].template get<result_type>();

            auto wrapper  = *nvp.value().setter();
            auto function = std::bind(wrapper, &_data, v);
            function();
            return true;
        }

        template <typename KeyT, typename ValueT>
        bool operator()(nvp<policies::function, KeyT, ValueT>& nvp){
            return false;
        }

        const nlohmann::json& _root;
        DataT& _data;
    };

#endif
}

}
}
}


#endif // UDHO_VIEW_DATA_DETAIL_H
