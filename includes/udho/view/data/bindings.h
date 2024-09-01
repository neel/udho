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
#include <udho/view/data/nvp.h>
#include <iostream>

#ifdef WITH_JSON_NLOHMANN
#include <nlohmann/json.hpp>
#endif

namespace udho{
namespace view{
namespace data{

template <template<class> class BinderT, typename Class>
struct binder;

template <typename StateT, typename T>
struct bindings{

    template <template<class> class BinderT, typename Class>
    friend struct binder;

    static bool exists() { return _exists; }
    private:
        static bool _exists;
};

template <typename DerivedT, typename T>
bool bindings<DerivedT, T>::_exists = false;

template <template<class> class BinderT, typename ClassT>
struct binder{
    template <typename StateT>
    static void apply(StateT& state, udho::view::data::type<ClassT> type){
        if(!udho::view::data::bindings<StateT, ClassT>::exists()){
            auto meta = prototype(type);
            std::cout << "udho::view::data::binder: binding " << meta.name() << std::endl;
            BinderT<ClassT> binder(state, meta.name());
            meta.members().apply_all(std::move(binder));

            bindings<StateT, ClassT>::_exists = true;
        }
    }
};



}
}
}


#endif // UDHO_VIEW_DATA_BINDINGS_H
