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

#include <udho/view/resources/tmpl/substore.h>
#include <udho/view/resources/tmpl/store.h>
#include <udho/view/resources/asset/store.h>
#include <udho/view/resources/fwd.h>

namespace udho{
namespace view{
namespace resources{

template <typename... Bridges>
struct store{
    template <typename... XBridges>
    friend struct const_store;

    using asset_store_type      = udho::view::resources::asset::store;
    using tmpl_multi_substore_type = udho::view::resources::tmpl::store<Bridges...>;

    store(Bridges&... bridges): _tmpls(bridges...) {}

    template <typename Bridge>
    typename tmpl_multi_substore_type::template substore_type<Bridge>& tmpl() { return _tmpls.template substore<Bridge>(); }

    asset_store_type& assets() { return _assets; }

    void lock() {
        _tmpls.lock();
        _assets.lock();
    }

    private:
        asset_store_type      _assets;
        tmpl_multi_substore_type _tmpls;
};

template <typename... Bridges>
struct const_store_prefixed;

template <typename... XBridges>
struct const_store{
    using asset_substore_readonly_js   = udho::view::resources::asset::const_substore<asset::type::js>;
    using asset_substore_readonly_css  = udho::view::resources::asset::const_substore<asset::type::css>;
    using asset_substore_readonly_img  = udho::view::resources::asset::const_substore<asset::type::img>;

    // template <typename... Bridges>
    // friend struct const_store_prefixed;

    using asset_substore_readonly_type   = udho::view::resources::asset::const_store;
    using tmpl_const_multi_substore_type = udho::view::resources::tmpl::const_store<XBridges...>;

    template <typename... Bridges>
    const_store(const store<Bridges...>& store): _tmpls_proxy(store._tmpls), _assets(store._assets), _assets_js(_assets), _assets_css(_assets), _assets_img(_assets) { }

    template <typename XBridgeT>
    udho::view::resources::tmpl::const_substore<XBridgeT> tmpl() { return _tmpls_proxy.template substore<XBridgeT>(); }

    template <typename XBridgeT>
    udho::view::resources::tmpl::proxy<XBridgeT> view(const std::string& prefix, const std::string& name){ return tmpl<XBridgeT>()(prefix, name); }

    template <asset::type Type>
    const asset_substore_readonly_type& assets() { return _assets; }

    const asset_substore_readonly_js&  js()  const { return _assets_js;  }
    const asset_substore_readonly_css& css() const { return _assets_css; }
    const asset_substore_readonly_img& img() const { return _assets_img; }

    // const_store_prefixed<XBridges...> operator[] (const std::string& prefix) const { return const_store_prefixed<XBridges...>{*this, prefix}; }

    private:
        tmpl_const_multi_substore_type _tmpls_proxy;
        asset_substore_readonly_type   _assets;
        asset_substore_readonly_js     _assets_js;
        asset_substore_readonly_css    _assets_css;
        asset_substore_readonly_img    _assets_img;

};

// template <typename... XBridges>
// struct const_store_prefixed{
//     using asset_substore_readonly_js   = udho::view::resources::asset::const_substore_prefixed<asset::type::js>;
//     using asset_substore_readonly_css  = udho::view::resources::asset::const_substore_prefixed<asset::type::css>;
//     using asset_substore_readonly_img  = udho::view::resources::asset::const_substore_prefixed<asset::type::img>;
//     using store_readonly_prefixed_type = udho::view::resources::tmpl::const_multi_substore_prefixed<XBridges...>;
//
//     template <typename... Bridges>
//     const_store_prefixed(const const_store<Bridges...>& store, const std::string& prefix): _prefix(prefix), _tmpls_prefixed(store._tmpls_proxy, prefix), _assets_prefixed_js(store._assets, prefix), _assets_prefixed_css(store._assets, prefix), _assets_prefixed_img(store._assets, prefix) { }
//
//     template <typename XBridgeT>
//     auto tmpl() const { return _tmpls_prefixed.template substore<XBridgeT>(); }
//
//     const asset_substore_readonly_js&  js()  const { return _assets_prefixed_js;  }
//     const asset_substore_readonly_css& css() const { return _assets_prefixed_css; }
//     const asset_substore_readonly_img& img() const { return _assets_prefixed_img; }
//
//     private:
//         std::string                  _prefix;
//         store_readonly_prefixed_type _tmpls_prefixed;
//         asset_substore_readonly_js   _assets_prefixed_js;
//         asset_substore_readonly_css  _assets_prefixed_css;
//         asset_substore_readonly_img  _assets_prefixed_img;
//
// };


}
}
}

#endif // UDHO_VIEW_RESOURCES_STORE_H
