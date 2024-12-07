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
    /**
     * @brief writter callback passed to the visitor.
     * Used for writting an input value to a property in the data object.
     * @tparam Value type of the input.
     */
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
        /**
         * @brief checks whether the value was successfully read or not.
         */
        bool assigned() const { return _assigned; }

        private:
            Ret& _ret;
            bool _assigned;
    };

    /**
     * @brief writter callback passed to the visitor.
     * Used for extracting a property of data or getting return value from a function call in data object and store it on a target variable.
     * @tparam Value type of the input.
     */
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
        /**
         * @brief checks whether the value was successfully written or not.
         */
        bool assigned() const { return _assigned; }

        private:
            const Value& _value;
            bool _assigned;
    };

    /**
     * @brief NoOP callback passed to the visitor.
     * Used when the purpose of executing the meta code is not to get or set any external variable but to mutate the existing data object.
     */
    struct value_noop{
        template <typename T>
        bool operator()(const T&){ return false; }
    };

    /**
     * @brief executes meta syntax. Besade on the instructions provided in the meta syntax, calls the callback with the relevant, extracted or derived information from the data object.
     * 1. Parses the input syntax into @ref ast
     * 2. Creates metatype for the data provided
     * 3. apply visitor on the root node of the AST.
     *
     * Different executors pass different callbacks to this basic_executor. Only @ref value_noop callback is Re Entrant, which ignores all inputs passed to it.
     *
     * @tparam DataT Data type of the provided data
     * @tparam Function type of the callback which is supposed to be called with extracted data from the meta type
     * @tparam ReEntrant Boolean flag to state whether passed callback is reentrant or not.
     */
    template <typename DataT, typename Function, bool ReEntrant>
    struct basic_executor{
        using function_type = Function;
        using finder_type   = visitor<DataT, function_type>;
        using meta_type     = decltype(metatype(std::declval<udho::view::data::type<DataT>>()));
        using node_ptr_type = ast::node_ptr_type;

        /**
         * @brief construct a basic executor
         *
         * @param syntax the meta code.
         * @param data the data object which is either to be modified or from which relevant information has to be extracted.
         * @param function the callback which will be called with the extracted information or reference to the extracted part of the object.
         */
        basic_executor(const std::string& syntax, DataT& data, function_type& function): _syntax(syntax), _ast(_syntax), _data(data), _meta(metatype(udho::view::data::type<DataT>{})), _function(function), _grammar(_ast.root()->children[0]) {
            assert(_grammar->template is_type<ast::grammar>());
            assert(_grammar->children.size() > 0);
        }

        /**
         * @brief applies a visitor on the root node of the ast obtained from the meta code.
         */
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

    /**
     * @brief executor to execute meta code that mutates the data object passed to it while the values are part of of the input meta code
     */
    template <typename DataT>
    using executor = basic_executor<DataT, value_noop, true>;
    /**
     * @brief executor to execute meta code intended to read some value from the meta object and set it to some variable passed by the user code
     */
    template <typename DataT, typename ValueT>
    using reader   = basic_executor<DataT, value_reader<ValueT>, false>;
    /**
     * @brief executor to execute meta code intended to write a value to some value to the meta object
     */
    template <typename DataT, typename ValueT>
    using writer   = basic_executor<DataT, value_writer<ValueT>, false>;

}

}
}
}
}

#endif // UDHO_VIEW_META_EXECUTOR_H
