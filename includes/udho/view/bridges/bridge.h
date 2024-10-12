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

#ifndef UDHO_VIEW_BRIDGES_COMMON_BRIDGE_H
#define UDHO_VIEW_BRIDGES_COMMON_BRIDGE_H

#include <string>
#include <vector>
#include <functional>
#include <sol/sol.hpp>
#include <udho/url/detail/format.h>
#include <udho/url/summary.h>
#include <udho/view/tmpl/sections.h>
#include <udho/view/tmpl/parser.h>
#include <udho/view/bridges/lua/script.h>
#include <udho/view/bridges/lua/buffer.h>
#include <udho/view/bridges/lua/binder.h>
#include <udho/view/resources/resource.h>

namespace udho{
namespace view{
namespace data{

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
 */
template <typename BridgeT, typename ClassT>
struct bind{
    using state_type  = typename BridgeT::state_type;
    using class_type  = ClassT;
    using binder_type = typename BridgeT::template default_binder_type<class_type>;

    static void apply(state_type& state){
        binder_type::apply(state, udho::view::data::type<class_type>{});
    }
};

namespace bridges{

/**
 * @brief a subset of information regarding the global state, environment and context of invocation (e.g. HTTP request, URL routes summary, resource dictionary etc..)
 */
// struct context{
//     const udho::url::summary::router& _router_summary;
// };

/**
 * @brief supposed to be instantiated by the bridge itself for binding any type with that bridge.
 * @warning Do not specialize. Not intended to be used directly by the user code.
 * @details checks whether the type is already bound or not. If not then forwards to @ref udho::view::data::bind
 * @details borrows state. does not own anything.
 */
template <typename BridgeT>
struct bind{
    using state_type  = typename BridgeT::state_type;

    bind(state_type& state): _state(state) {}

    template <typename ClassT>
    void operator()(udho::view::data::type<ClassT> handle){
        if(!udho::view::data::bindings<state_type, ClassT>::exists()){
            udho::view::data::bind<BridgeT, ClassT>::apply(_state);
            udho::view::data::bindings<state_type, ClassT>::_exists = true;
        }
    }

    private:
        state_type& _state;
};

/**
 * @class bridge
 * @brief Manages the compilation and execution of scripts within a template engine framework.
 *
 * This template class binds scripting functionality with a state management system, allowing for dynamic compilation and execution of templates.
 *
 * @tparam StateT The type representing the scripting engine's state.
 * @tparam CompilerT The compiler used to compile scripts.
 * @tparam ScriptT The type of script being compiled and executed.
 * @tparam BinderT A template template parameter representing the binder used for data bindings.
 */
template <typename StateT, typename CompilerT, typename ScriptT, template <class> typename BinderT>
struct bridge{
    using state_type    = StateT;
    using compiler_type = CompilerT;
    using script_type   = ScriptT;
    template <typename X>
    using binder_type   = BinderT<X>;
    template <typename X>
    using default_binder_type = udho::view::data::detail::binder<BinderT, X>;

    static constexpr auto name() { return state_type::name(); }

    /**
     * @brief Initializes the scripting engine state.
     */
    void init(){
        _state.init();
    }

    /**
     * @brief Binds a data type to the scripting engine, enabling data access within the scripts generated from the templates (view files).
     * @tparam ClassT The data type to bind.
     * @param handle A handle representing the data type.
     */
    template <typename ClassT>
    void bind(udho::view::data::type<ClassT> handle){
        // udho::view::data::binder<BinderT, ClassT>::apply(_state, handle);

        using self_type = bridge<StateT, CompilerT, ScriptT, BinderT>;

        udho::view::data::bridges::bind<self_type> binder{_state};
        binder(handle);
    }

    /**
     * @brief Compiles a template (view) from a resource buffer into a script.
     * @param view The resource buffer containing the template data.
     * @param prefix A prefix used in the naming of the script.
     * @return True if compilation was successful, false otherwise.
     */
    template <typename IteratorT>
    bool compile(resources::resource_buffer<udho::view::resources::type::view, IteratorT>&& view, const std::string& prefix){
        std::string key = view_key(view.name(), prefix);
        return compile(view.begin(), view.end(), key);
    }

    /**
     * @brief Compiles a template (view) from a resource file into a script.
     * @param view The resource file containing the template data.
     * @param prefix A prefix used in the naming of the script.
     * @return True if compilation was successful, false otherwise.
     */
    bool compile(resources::resource_file<udho::view::resources::type::view>&& view, const std::string& prefix){
        std::string key = view_key(view.name(), prefix);
        return compile(view.begin(), view.end(), key);
    }

    /**
     * @brief Compiles a template from a resource file into a script.
     * @param view The resource file containing the template data.
     * @param prefix A prefix used in the naming of the script.
     * @return True if compilation was successful, false otherwise.
     */
    template <typename T, typename Aux>
    std::size_t exec(const std::string& name, const std::string& prefix, const T& data, const Aux& aux, std::string& output){
        if(!udho::view::data::bindings<StateT, T>::exists()){
            bind(udho::view::data::type<T>{});
        }
        if(!udho::view::data::bindings<StateT, Aux>::exists()){
            bind(udho::view::data::type<Aux>{});
        }
        return _state.exec(view_key(name, prefix), std::ref(data), std::ref(aux), output);
    }

    private:
        /**
         * @brief Generates a key for identifying a view script.
         * @param name The name of the view.
         * @param prefix The prefix used in the naming.
         * @return The generated view key.
         */
        std::string view_key(const std::string& name, const std::string& prefix) const {
            return udho::url::format(":{}/{}", prefix, name);
        }
        /**
         * @brief Compiles a script from a range of iterators that encapsulate template data.
         * @details This function takes iterators pointing to the beginning and end of a template, generates a script using the specified script handler, and attempts to compile this script using the scripting engine associated with this bridge.
         *          It processes the template data, transforms it into the scripting language using the provided script handler, and manages the compilation through the scripting engine's compiler.
         * @param begin Iterator to the beginning of the template data.
         * @param end Iterator to the end of the template data.
         * @param key A unique key or identifier for the compiled script, used to store and reference the script within the scripting engine.
         * @return True if the compilation was successful, false otherwise.
         * @tparam IteratorT The type of the iterator (e.g., string iterator, file buffer iterator).
         */
        template <typename IteratorT>
        bool compile(IteratorT begin, IteratorT end, std::string key){
            script_type script{key};
            udho::view::tmpl::parser parser;
            parser.parse(begin, end, script);
            script.finish();

            std::string name = script.save();
            std::cout << "Generated script at " << name << std::endl;
            compiler_type compiler{_state};
            return compiler(std::move(script));
        }
    private:
        StateT _state;

};

}
}
}
}
#endif // UDHO_VIEW_BRIDGES_COMMON_BRIDGE_H
