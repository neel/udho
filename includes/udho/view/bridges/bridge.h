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
#include <queue>
#include <thread>
#include <mutex>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <memory>
#include <udho/url/detail/format.h>
#include <udho/url/summary.h>
#include <udho/view/tmpl/sections.h>
#include <udho/view/tmpl/parser.h>
#include <udho/view/bridges/lua/script.h>
#include <udho/view/bridges/lua/buffer.h>
#include <udho/view/bridges/lua/binder.h>
#include <udho/view/bridges/results.h>
#include <udho/view/resources/resource.h>

// #define UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE 1

#if (UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE)
#include <boost/lockfree/queue.hpp>
#endif

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

    static void apply(state_type& state){}
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


namespace bridges{

/**
 * @enum policy
 * @brief Defines the synchronization policies for handling execution in different concurrency environments.
 *
 * This enum class provides identifiers for selecting the appropriate synchronization mechanisms
 * based on the operational needs and threading environment of the application.
 */
enum class policy{
    no_policy,
    /**
     * @brief Suitable for high-concurrency environments.
     *
     * Uses a fixed number of states on which the foreign functions will be executed.
     * Multiple threads calling exec function will either wait or get a free state allocated from the state pool.
     */
    state_pool,

    /**
     * @brief Provides thread safety using a single mutex.
     *
     * Doesn't maintain a pool of states, rather uses a single lock to synchronize execution of foreign function on the same state.
     */
    thread_safe,

    /**
     * @brief Intended for use in single-threaded or non-concurrent environments.
     *
     * Doesn't implement any synchronization mechanism, applicable for scenarios when then server is running on single threaded environment.
     */
    non_concurrent
};

/**
 * @brief common functionalities required by bridges of all languages
 * @ingroup view
 */
struct common{
    /**
     * @brief given a prefix and a view name construct a key against which the actual compipled view functions will be stored
     * @param name std::string view name
     * @param prefix std::string view prefix
     * @return returns the view key given the name and the prefix of the view
     */
    static std::string view_key(const std::string& name, const std::string& prefix) {
        return udho::url::format(":{}/{}", prefix, name);
    }
};

/**
 * @brief supposed to be instantiated by the bridge itself for binding any type with that bridge.
 * @warning Do not specialize. Not intended to be used directly by the user code.
 * @details checks whether the type is already bound or not. If not then forwards to @ref udho::view::data::bind
 * @details borrows state. does not own anything.
 * @ingroup view
 */
template <typename BridgeT>
struct bind{
    using state_type  = typename BridgeT::state_type;

    bind(state_type& state): _state(state) {}

    template <typename ClassT>
    void operator()(udho::view::data::type<ClassT>){
        // TODO now as we are dealing with multiple states we need to keep tract whether the type was bounded on a particular state or not.
        if(!udho::view::data::bindings<state_type, ClassT>::exists(_state)){
            udho::view::data::bind<BridgeT, ClassT>::apply(_state);
            udho::view::data::bindings<state_type, ClassT>::bound_one(_state);
        } else {
            // bindings already exists no need to do it again.
        }
    }

    private:
        state_type& _state;
};

/**
 * @class bridge
 * @ingroup view
 * @brief Manages the compilation and execution of scripts within a template engine framework.
 *
 * This template class binds scripting functionality with a state management system, allowing for dynamic compilation and execution of templates.
 *
 * @tparam StateT The type representing the scripting engine's state.
 * @tparam CompilerT The compiler used to compile scripts.
 * @tparam ScriptT The type of script being compiled and executed.
 * @tparam BinderT A template template parameter representing the binder used for data bindings.
 */
// template <typename StateT, typename CompilerT, typename ScriptT, template <class> typename BinderT>
// struct bridge{
//     using state_type    = StateT;
//     using compiler_type = CompilerT;
//     using script_type   = ScriptT;
//     template <typename X>
//     using binder_type   = BinderT<X>;
//     template <typename X>
//     using default_binder_type = udho::view::data::detail::binder<BinderT, X>;
//     using self_type = bridge<StateT, CompilerT, ScriptT, BinderT>;
//
//     static constexpr auto name() { return state_type::name(); }
//
//
//     /**
//      * @brief Initializes the scripting engine state.
//      */
//     void init(){
//         _state.init();
//     }
//
//     /**
//      * @brief Binds a data type to the scripting engine, enabling data access within the scripts generated from the templates (view files).
//      * @tparam ClassT The data type to bind.
//      * @param handle A handle representing the data type.
//      */
//     template <typename ClassT>
//     void bind(udho::view::data::type<ClassT> handle){
//         udho::view::data::bridges::bind<self_type> binder{_state};
//         binder(handle);
//     }
//
//     /**
//      * @brief Compiles a template (view) from a resource buffer into a script.
//      * @param view The resource buffer containing the template data.
//      * @param prefix A prefix used in the naming of the script.
//      * @return True if compilation was successful, false otherwise.
//      */
//     bool compile(udho::view::resources::tmpl::resource&& view, const std::string& prefix){
//         std::string key = view_key(view.name(), prefix);
//         return compile(view.begin(), view.end(), key);
//     }
//
//     /**
//      * @brief Compiles a template from a resource file into a script.
//      * @param view The resource file containing the template data.
//      * @param prefix A prefix used in the naming of the script.
//      * @return True if compilation was successful, false otherwise.
//      */
//     template <typename T, typename Aux>
//     udho::view::data::bridges::results exec(const std::string& name, const std::string& prefix, const T& data, const Aux& aux, std::string& output){
//         if(!udho::view::data::bindings<StateT, T>::exists()){
//             bind(udho::view::data::type<T>{});
//         }
//         if(!udho::view::data::bindings<StateT, Aux>::exists()){
//             bind(udho::view::data::type<Aux>{});
//         }
//         udho::view::data::bridges::results results;
//         results.start(0);
//         std::size_t size = _state.exec(view_key(name, prefix), std::ref(data), std::ref(aux), output);
//         results.finish(size);
//         return results;
//     }
//
//     private:
//         /**
//          * @brief Generates a key for identifying a view script.
//          * @param name The name of the view.
//          * @param prefix The prefix used in the naming.
//          * @return The generated view key.
//          */
//         std::string view_key(const std::string& name, const std::string& prefix) const {
//             return bridges::common::view_key(name, prefix);
//         }
//         /**
//          * @brief Compiles a script from a range of iterators that encapsulate template data.
//          * @details This function takes iterators pointing to the beginning and end of a template, generates a script using the specified script handler, and attempts to compile this script using the scripting engine associated with this bridge.
//          *          It processes the template data, transforms it into the scripting language using the provided script handler, and manages the compilation through the scripting engine's compiler.
//          * @param begin Iterator to the beginning of the template data.
//          * @param end Iterator to the end of the template data.
//          * @param key A unique key or identifier for the compiled script, used to store and reference the script within the scripting engine.
//          * @return True if the compilation was successful, false otherwise.
//          * @tparam IteratorT The type of the iterator (e.g., string iterator, file buffer iterator).
//          */
//         template <typename IteratorT>
//         bool compile(IteratorT begin, IteratorT end, std::string key){
//             script_type script{key};
//             udho::view::tmpl::parser parser;
//             parser.parse(begin, end, script);
//             script.finish();
//
//             std::string name = script.save(key);
//             std::cout << "Generated script at " << name << std::endl;
//             compiler_type compiler{_state};
//             return compiler(std::move(script));
//         }
//     private:
//         StateT _state;
//
// };

/**
 * @class bridge
 * @ingroup view
 * @brief Manages the compilation and execution of scripts within a template engine framework.
 *
 * This template class binds scripting functionality with a state management system, allowing for dynamic compilation and execution of templates.
 *
 * @tparam StateT The type representing the scripting engine's state.
 * @tparam CompilerT The compiler used to compile scripts.
 * @tparam ScriptT The type of script being compiled and executed.
 * @tparam BinderT A template template parameter representing the binder used for data bindings.
 *
 * @section synchronization Synchronization Overview
 * The `bridge` class employs a combination of mutexes and semaphores to manage synchronization between multiple threads, particularly focusing on the `bind` and `exec` functions.
 *
 * @subsection bind_function Bind Function
 * Binds a data type to all Lua states. This operation must have exclusive access to all Lua states because it modifies the state bindings that the exec function relies upon.
 *
 * @pre
 * No Lua state is currently executing a script.
 * All Lua states are available for modification.
 *
 * @post
 * All Lua states have the new data type bound.
 * All Lua states are again available for script execution.
 *
 * @par Strategy:
 * Uses a recursive mutex to ensure exclusive access across threads.
 * Uses a semaphore to ensure all Lua states are free and not in use.
 *
 * @par Entry Condition:
 * Acquires a recursive mutex to ensure exclusive access, preventing other threads from modifying Lua states during the binding process.
 *
 * @par Exit Condition:
 * Releases the mutex and all Lua states back to the pool for further use.
 *
 * @par Critical Section:
 * The function iterates over all Lua states, applying the binding operation. This section is protected using a recursive mutex and a semaphore is used to check state availability.
 *
 * @code
 * function bind(ClassT handle):
 *     // Entry: Ensure exclusive access to bind operation.
 *     lock mutex_bind recursively
 *
 *     // Ensure all Lua states are available.
 *     for i from 1 to pool_size:
 *         wait on semaphore_exec
 *
 *     // Critical Section: Binding operation.
 *     assert(free_queue.size() == pool_size)  // All states must be free.
 *     for each state in states:
 *         perform binding operation on state with handle
 *
 *     // Release all Lua states for use.
 *     for i from 1 to pool_size:
 *         post to semaphore_exec
 *
 *     // Exit: Release exclusive access lock.
 *     unlock mutex_bind
 * end function
 * @endcode
 *
 * @par Performance Assumptions:
 * This function is called infrequently relative to exec since bindings do not need to be updated often.
 * Can block exec operations temporarily (not forever) but guarantees system integrity by updating all states consistently.
 *
 * @subsection exec_function Exec Function
 * Executes a Lua script using an available Lua state, potentially performing bindings if they are not already in place.
 *
 * @pre
 * At least one Lua state is available for executing a script.
 *
 * @post
 * The script is executed on a Lua state, or an error is handled.
 * The Lua state used is released back into the pool for further use.
 *
 * @par Entry Condition:
 * Optionally acquires a recursive mutex if the required bindings are not already established.
 *
 * @par Exit Condition:
 * Ensures the Lua state is released back to the pool, even in cases of exceptions.
 *
 * @par Critical Section:
 * Script execution takes place within a try-catch block to handle any Lua or C++ exceptions, ensuring clean error handling and Lua state release.
 *
 * @par Synchronization Strategy:
 * Uses a recursive mutex to protect binding operations within the execution context.
 * Uses a semaphore to manage access to individual Lua states ensuring only available states are accessed.
 *
 * @code
 * function exec(T data, Aux aux, name, prefix):
 *     // Entry: Try to lock for binding if necessary, defer otherwise.
 *     if not binding_exists for T:
 *         lock mutex_bind recursively
 *         if not binding_exists for T:  // Double-checked locking
 *             bind data type T
 *         unlock mutex_bind
 *
 *     // Wait for an available Lua state.
 *     wait on semaphore_exec
 *     get an available state index from free_queue
 *
 *     // Critical Section: Execute the Lua script.
 *     try:
 *         execute Lua script on the state
 *     catch any exception:
 *         handle the exception and prepare error message
 *
 *     // Release the Lua state back to the pool.
 *     push state index back to free_queue
 *     post to semaphore_exec
 *
 *     // Exit: Lua state is released, and function ends.
 * end function
 * @endcode
 *
 * @par Expectations:
 * This function is expected to be called frequently.
 * It handles its own errors internally and ensures that even in case of failure, the Lua state is properly released.
 *
 * @par Caution:
 * Major parts of this above documentation was generated by ChatGPT 4.0 based on the code in the two functions, bind and exec.
 * Additionally, ChatGPT o1 was used to verify whether these two functions can lead to any race conditions in any circumstances.
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
    using self_type = bridge<StateT, CompilerT, ScriptT, BinderT>;


    static constexpr auto name() { return state_type::name(); }

    bridge(std::size_t states = 1, bridges::policy policy = bridges::policy::no_policy): _pool_size(states), _semaphore_exec(_pool_size)
#if (UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE)
    , _free_lf_q(_pool_size)
#endif
    {
        // { Preconditions and adjustments
        //   Number of states must be greater than or equal to 1
        //   If no_policy is provided then the default policy appropriate for the given number of ststes will be choosen.
        //   If number of states is set to 1 then thread_safe policy is used which uses a single mutex to guard the critical section
        //   However, if the user knows that there will be no parellal entry to the critical section (e.g. single threaded environment)
        //       then user can specify a different policy like non_concurrent for better performance
        //   If number of states is more than 1 then state_pool policy is choosen.
        assert(states > 0);
        if(states > 1){
            if(policy == bridges::policy::no_policy || policy == bridges::policy::state_pool){
                _policy = bridges::policy::state_pool;
            } else {
                throw std::runtime_error{"Specified policy is incompatiable with more than one bridge."};
            }
        } else {
            if(policy == bridges::policy::no_policy){
                _policy = bridges::policy::thread_safe;
            } else {
                _policy = policy;
            }
        }
#if (UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE)
        if(_policy! = bridges::policy::state_pool){
            throw std::runtime_error{"UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE is only applicable for state_pool policy"};
        }
#endif
        // }

        for (int i = 0; i < _pool_size; ++i) {
            auto state = std::make_unique<state_type>();
            _states.emplace_back(std::move(state));
        }
    }

    /**
     * @brief Initializes the scripting engine state.
     */
    void init(){
        std::size_t counter = 0;
        for (std::unique_ptr<state_type>& state: _states){
            state->init();
#if (UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE)
            _free_lf_q.push(counter);
#else
            _freeq.push(counter);
#endif

            ++counter;
        }
    }

    /**
     * @brief Compiles a template (view) from a resource buffer into a script.
     * @param view The resource buffer containing the template data.
     * @param prefix A prefix used in the naming of the script.
     * @return True if compilation was successful, false otherwise.
     */
    bool compile(udho::view::resources::tmpl::resource&& view, const std::string& prefix){
        std::string key = view_key(view.name(), prefix);
        return compile(view.begin(), view.end(), key);
    }

    /**
     * @brief Binds a data type to the scripting engine, enabling data access within the scripts generated from the templates (view files).
     * @tparam ClassT The data type to bind.
     * @param handle A handle representing the data type.
     */
    template <typename ClassT>
    void bind(udho::view::data::type<ClassT> handle){
        if(_policy == bridges::policy::non_concurrent) {
            bind_nolock(handle);
        } else if(_policy == bridges::policy::thread_safe){
            bind_mutex(handle);
        } else if(_policy == bridges::policy::state_pool) {
            bind_pool(handle);
        } else {
            assert(("no policy is det for the bridge", false));
        }
    }


    /**
     * @brief Compiles a template from a resource file into a script.
     * @param view The resource file containing the template data.
     * @param prefix A prefix used in the naming of the script.
     * @return True if compilation was successful, false otherwise.
     */
    template <typename T, typename Aux>
    udho::view::data::bridges::results exec(const std::string& name, const std::string& prefix, const T& data, const Aux& aux, std::string& output){
        if(_policy == bridges::policy::non_concurrent) {
            return exec_nolock(name, prefix, data, aux, output);
        } else if(_policy == bridges::policy::thread_safe){
            return exec_mutex(name, prefix, data, aux, output);
        } else if(_policy == bridges::policy::state_pool) {
            return exec_pool(name, prefix, data, aux, output);
        } else {
            assert(("no policy is det for the bridge", false));
        }
    }

    private:
        template <typename ClassT>
        void bind_nolock(udho::view::data::type<ClassT> handle){
            // { Enter CS
            // Assuming no parellal access to the CS will happen
            // }
            // { CS
            assert(_states.size() == 1);
            bind_cs(handle);
            // }

            // { Exit CS
            // No locking mechanism
            // }
        }

        template <typename T, typename Aux>
        udho::view::data::bridges::results exec_nolock(const std::string& name, const std::string& prefix, const T& data, const Aux& aux, std::string& output){
            udho::view::data::bridges::results results;
            {
                if(!udho::view::data::bindings<StateT, T>::exists(_states.size())){
                    bind_nolock(udho::view::data::type<T>{});
                }
            }
            // { Enter CS
            // Assuming no parellal access to the CS will happen
            // }

            // { CS
            assert(_states.size() == 1);
            std::size_t index = 0;
            std::unique_ptr<state_type>& state = _states[index];
            exec_cs(name, prefix, data, aux, output, state, results);
            // }

            // { Exit CS
            // No locking mechanism
            // }
            return results;
        }

        template <typename ClassT>
        void bind_mutex(udho::view::data::type<ClassT> handle){
            // { Enter CS
            std::lock_guard<std::recursive_mutex> lock(_mutex_bind);
            // }
            // { CS
            assert(_states.size() == 1);
            bind_cs(handle);
            // }

            // { Exit CS
            // RAII lock _mutex_bind unlocked
            // }
        }

        template <typename T, typename Aux>
        udho::view::data::bridges::results exec_mutex(const std::string& name, const std::string& prefix, const T& data, const Aux& aux, std::string& output){
            udho::view::data::bridges::results results;
            {
                std::unique_lock<std::recursive_mutex> lock(_mutex_bind, std::defer_lock);
                if(!udho::view::data::bindings<StateT, T>::exists(_states.size())){
                    // { Enter CS
                    lock.lock();
                    // }

                    // { CS
                    bind_mutex(udho::view::data::type<T>{});
                    // }

                    // { Exit CS
                    lock.unlock();
                    // }
                }
            }
            // { Enter CS
            std::lock_guard<std::recursive_mutex> lock(_mutex_bind);
            // }

            // { CS
            assert(_states.size() == 1);
            std::size_t index = 0;
            std::unique_ptr<state_type>& state = _states[index];
            results.start(index);
            exec_cs(name, prefix, data, aux, output, state, results);
            // }

            // { Exit CS
            // RAII lock _mutex_bind unlocked
            // }

            return results;
        }

        template <typename ClassT>
        void bind_pool(udho::view::data::type<ClassT> handle){
            // { Enter CS
            std::lock_guard<std::recursive_mutex> lock(_mutex_bind);
            for (int i = 0; i < _pool_size; ++i) {
                _semaphore_exec.wait();
            }
            // }

            // _semaphore_exec decremented to 0
            // _mutex_bind is either locked once or twice.
            // if once then bind was called directly from usercode.
            // if twice then both locks are from the same thread.
            // in either way all other threads calling exec or bind will wait.
    #if (UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE)
            // boost lock free queue doesn;t have a size method
    #else
            assert(_freeq.size() == _pool_size);
    #endif

            // { CS
            bind_cs(handle);
            // }

            // { Exit CS
            for (int i = 0; i < _pool_size; ++i) {
                _semaphore_exec.post();
            }
            // _semaphore_exec incremented to _semaphore_exec
            // _mutex_bind still locks implying all exec are waiting
            // RAII lock _mutex_bind unlocked
            // }
        }

        template <typename T, typename Aux>
        udho::view::data::bridges::results exec_pool(const std::string& name, const std::string& prefix, const T& data, const Aux& aux, std::string& output){
            udho::view::data::bridges::results results;
            {
                std::unique_lock<std::recursive_mutex> lock(_mutex_bind, std::defer_lock);
                if(!udho::view::data::bindings<StateT, T>::exists(_states.size())){
                    // { Enter CS
                    lock.lock();
                    // }

                    // { CS
                    bind_pool(udho::view::data::type<T>{});
                    // }

                    // { Exit CS
                    lock.unlock();
                    // }
                }
            }

            // { Enter CS
            _semaphore_exec.wait();
    #if (UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE)
            std::size_t index = 0;
            _free_lf_q.pop(index);
    #else
            _mutex_queue.lock();
            assert(_freeq.size() > 0);
            std::size_t index = _freeq.front();
            _freeq.pop();
            _mutex_queue.unlock();
    #endif
            std::unique_ptr<state_type>& state = _states[index];
            // }

            // { CS
            results.start(index);
            exec_cs(name, prefix, data, aux, output, state, results);
            // }

            // { Exit CS
    #if (UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE)
            _free_lf_q.push(index);
    #else
            _mutex_queue.lock();
            _freeq.push(index);
            _mutex_queue.unlock();
    #endif
            // }
            _semaphore_exec.post();
            return results;
        }

        template <typename ClassT>
        void bind_cs(udho::view::data::type<ClassT> handle){
            for (std::unique_ptr<state_type>& state: _states){
                udho::view::data::bridges::bind<self_type> binder{*state};
                binder(handle);
            }
        }
        template <typename T, typename Aux>
        void exec_cs(const std::string& name, const std::string& prefix, const T& data, const Aux& aux, std::string& output, std::unique_ptr<state_type>& state, udho::view::data::bridges::results& results){
            std::size_t size = 0;
            try {
                size = state->exec(view_key(name, prefix), std::ref(data), std::ref(aux), output);
            } catch(const sol::error& e){
                std::string execption_str = udho::url::format("Lua Exeption while executing view {}/{}: ", prefix, name);
                std::string ex_str;
                ex_str += execption_str;
                ex_str += e.what();
                size = ex_str.size();
                output = ex_str;
                std::cerr << execption_str << e.what() << std::endl;
            } catch(const std::exception& e){
                std::string execption_str = udho::url::format("C++ Exeption while executing view {}/{}: ", prefix, name);
                std::string ex_str;
                ex_str += execption_str;
                ex_str += e.what();
                size = ex_str.size();
                output = ex_str;
                std::cerr << execption_str << e.what() << std::endl;
            } catch(...){
                std::string execption_str = udho::url::format("Unknown Exeption while executing view {}/{}: ", prefix, name);
                std::string ex_str;
                ex_str += execption_str;
                size = ex_str.size();
                output = ex_str;
                std::cerr << execption_str << std::endl;
            }
            results.finish(size);
        }

    private:
        /**
         * @brief Generates a key for identifying a view script.
         * @param name The name of the view.
         * @param prefix The prefix used in the naming.
         * @return The generated view key.
         */
        std::string view_key(const std::string& name, const std::string& prefix) const {
            return bridges::common::view_key(name, prefix);
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

            std::string name = script.save(key);
            std::cout << "Generated script at " << name << std::endl;
            bool success = true;
            for (std::unique_ptr<state_type>& state: _states){
                compiler_type compiler{*state};
                success = success && compiler(std::move(script));
            }
            return success;
        }
    private:
        std::size_t                                 _pool_size;
        std::vector<std::unique_ptr<state_type>>    _states;
        boost::interprocess::interprocess_semaphore _semaphore_exec;
#if (UDHO_INTERNAL_BRIDGE_USE_LOCKFREE_QUEUE)
        boost::lockfree::queue<std::size_t>         _free_lf_q;
#else
        std::queue<std::size_t>                     _freeq;
        std::mutex                                  _mutex_queue;
#endif
        std::recursive_mutex                        _mutex_bind;
        bridges::policy                             _policy;

};

}
}
}
}
#endif // UDHO_VIEW_BRIDGES_COMMON_BRIDGE_H
