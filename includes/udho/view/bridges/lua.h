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

#ifndef UDHO_VIEW_BRIDGES_LUA_H
#define UDHO_VIEW_BRIDGES_LUA_H

#include <udho/view/bridges/lua/state.h>
#include <udho/view/bridges/lua/compiler.h>
#include <udho/view/bridges/lua/script.h>
#include <udho/view/bridges/bridge.h>
#include <udho/view/resources/store.h>
#include <fmt/core.h>
#include <fmt/args.h>
#include <tabulate/table.hpp>
#include <udho/view/bridges/lua/binder.h>
#include <udho/manifold/context.h>
#include <udho/www/components/fwd.h>
#include <udho/www/features.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

/**
 * @typedef lua
 * @ingroup view
 * @brief A specialized bridge configured for Lua scripting.
 *
 * This type alias represents lua specific instantiation of the `bridge` template, configured to use Lua-specific components for state management, scripting, and binding. It encapsulates the interaction between template parsing, Lua script generation, and execution, providing a streamlined interface for integrating Lua scripting into the template engine.
 *
 * @details The lua type is defined using several Lua-specific components:
 * - `detail::lua::state`: Manages the state specific to Lua scripts, such as global variables and function registrations.
 * - `detail::lua::compiler`: Responsible for compiling Lua scripts into a form that can be executed by the Lua interpreter.
 * - `detail::lua::script`: Handles the generation of Lua scripts from parsed template sections.
 * - `detail::lua::binder`: Provides mechanisms to bind C++ data structures to Lua scripts, enabling data exchange between C++ and Lua.
 */
using lua = udho::view::data::bridges::bridge<
                detail::lua::state,
                detail::lua::compiler,
                detail::lua::script,
                detail::lua::binder
            >;

}
}
}
}

namespace udho::view::data{

    template <>
    struct bind<bridges::lua, udho::view::resources::asset::type>{
        using state_type  = typename bridges::lua::state_type;
        using enum_type  = udho::view::resources::asset::type;
        using binder_type = typename bridges::lua::template default_binder_type<enum_type>;

        static void apply(state_type& state){
            // using user_type = sol::usertype<enum_type>;

            std::cout << "udho::view::data::bind<lua, udho::view::resources::asset::type>>: binding" << std::endl;

            state.udho().new_enum<enum_type>("asset_type", {
                    {"js",  enum_type::js},
                    {"css", enum_type::css},
                    {"txt", enum_type::txt},
                    {"img", enum_type::img}
                });
        }
    };

    template <>
    struct bind<bridges::lua, udho::url::summary::mount_point::url_proxy>{
        using state_type  = typename bridges::lua::state_type;
        using class_type  = udho::url::summary::mount_point::url_proxy;
        using binder_type = typename bridges::lua::template default_binder_type<class_type>;

        static void apply(state_type& state){
            using user_type = sol::usertype<class_type>;

            std::cout << "udho::view::data::bind<lua, url_proxy>: binding" << std::endl;

            user_type type = state.udho().new_usertype<class_type>("url_proxy",
                "new", sol::no_constructor,
                "pattern", sol::property(&class_type::pattern)
            );

            type.set_function("replace", [](const class_type& self, sol::variadic_args va) {
                return replacement(self, va);
            });

            type.set_function("_call", [](const class_type& self, sol::variadic_args va) {
                return replacement(self, va);
            });
        }

        private:
            static std::string replacement(const class_type& self, sol::variadic_args va){
                fmt::dynamic_format_arg_store<fmt::format_context> store;
                for (auto v : va) {
                    if (v.is<int>()) {
                        store.push_back(v.as<int>());
                    } else if (v.is<double>()) {
                        store.push_back(v.as<double>());
                    } else if (v.is<bool>()) {
                        store.push_back(v.as<bool>());
                    } else {
                        store.push_back(v.as<std::string>());
                    }
                }
                return fmt::vformat(self.pattern(), store);
            }
    };

    namespace detail{

    // template <typename... Bridges>
    // struct bind_helper_bridges;

    // template <typename Bridge>
    // struct bind_helper_bridge;

    // template <typename Component, typename... Components>
    // struct bind_helper_bridges<Component, Components...>: private bind_helper_bridges<Components...>{
    //     template <typename ClassT>
    //     static void apply(sol::usertype<ClassT>& utype) {
    //         bind_helper_bridge<Component>::apply(utype);
    //         bind_helper_bridges<Components...>::apply(utype);
    //     }
    // };

    // template <>
    // struct bind_helper_bridges<>{
    //     template <typename ClassT>
    //     static void apply(sol::usertype<ClassT>& utype) {}
    // };

    // template <>
    // struct bind_helper_bridge<udho::view::data::bridges::lua>{
    //     template <typename ClassT>
    //     static void apply(sol::usertype<ClassT>& utype) {
    //         utype.set_function("view", [](ClassT& self, const std::string& prefix, const std::string& name) -> detail::proxy_wrapper<udho::view::data::bridges::lua, ClassT> {
    //             return detail::proxy_wrapper<udho::view::data::bridges::lua, ClassT>{self, self.template view<udho::view::data::bridges::lua>(prefix, name)};
    //         });
    //     }
    // };

    template <typename... Components>
    struct bind_helper;

    template <typename Component>
    struct bind_helper_component{
        template <typename ClassT>
        static void apply(sol::usertype<ClassT>& utype) {}
    };

    template <typename Component, typename... Components>
    struct bind_helper<Component, Components...>: private bind_helper<Components...>{
        template <typename ClassT>
        static void apply(sol::usertype<ClassT>& utype) {
            bind_helper_component<Component>::apply(utype);
            bind_helper<Components...>::apply(utype);
        }
    };

    template <>
    struct bind_helper<>{
        template <typename ClassT>
        static void apply(sol::usertype<ClassT>& utype) {}
    };

    template <typename... Bridges>
    struct bind_helper_component<udho::www::components::resources<Bridges...>>{
        template <typename ClassT>
        static void apply(sol::usertype<ClassT>& utype) {
            utype.set("resources", sol::property([](ClassT& portal) {
                return portal.resources();
            }));
            // bind_helper_bridges<Bridges...>::apply(utype);
        }
    };

    template <typename Router>
    struct bind_helper_component<udho::www::components::routing<Router>>{
        template <typename ClassT>
        static void apply(sol::usertype<ClassT>& utype) {
            utype.set("routes", sol::property([](ClassT& portal) {
                return portal.routes();
            }));
        }
    };

    /**
     * @brief A proxy around the context used internally to make a view accessible conveniently
     * @tparam XBridgeT the bridge on which the intended view is registered
     * @tparam Bridges...  The bridges supported by the context
     */
    template <typename XBridgeT, typename ContextT>
    struct proxy_wrapper{
        using context_type = ContextT;
        using proxy_type   = udho::view::resources::tmpl::proxy<XBridgeT>;
        using self_type    = proxy_wrapper<XBridgeT, ContextT>;

        proxy_wrapper() = delete;
        proxy_wrapper(const proxy_wrapper&) = delete;
        proxy_wrapper(const context_type& ctx, proxy_type&& proxy): _ctx(ctx), _proxy(std::move(proxy)) {}
        proxy_wrapper(proxy_wrapper&& other): _ctx(other._ctx), _proxy(std::move(other._proxy)) {}


        /**
     * @brief Returns the name of the resource associated with this proxy.
     * @return The name of the resource.
     */
        inline std::string name() const { return _proxy.name(); }

        /**
     * @brief Returns the prefix of the resource associated with this proxy.
     * @return The prefix of the resource.
     */
        inline std::string prefix() const { return _proxy.prefix(); }

        /**
     * @brief Returns const reference to the context
     * @return context
     */
        const context_type& context() const { return _ctx; }

        friend auto metatype(udho::view::data::type<self_type>){
            using namespace udho::view::data;

            return assoc("view_proxy_wrapper"),
                   fvar("name",   &self_type::name),
                   fvar("prefix", &self_type::prefix);
        }

    private:
        const context_type& _ctx;
        proxy_type    _proxy;
    };

    }

    template <typename... Components>
    struct bind<bridges::lua, udho::manifold::portal<Components...>>{
        using state_type  = typename bridges::lua::state_type;
        using class_type  = udho::manifold::portal<Components...>;
        using binder_type = typename bridges::lua::template default_binder_type<class_type>;

        static void apply(state_type& state){
            using user_type = sol::usertype<class_type>;

            std::cout << "udho::manifold::portal<Components...>: binding" << std::endl;

            sol::automagic_enrollments enrollments{false, false, false, false, false, false, false, false, false};
            user_type type = state.udho().new_usertype<class_type>("portal", enrollments);

            detail::bind_helper<Components...>::apply(type);
        }

    };

    template <typename... Bridges>
    struct bind<bridges::lua, udho::view::resources::const_store<Bridges...>>{
        using state_type  = typename bridges::lua::state_type;
        using class_type  = udho::view::resources::const_store<Bridges...>;
        using binder_type = typename bridges::lua::template default_binder_type<class_type>;

        static void apply(state_type& state){
            using user_type = sol::usertype<class_type>;

            std::cout << "udho::view::data::bind<lua, udho::view::resources::const_store<...>>: binding" << std::endl;

            // first bind according to the metatype
            typename binder_type::foreign_binder_type binder = binder_type::apply(state, udho::view::data::type<class_type>{});

            // // then add lua specific functionalities
            // user_type& type = binder.type();

            // type.set_function("view", [](const class_type& self, const std::string& prefix, const std::string& name){
            //     return self.template view<bridges::lua>(prefix, name);
            // });
        }
    };

    template <typename StreamT, typename... Components>
    struct bind<bridges::lua, udho::manifold::basic_context<StreamT, Components...>>{
        using state_type                = typename bridges::lua::state_type;
        using class_type                = udho::manifold::basic_context<StreamT, Components...>;
        using binder_type               = typename bridges::lua::template default_binder_type<class_type>;
        using context_type              = class_type;
        using portal_type               = typename context_type::portal_type;
        using composition_type          = typename portal_type::composition_view_type;
        using resource_component_type   = typename composition_type::template component_at<udho::www::feature::resources_storage, 0>;
        using store_type                = typename resource_component_type::store_type;

        static void apply(state_type& state){
            using user_type = sol::usertype<class_type>;

            std::cout << "udho::view::data::bind<lua, udho::manifold::basic_context<...>>: binding" << std::endl;

            {
                udho::view::data::bridges::bind<bridges::lua> binder{state};
                binder(udho::view::data::type<udho::view::resources::tmpl::proxy<bridges::lua>>{});
                binder(udho::view::data::type<udho::view::resources::const_store<bridges::lua>>{});
                binder(udho::view::data::type<detail::proxy_wrapper<bridges::lua, class_type>>{});
                binder(udho::view::data::type<portal_type>{});
            }

            sol::automagic_enrollments enrollments{false, false, false, false, false, false, false, false, false};
            user_type type = state.udho().new_usertype<class_type>("context", enrollments);

            type.set("portal", sol::property([](class_type& context) {
                return context.portal();
            }));

            type.set_function("view", [](class_type& self, const std::string& prefix, const std::string& name) -> detail::proxy_wrapper<bridges::lua, class_type> {
                return detail::proxy_wrapper<udho::view::data::bridges::lua, class_type>{self, self.portal().template view<udho::view::data::bridges::lua>(prefix, name)};
            });

            // first bind according to the metatype
            // typename binder_type::foreign_binder_type binder = binder_type::apply(state, udho::view::data::type<class_type>{});
            // store_type::bind(state);

            // then add lua specific functionalities
            // user_type& type = binder.type();
        }
    };

    template <typename ContextT>
    struct bind<bridges::lua, detail::proxy_wrapper<bridges::lua, ContextT>>{
        using state_type  = typename bridges::lua::state_type;
        using class_type  = detail::proxy_wrapper<bridges::lua, ContextT>;
        using binder_type = typename bridges::lua::template default_binder_type<class_type>;

        static void apply(state_type& state){
            using user_type = sol::usertype<class_type>;

            std::cout << "udho::view::data::bind<lua, detail::proxy_wrapper<bridges::lua, ContextT>>: binding" << std::endl;

            // first bind according to the metatype
            typename binder_type::foreign_binder_type binder = binder_type::apply(state, udho::view::data::type<class_type>{});

            // then add lua specific functionalities
            user_type& type = binder.type();

            type.set_function("render", sol::overload(
                [&state](const class_type& self) mutable -> std::string {
                    try {
                        std::string view_key = bridges::common::view_key(self.name(), self.prefix());
                        return state.exec_lua(view_key, sol::nil, self.context());
                    } catch (const std::exception& e) {
                        // If there is an error, throw Lua exception with the error message
                        throw sol::error(e.what());
                    }
                },
                [&state](const class_type& self, sol::object d) mutable -> std::string {
                    try {
                        std::string view_key = bridges::common::view_key(self.name(), self.prefix());
                        return state.exec_lua(view_key, d, self.context());
                    } catch (const std::exception& e) {
                        // If there is an error, throw Lua exception with the error message
                        throw sol::error(e.what());
                    }
                },
                [&state](const class_type& self, sol::object d, udho::view::data::bridges::detail::lua::buffer& stream) mutable -> std::size_t {
                    try {
                        std::string view_key = bridges::common::view_key(self.name(), self.prefix());
                        std::string output = state.exec_lua(view_key, d, self.context());
                        return stream.puts(output);
                    } catch (const std::exception& e) {
                        // If there is an error, throw Lua exception with the error message
                        throw sol::error(e.what());
                    }
                }
            ));
        }
    };

    template <>
    struct bind<bridges::lua, udho::view::resources::tmpl::proxy<bridges::lua>>{
        using state_type  = typename bridges::lua::state_type;
        using class_type  = udho::view::resources::tmpl::proxy<bridges::lua>;
        using binder_type = typename bridges::lua::template default_binder_type<class_type>;

        static void apply(state_type& state){
            using user_type = sol::usertype<class_type>;

            std::cout << "udho::view::data::bind<lua, udho::view::resources::tmpl::proxy<bridges::lua>>: binding" << std::endl;

            // first bind according to the metatype
            typename binder_type::foreign_binder_type binder = binder_type::apply(state, udho::view::data::type<class_type>{});

            // then add lua specific functionalities
            user_type& type = binder.type();

            type.set_function("render", [&state](const class_type& self, sol::object d, sol::object aux) mutable {
                try {
                    std::string view_key = bridges::common::view_key(self.name(), self.prefix());
                    return state.exec_lua(view_key, d, aux);  // script_name should be replaced with actual script identifier
                } catch (const std::exception& e) {
                    // If there is an error, throw Lua exception with the error message
                    throw sol::error(e.what());
                }
            });
        }
    };

    template <>
    struct bind<bridges::lua, tabulate::Table>{
        using state_type  = typename bridges::lua::state_type;
        using class_type  = tabulate::Table;
        using binder_type = typename bridges::lua::template default_binder_type<class_type>;

        static void apply(state_type& state){
            using user_type = sol::usertype<class_type>;

            std::cout << "udho::view::data::bind<lua, tabulate::Table>>: binding" << std::endl;

            user_type type = state.udho().new_usertype<class_type>("Tabulate",
                "new", sol::constructors<class_type()>(),
                "str", &class_type::str,
                "__tostring", &class_type::str
            );
            type.set_function("add", [](class_type& self, sol::variadic_args va) {
                using row_type = tabulate::Table::Row_t;
                row_type row;

                for (auto v : va) {
                    if(v.is<tabulate::Table>()){
                        row.push_back("Unsupported");
                    } else {
                        std::string value = v.as<std::string>();
                        row.push_back(value);
                    }
                }
                self.add_row(row);
            });
        }
    };

}


#endif // UDHO_VIEW_BRIDGES_LUA_H

