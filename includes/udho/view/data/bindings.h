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

#ifndef UDHO_VIEW_DATA_BINDINGS_H
#define UDHO_VIEW_DATA_BINDINGS_H


#include <udho/view/data/fwd.h>
#include <udho/view/bridges/fwd.h>
#include <udho/view/data/nvp.h>
#include <iostream>
#include <udho/view/data/type.h>
#include <set>
#include <cstdint>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif

namespace udho{
namespace view{
namespace data{

/**
 * @addtogroup DoxyG_view_tmpl_data
 * @{
 */

/**
 * @class bindings
 * @brief keeps track of binding of a C++ wiith with a foreign language backend.
 * @tparam StateT the state class representing the state of the foreign language runtime
 * @tparam T type C++ type
 */
template <typename StateT, typename T>
struct bindings{

    // template <template<class> class BinderT, typename Class>
    // friend struct binder;

    template <typename BridgeT>
    friend struct udho::view::data::bridges::bind;

    /**
     * @brief atomic load count and return
     */
    inline static std::uint32_t count() { return _states.size(); }
    /**
     * @brief atomic checks whether binding already exists or not (count > 0)
     */
    inline static bool exists_any() { return count() > 0; }
    /**
     * @brief atomic checks whether binding already performed at least expected_count number of times.
     */
    inline static bool exists(std::uint32_t expected_count) { return count() >= expected_count; }
    /**
     * @brief atomic checks whether binding already exists or not (count > 0)
     */
    inline static bool exists(const StateT& state) {
        return _states.count(state.id()) == 1;
    }
    inline static void unbind(const StateT& state) {
        auto it = _states.find(state.id());
        if(it != _states.end()){
            _states.erase(it);
        }
    }

    private:
        inline static void bound_one(const StateT& state){
            _states.insert(state.id());
            state.add_unbinder([](const StateT& state){
                bindings<StateT, T>::unbind(state);
            });
            // ++_count;
        }
        /**
         * @brief atomic increment count
         */
        // inline static void bound_one(){ ++_count; }
        // static std::atomic_uint _count;
        static std::set<typename StateT::id_type> _states;
};

// template <typename StateT, typename T>
// std::atomic_uint bindings<StateT, T>::_count = 0;

template <typename StateT, typename T>
std::set<typename StateT::id_type> bindings<StateT, T>::_states;

namespace detail{

template <template<class> class BinderT, typename ClassT>
struct binder{
    using foreign_binder_type = BinderT<ClassT>;

    template <typename StateT>
    static foreign_binder_type apply(StateT& state, udho::view::data::type<ClassT> type){
        // if(!udho::view::data::bindings<StateT, ClassT>::exists()){

        auto meta = metatype(type);
        std::cout << "udho::view::data::detail::binder: binding metatype " << meta.name() << std::endl;
        foreign_binder_type binder(state, meta.name());
        meta.members().apply_all(binder);

        return binder;

        // bindings<StateT, ClassT>::_exists = true;
        // }
    }
};

}

/** @} */

}
}
}




#endif // UDHO_VIEW_DATA_BINDINGS_H
