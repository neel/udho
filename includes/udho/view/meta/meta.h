#ifndef UDHO_VIEW_META_META_H
#define UDHO_VIEW_META_META_H



#include <string>
#include <exception>
#include <udho/url/detail/format.h>
#include <udho/view/meta/detail/executor.h>

namespace udho{
namespace view{
namespace data{
namespace meta{

/**
 * @addtogroup DoxyG_view_tmpl_data
 * @{
 */

/**
 * @brief Executes a given operation on data.
 *
 * This function executes an operation specified by a syntax string.
 *
 * @tparam DataT The data type on which the operation is performed.
 * @param data Reference to the data on which the operation is performed.
 * @param syntax The operation to be performed, specified as a string.
 */
template <typename DataT>
void exec(DataT& data, const std::string& syntax){
    using executor_type = detail::executor<DataT>;
    using function_type = typename executor_type::function_type;

    function_type function;
    executor_type executor{syntax, data, function};
    executor();
}

/**
 * @brief Attempts to retrieve a value based on a given expression.
 *
 * This function template tries to get a value by executing a reader executor constructed with the given syntax.
 * It returns a boolean indicating success or failure.
 *
 * @tparam DataT The data type from which the value is read.
 * @tparam ValueT The type of value to be read.
 * @param data Reference to the data from which the value is read.
 * @param syntax The syntax specifying what value to read.
 * @param value Reference to store the read value if successful.
 * @return true if the value was successfully retrieved, false otherwise.
 */
template <typename DataT, typename ValueT>
bool get(DataT& data, const std::string& syntax, ValueT& value){
    using executor_type = detail::reader<DataT, ValueT>;
    using function_type = typename executor_type::function_type;

    function_type function{value};
    executor_type executor{syntax, data, function};
    executor();

    return function.assigned();
}

/**
 * @brief Retrieves a value based on a given syntax or throws an exception if unsuccessful.
 *
 * This function template retrieves a value by executing a reader executor. If the value cannot be successfully retrieved, it throws a runtime_error.
 *
 * @tparam ValueT The type of value to be retrieved.
 * @tparam DataT The data type from which the value is read.
 * @param data Reference to the data from which the value is read.
 * @param syntax The syntax specifying what value to retrieve.
 * @return The retrieved value of type ValueT.
 * @throws std::runtime_error If the value could not be assigned.
 */
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

/**
 * @brief Sets a value on the data based on a given syntax.
 *
 * This function template attempts to set a value by executing a writer executor constructed with the given syntax. It returns a boolean indicating if the value was successfully set.
 *
 * @tparam DataT The data type on which the value is set.
 * @tparam ValueT The type of value to be set.
 * @param data Reference to the data on which the value is set.
 * @param syntax The syntax specifying where to set the value.
 * @param value The value to set.
 * @return true if the value was successfully set, false otherwise.
 */
template <typename DataT, typename ValueT>
bool set(DataT& data, const std::string& syntax, const ValueT& value){
    using executor_type = detail::writer<DataT, ValueT>;
    using function_type = typename executor_type::function_type;

    function_type function{value};
    executor_type executor{syntax, data, function};
    executor();

    return function.assigned();
}

/** @} */

}
}
}
}



#endif // UDHO_VIEW_META_META_H
