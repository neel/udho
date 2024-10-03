#ifndef UDHO_VIEW_META_EXECUTOR_H
#define UDHO_VIEW_META_EXECUTOR_H

#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <udho/view/meta/detail/parser.h>
#include <udho/view/meta/detail/visitor.h>

namespace udho{
namespace view{
namespace data{
namespace meta{

namespace detail{

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
    struct value_writer{
        value_writer(const Value& value): _value(value), _assigned(false) {}

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

    template <typename DataT, typename Function, bool ReEntrant>
    struct basic_executor{
        using function_type = Function;
        using finder_type   = visitor<DataT, function_type>;
        using meta_type     = decltype(metatype(std::declval<udho::view::data::type<DataT>>()));
        using node_ptr_type = ast::node_ptr_type;

        basic_executor(const std::string& syntax, DataT& data, function_type& function): _syntax(syntax), _ast(_syntax), _data(data), _meta(metatype(udho::view::data::type<DataT>{})), _function(function), _grammar(_ast.root()->children[0]) {
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
                _meta.members().apply_until(finder);

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
    using writer   = basic_executor<DataT, value_writer<ValueT>, false>;

}

}
}
}
}

#endif // UDHO_VIEW_META_EXECUTOR_H
