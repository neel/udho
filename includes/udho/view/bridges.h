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

#ifndef UDHO_VIEW_INTEROP_BRIDGES_H
#define UDHO_VIEW_INTEROP_BRIDGES_H

#include <string>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <udho/view/scope.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{


struct lua{
    template <typename X>
    struct binder{
        using user_type = sol::usertype<X>;

        binder(sol::table& table, const std::string& name): _type(table.new_usertype<X>(name)) {}

        template <typename KeyT, typename T>
        binder& operator()(udho::view::data::nvp<udho::view::data::policies::property, KeyT, udho::view::data::wrapper<T>>& nvp){
            auto& w = nvp.wrapper();
            if(w.assignable){
                _type.set(nvp.name(), *w);
            }else{
                _type.set(nvp.name(), sol::readonly(*w));
            }
            return *this;
        }
        template <typename KeyT, typename U, typename V>
        binder& operator()(udho::view::data::nvp<udho::view::data::policies::property, KeyT, udho::view::data::wrapper<U, V>>& nvp){
            auto& w = nvp.wrapper();
            _type.set_property(nvp.name(),
                *static_cast<udho::view::data::getter_value<U>&>(w),
                *static_cast<udho::view::data::setter_value<V>&>(w)
            );
            return *this;
        }
        private:
            user_type _type;
    };

    lua() {
        _state.open_libraries(sol::lib::base);
    }
    void init(){
        _udho = _state["udho"].get_or_create<sol::table>();
    }

    template <typename ClassT, typename... Xs>
    void bind(const std::string& name, udho::view::data::associative<Xs...>& assoc){
        binder<ClassT> user_type(_udho, name);
        assoc.apply(user_type);
    }

    private:
        sol::state _state;
        sol::table _udho;
};


}
}
}
}


#endif // UDHO_VIEW_INTEROP_BRIDGES_H
