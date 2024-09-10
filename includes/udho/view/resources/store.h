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

#include <udho/view/resources/substores/tmpl.h>
#include <udho/view/resources/substores/tmpl_multistore.h>
#include <udho/view/resources/substores/asset.h>
#include <udho/view/resources/fwd.h>

namespace udho{
namespace view{
namespace resources{

template <typename... Bridges>
struct store{
    template <typename... XBridges>
    friend struct storage_proxy;

    using asset_substore_type = udho::view::resources::asset::substore;
    using tmpl_substore_type  = udho::view::resources::tmpl::multi_substore<Bridges...>;
    using writer_type         = mutable_subsets<store<Bridges...>>;
    using reader_type         = readonly_subsets<store<Bridges...>>;

    store(Bridges&... bridges): _tmpls(bridges...) {}

    template <typename Bridge>
    typename tmpl_substore_type::template substore_type<Bridge>& tmpl() { return _tmpls.template substore<Bridge>(); }

    asset_substore_type& assets() { return _assets; }

    writer_type writer(const std::string& prefix) { return writer_type{prefix, *this}; }
    reader_type reader(const std::string& prefix) { return reader_type{prefix, *this}; }

    void lock() {
        _tmpls.lock();
        _assets.lock();
    }

    private:
        asset_substore_type  _assets;
        tmpl_substore_type   _tmpls;
};

template <typename... XBridges>
struct storage_proxy{
    using asset_substore_type = udho::view::resources::asset::substore;
    using proxy_type          = udho::view::resources::tmpl::multi_substore_proxy<XBridges...>;
    using writer_type         = mutable_subsets<storage_proxy<XBridges...>>;
    using reader_type         = readonly_subsets<storage_proxy<XBridges...>>;

    template <typename... Bridges>
    storage_proxy(store<Bridges...>& store): _tmpls_proxy(store._tmpls), _assets(store.assets()) { }

    template <typename Bridge>
    udho::view::resources::tmpl::substore<Bridge>& tmpl() { return _tmpls_proxy.template substore<Bridge>(); }

    asset_substore_type& assets() { return _assets; }

    writer_type writer(const std::string& prefix) { return writer_type{prefix, *this}; }
    reader_type reader(const std::string& prefix) { return reader_type{prefix, *this}; }

    void lock() {
        _tmpls_proxy.lock();
        _assets.lock();
    }

    private:
        proxy_type           _tmpls_proxy;
        asset_substore_type& _assets;

};

template <typename... Bridges>
struct mutable_subsets<storage_proxy<Bridges...>>{
    using store_type = storage_proxy<Bridges...>;

    mutable_subsets() = delete;
    mutable_subsets(const std::string& prefix, store_type& store): _store(store), _prefix(prefix) {}
    mutable_subsets(const mutable_subsets&) = delete;

    template <typename Bridge>
    typename udho::view::resources::tmpl::substore<Bridge>::mutable_accessor_type views() { return _store.template tmpl<Bridge>().writer(_prefix); }

    template <asset::type Type>
    typename store_type::asset_substore_type::template mutable_accessor_type<Type> assets() { return _store.assets().template writer<Type>(_prefix); }

    typename store_type::asset_substore_type::template mutable_accessor_type<asset::type::js>  js()  { return assets<asset::type::js>(_prefix);  }
    typename store_type::asset_substore_type::template mutable_accessor_type<asset::type::css> css() { return assets<asset::type::css>(_prefix); }
    typename store_type::asset_substore_type::template mutable_accessor_type<asset::type::img> img() { return assets<asset::type::img>(_prefix); }

    private:
        store_type& _store;
        std::string _prefix;
};

template <typename... Bridges>
struct readonly_subsets<storage_proxy<Bridges...>>{
    using store_type = storage_proxy<Bridges...>;

    readonly_subsets() = delete;
    readonly_subsets(const std::string& prefix, store_type& store): _store(store), _prefix(prefix) {}
    readonly_subsets(const readonly_subsets&) = default;

    template <typename Bridge>
    typename udho::view::resources::tmpl::substore<Bridge>::readonly_accessor_type views() { return _store.template tmpl<Bridge>().reader(_prefix); }

    template <asset::type Type>
    typename store_type::asset_substore_type::template readonly_accessor_type<Type> assets() { return _store.assets().template reader<Type>(_prefix); }

    typename store_type::asset_substore_type::template readonly_accessor_type<asset::type::js>  js()  { return assets<asset::type::js>(_prefix);  }
    typename store_type::asset_substore_type::template readonly_accessor_type<asset::type::css> css() { return assets<asset::type::css>(_prefix); }
    typename store_type::asset_substore_type::template readonly_accessor_type<asset::type::img> img() { return assets<asset::type::img>(_prefix); }

    private:
        store_type& _store;
        std::string _prefix;
};

}
}
}

#endif // UDHO_VIEW_RESOURCES_STORE_H
