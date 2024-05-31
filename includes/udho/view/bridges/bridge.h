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
#include <udho/view/parser.h>
#include <udho/view/bridges/lua/script.h>
#include <udho/view/bridges/lua/buffer.h>
#include <udho/view/bridges/lua/binder.h>
#include <udho/view/resources/resource.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

template <typename StateT, typename CompilerT, typename ScriptT, template <class> typename BinderT>
struct bridge{
    using state_type    = StateT;
    using compiler_type = CompilerT;
    using script_type   = ScriptT;
    template <typename X>
    using binder_type   = BinderT<X>;

    void init(){
        _state.init();
    }

    template <typename ClassT>
    void bind(udho::view::data::type<ClassT> handle){
        udho::view::data::binder<BinderT, ClassT>::apply(_state, handle);
    }

    template <typename IteratorT>
    bool compile(resources::resource_buffer<udho::view::resources::type::view, IteratorT>&& view, const std::string& prefix){
        std::string key = view_key(view.name(), prefix);
        return compile(view.begin(), view.end(), key);
    }

    bool compile(resources::resource_file<udho::view::resources::type::view>&& view, const std::string& prefix){
        std::string key = view_key(view.name(), prefix);
        return compile(view.begin(), view.end(), key);
    }

    template <typename T>
    std::size_t exec(const std::string& name, const std::string& prefix, const T& data, std::string& output){
        if(!udho::view::data::bindings<StateT, T>::exists()){
            bind(udho::view::data::type<T>{});
        }
        return _state.exec(view_key(name, prefix), data, output);
    }

    private:
        std::string view_key(const std::string& name, const std::string& prefix) const {
            return udho::url::format(":{}/{}", prefix, name);
        }
        template <typename IteratorT>
        bool compile(IteratorT begin, IteratorT end, std::string key){
            script_type script{key};
            udho::view::sections::parser parser;
            parser.parse(begin, end, script);
            script.finish();
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
