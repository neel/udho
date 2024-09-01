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

#endif // UDHO_VIEW_META_META_H
