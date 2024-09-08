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

#ifndef UDHO_VIEW_RESOURCES_SUBSTORE_STORE_H
#define UDHO_VIEW_RESOURCES_SUBSTORE_STORE_H

#include <udho/view/resources/substores/tmpl.h>
#include <udho/view/resources/substores/asset.h>

namespace udho{
namespace view{
namespace resources{

namespace tmpl{

template <typename BridgeT, typename TailT = void>
struct multi_substore_{
    using bridge_type = BridgeT;
    using tmpl_substore_type = udho::view::resources::tmpl::substore<BridgeT>;
    using tail_type = TailT;
    template <typename XBridgeT>
    using substore_type = std::conditional_t< std::is_same_v<BridgeT, XBridgeT>, tmpl_substore_type, typename tail_type::template substore_type<XBridgeT>>;

    template <typename... X>
    multi_substore_(bridge_type& bridge, X&&... x): _substore(bridge), tail_type(x...) {}

    template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    tmpl_substore_type& substore(){ return _substore; }
    template <typename XBridgeT, std::enable_if_t<!std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    typename tail_type::template substore_type<XBridgeT>& substore() { return _tail.template substore<XBridgeT>(); }

    void lock() {
        _substore.lock();
        _tail.lock();
    }

    private:
        tmpl_substore_type _substore;
        tail_type          _tail;
};

template <typename BridgeT>
struct multi_substore_<BridgeT, void>{
    using bridge_type = BridgeT;
    using tmpl_substore_type = udho::view::resources::tmpl::substore<BridgeT>;
    using tail_type = void;
    template <typename XBridgeT>
    using substore_type = std::conditional_t< std::is_same_v<BridgeT, XBridgeT>, tmpl_substore_type, void>;

    multi_substore_(bridge_type& bridge): _substore(bridge) {}

    template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    tmpl_substore_type& substore(){ return _substore; }

    void lock() {
        _substore.lock();
    }

    private:
        tmpl_substore_type _substore;
};

template <typename Bridge, typename... Bridges>
struct multi_substore: multi_substore_<Bridge, multi_substore<Bridges...>> {
    using base = multi_substore_<Bridge, multi_substore<Bridges...>>;
    using tail = typename base::tail_type;

    multi_substore(Bridge& bridge, Bridges&... bridges): base(bridge, tail(bridges...)) {}
    void lock() {
        base::lock();
    }
};

template <typename Bridge>
struct multi_substore<Bridge>: multi_substore_<Bridge> {
    using base = multi_substore_<Bridge>;
    using tail = typename base::tail_type;

    multi_substore(Bridge& bridge): base(bridge) {}
    void lock() {
        base::lock();
    }
};

}

template <typename StoreT>
struct mutable_subsets;

template <typename StoreT>
struct readonly_subsets;

template <typename... Bridges>
struct storage{
    using asset_substore_type = udho::view::resources::asset::substore;
    using tmpl_substore_type  = udho::view::resources::tmpl::multi_substore<Bridges...>;
    using writer_type         = mutable_subsets<storage<Bridges...>>;
    using reader_type         = readonly_subsets<storage<Bridges...>>;

    storage(Bridges&... bridges): _tmpls(bridges...) {}

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

template <typename... Bridges>
struct mutable_subsets<storage<Bridges...>>{
    using store_type = storage<Bridges...>;

    mutable_subsets() = delete;
    mutable_subsets(const std::string& prefix, store_type& store): _store(store), _prefix(prefix) {}
    mutable_subsets(const mutable_subsets&) = delete;

    template <typename Bridge>
    typename store_type::tmpl_substore_type::template substore_type<Bridge>::mutable_accessor_type views() { return _store.template tmpl<Bridge>().writer(_prefix); }

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
struct readonly_subsets<storage<Bridges...>>{
    using store_type = storage<Bridges...>;

    readonly_subsets() = delete;
    readonly_subsets(const std::string& prefix, store_type& store): _store(store), _prefix(prefix) {}
    readonly_subsets(const readonly_subsets&) = delete;

    template <typename Bridge>
    typename store_type::tmpl_substore_type::template substore_type<Bridge>::readonly_accessor_type views() { return _store.template tmpl<Bridge>().reader(_prefix); }

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

#endif // UDHO_VIEW_RESOURCES_SUBSTORE_STORE_H
