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
#include <udho/view/sections.h>
#include <udho/view/bridges/lua/script.h>
#include <udho/view/bridges/lua/buffer.h>
#include <udho/view/bridges/lua/binder.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

template <typename DerivedT, typename ScriptT>
struct common;

template <typename DerivedT, typename T>
struct bindings{
    template <typename D, typename ScriptT>
    friend struct common;

    static bool exists() { return _exists; }
    private:
        static bool _exists;
};

template <typename DerivedT, typename T>
bool bindings<DerivedT, T>::_exists = false;

/**
 * @brief Represents a bridge for scripting functionalities within the system.
 *
 * The `common` bridge is designed to serve as a core component that facilitates
 * the interaction between native C++ code and script execution. It acts as a generic
 * interface for managing scripts in various programming environments, offering
 * capabilities to initialize, bind, create, compile, execute, and check the existence
 * of scripts.
 */
template <typename DerivedT, typename ScriptT>
struct common{
    /// Represents the type of script handled by the bridge.
    using script_type  = ScriptT;
    using derived_type = DerivedT;

    common(): _initialized(false) {}

    /**
     * @brief Initializes the bridge.
     *
     * This function should be called to prepare the bridge for operation, setting up
     * any necessary states or resources required for script handling.
     */
    void init(){
        if(!_initialized){
            self().init();
            _initialized = true;
        }
    }

    /**
     * @brief Binds a class to the bridge.
     *
     * This template function allows a class to be registered with the scripting
     * environment, enabling its methods and properties to be accessible from scripts.
     *
     * @tparam ClassT The type of the class to bind.
     * @param handle An identifier for the class in the scripting environment.
     */
    template <typename ClassT>
    void bind(udho::view::data::type<ClassT> handle){
        if(!bindings<DerivedT, ClassT>::exists()){
            self().bind(handle);
            bindings<DerivedT, ClassT>::_exists = true;
        }
    }

    /**
     * @brief Creates a new script.
     *
     * This function is responsible for creating a new script instance which can be compiled
     * and executed.
     *
     * @param name The name identifier for the new script.
     * @return A script_type object representing the newly created script.
     */
    script_type create(const std::string& name);

    /**
     * @brief Compiles a script.
     *
     * Compiles the script identified by the provided script object. The prefix is used
     * to resolve any dependencies or includes within the script content.
     *
     * @param script A reference to the script object to compile.
     * @param prefix A string used as a prefix to modify or resolve script dependencies.
     * @return true if compilation is successful, false otherwise.
     */
    bool compile(script_type& script, const std::string& prefix);

    /**
     * @brief Executes a compiled script.
     *
     * Executes the script identified by name, with the provided data. The execution result
     * is stored in the output parameter.
     *
     * @tparam T The data type to be passed to the script during execution.
     * @param name The name of the script to execute.
     * @param prefix A prefix used to locate the script.
     * @param data The data to pass to the script.
     * @param output A reference to a string where the script output will be stored.
     * @return The size of the output generated by the script.
     */
    template <typename T>
    std::size_t exec(const std::string& name, const std::string& prefix, const T& data, std::string& output);

    /**
     * @brief Checks if a compiled script exists.
     *
     * Determines whether a script with the given name and prefix has been compiled and is
     * available for execution.
     *
     * @param name The name of the script to check.
     * @param prefix The prefix used to locate the script.
     * @return true if the script exists, false otherwise.
     */
    bool exists(const std::string& name, const std::string& prefix);

    private:
        derived_type& self() { return static_cast<derived_type&>(*this); }
    private:
        bool _initialized;
};

}
}
}
}
#endif // UDHO_VIEW_BRIDGES_COMMON_BRIDGE_H
