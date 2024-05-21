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

#ifndef UDHO_VIEW_RESOURCES_STORE_H
#define UDHO_VIEW_RESOURCES_STORE_H

#include <string>
#include <map>
#include <stdexcept>
#include <udho/view/resources/resource.h>
#include <udho/view/resources/bundle.h>

namespace udho{
namespace view{
namespace resources{

namespace detail{
template <typename BridgeT>
struct store{
    using bridge_type = BridgeT;
    using bundle_type = resources::bundle<bridge_type>;

    static constexpr const char* primary_bundle_name = "primary";

    inline store(bridge_type& bridge): _bridge(bridge) {
        bundle(primary_bundle_name);
    }
    store(const store&) = delete;
    inline bundle_type& bundle(const std::string& name) {
        auto it = _bundles.find(name);
        if(it == _bundles.end()){
            auto result = _bundles.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(_bridge, name));
            it = result.first;
        }
        return it->second;
    }
    inline const bundle_type& bundle(const std::string& name) const {
        auto it = _bundles.find(name);
        if(it != _bundles.end()){
            return it->second;
        }
        throw std::invalid_argument{udho::url::format("No such bundle named {}", name)};
    }
    inline bundle_type& primary() { return bundle(primary_bundle_name); }
    inline const bundle_type& primary() const { return bundle(primary_bundle_name); }
    private:
        bridge_type& _bridge;
        std::map<std::string, bundle_type> _bundles;
};
}

template <typename BridgeT>
struct store: detail::store<BridgeT>{
    using bridge_type = BridgeT;

    store(bridge_type& bridge): detail::store<BridgeT>(bridge), views(detail::store<BridgeT>::primary().views) {}
    friend store& operator<<(store& s, const resource& res){
        s.primary() << res;
        return s;
    }

    detail::bundle_view_proxy<bridge_type>& views;
};

}
}
}


#endif // UDHO_VIEW_RESOURCES_STORE_H
