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

#ifndef UDHO_VIEW_BRIDGES_CHAI_H
#define UDHO_VIEW_BRIDGES_CHAI_H

#include <string>
#include <nlohmann/json.hpp>
#include <udho/view/scope.h>
#include <chaiscript/chaiscript.hpp>

namespace udho{
namespace view{
namespace data{
namespace bridges{

struct chai{

    template <typename X>
    struct binder{

        binder(chaiscript::ChaiScript& chai, const std::string& name): _chai(chai) {
            _chai.add(chaiscript::constructor<X ()>(), name);
            _chai.add(chaiscript::constructor<X (const X&)>(), name);
            _chai.add(chaiscript::user_type<X>(), name);
        }

        template <typename KeyT, typename T>
        binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::writable>, KeyT, udho::view::data::wrapper<T>>& nvp){
            auto& w = nvp.wrapper();
            _chai.add(chaiscript::fun(*w), nvp.name());
            return *this;
        }
        template <typename KeyT, typename T>
        binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::readonly>, KeyT, udho::view::data::wrapper<T>>& nvp){
            auto& w = nvp.wrapper();
            _chai.add(chaiscript::fun(*w), nvp.name());
            return *this;
        }
        template <typename KeyT, typename U, typename V>
        binder& operator()(udho::view::data::nvp<udho::view::data::policies::property<udho::view::data::policies::functional>, KeyT, udho::view::data::wrapper<U, V>>& nvp){
            auto& w = nvp.wrapper();
            // _chai.add(chaiscript::fun(*w), nvp.name());
            return *this;
        }
        template <typename KeyT, typename T>
        binder& operator()(udho::view::data::nvp<udho::view::data::policies::function, KeyT, udho::view::data::wrapper<T>>& nvp){
            auto& w = nvp.wrapper();
            _chai.add(chaiscript::fun(*w), nvp.name());
            return *this;
        }
        private:
            chaiscript::ChaiScript& _chai;
    };

    inline chai() {

    }
    inline void init(){

    }

    template <typename ClassT, typename... Xs>
    void bind(udho::view::data::metatype<udho::view::data::associative<Xs...>>& type){
        bind<ClassT>(type.name(), type.members());
    }

    template <typename ClassT>
    void bind(udho::view::data::type<ClassT> handle){
        auto meta = prototype(handle);
        bind<ClassT>(meta);
    }

    private:
        template <typename ClassT, typename... Xs>
        void bind(const std::string& name, udho::view::data::associative<Xs...>& assoc){
            binder<ClassT> user_type(_state, name);
            assoc.apply(std::move(user_type));
            _state.eval(R"(
                print("begin chai");
                var obj = info();
                print(obj.name);
                print(obj.value);

                obj.name = "changed";
                obj.value = 100;
                obj.print();
                print("end chai");
            )");
        }

    private:
        chaiscript::ChaiScript _state;

};


}
}
}
}


#endif // UDHO_VIEW_BRIDGES_CHAI_H

