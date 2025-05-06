#ifndef UDHO_VIEW_DATA_DETAIL_BIND_H
#define UDHO_VIEW_DATA_DETAIL_BIND_H

#include <udho/view/data/type.h>

namespace udho{
namespace view{
namespace data{

namespace detail{

template <typename BridgeT, typename ClassT, bool Enable = udho::view::data::has_metatype<ClassT>::value>
struct bind{
    using state_type  = typename BridgeT::state_type;
    using class_type  = ClassT;
    using binder_type = typename BridgeT::template default_binder_type<class_type>;

    static void apply(state_type& state){
        binder_type::apply(state, udho::view::data::type<class_type>{});
    }
};

template <typename BridgeT, typename ClassT>
struct bind<BridgeT, ClassT, false>{
    using state_type  = typename BridgeT::state_type;
    using class_type  = ClassT;

    static void apply(state_type&){}
};

}

/**
 * @class bind
 * @brief default binder for a bridge that binds a given class with the bridge by using the metatype.
 * This template uses the default implementation which expects that the class has a metatype friend function overloaded.
 * @details the default binder is in @ref udho::view::data::detail::binder
 * @tparam BridgeT foreign language bridge, usually a parameterization of @ref udho::view::data::bridges::bridge
 * @tparam ClassT  the C++ struct/class which is supposed to be bound with the bridge
 *
 * Following is an example for specialization
 * @code
 * #include <udho/view/bridges/bridge.h>
 * #include <udho/view/bridges/lua.h>
 *
 * namespace udho::view::data{
 *
 * template <>
 * struct bind<bridges::lua, udho::url::summary::mount_point::url_proxy>{
 *
 *      using state_type  = typename bridges::lua::state_type;
 *      using class_type  = udho::url::summary::mount_point::url_proxy;
 *      using binder_type = typename bridges::lua::template default_binder_type<class_type>;
 *
 *      static void apply(state_type& state){
 *          // The default implementation is as follows
 *          // binder_type::apply(state, udho::view::data::type<class_type>{});
 *
 *          // or write your own implementation
 *          // following is an example
 *
 *          user_type type = state.udho().new_usertype<class_type>("url_proxy",
 *              "new", sol::no_constructor
 *          );
 *      }
 * };
 *
 * }
 * @endcode
 * @ingroup view
 */
template <typename BridgeT, typename ClassT>
struct bind: detail::bind<BridgeT, ClassT>{};


}
}
}

#endif // UDHO_VIEW_DATA_DETAIL_BIND_H
