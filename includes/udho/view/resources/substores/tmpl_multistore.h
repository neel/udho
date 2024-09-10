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

#ifndef UDHO_VIEW_RESOURCES_SUBSTORE_TMPL_MULTISTORE_H
#define UDHO_VIEW_RESOURCES_SUBSTORE_TMPL_MULTISTORE_H


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

    template <typename... BridgesT>
    multi_substore_(bridge_type& bridge, BridgesT&&... bridges): _substore(bridge), tail_type(bridges...) {}

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

    void lock() { _substore.lock(); }

    private:
        tmpl_substore_type _substore;
};

template <typename Bridge, typename... Bridges>
struct multi_substore: multi_substore_<Bridge, multi_substore<Bridges...>> {
    using base = multi_substore_<Bridge, multi_substore<Bridges...>>;
    using tail = typename base::tail_type;

    multi_substore(Bridge& bridge, Bridges&... bridges): base(bridge, tail(bridges...)) {}
    void lock() { base::lock(); }
};

template <typename Bridge>
struct multi_substore<Bridge>: multi_substore_<Bridge> {
    using base = multi_substore_<Bridge>;
    using tail = typename base::tail_type;

    multi_substore(Bridge& bridge): base(bridge) {}
    void lock() { base::lock(); }
};

template <typename BridgeT, typename TailT = void>
struct multi_substore_proxy_{
    using tmpl_substore_type = udho::view::resources::tmpl::substore<BridgeT>;
    using tail_type          = TailT;

    /**
     * @brief takes a store and takes a reference to the substore inside
     * - order doesn't matter
     * - allows same bridge to appear multiple times which is suboptimal
     * TODO need to check that the multi_substore contains substore of bridge BridgeT using enable_if
     */
    template <typename... XBridges>
    multi_substore_proxy_(multi_substore<XBridges...>& store): _substore(store.template substore<BridgeT>()), _tail(store) { }

    template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    tmpl_substore_type& substore(){ return _substore; }
    template <typename XBridgeT, std::enable_if_t<!std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    typename tail_type::template substore_type<XBridgeT>& substore() { return _tail.template substore<XBridgeT>(); }

    void lock() {
        _substore.lock();
        _tail.lock();
    }

    private:
        tmpl_substore_type& _substore;
        tail_type           _tail;
};

template <typename BridgeT>
struct multi_substore_proxy_<BridgeT, void>{
    using tmpl_substore_type = udho::view::resources::tmpl::substore<BridgeT>;

    /**
     * @brief takes a store and takes a reference to the substore inside
     * - order doesn't matter
     * - allows same bridge to appear multiple times which is suboptimal
     * TODO need to check that the multi_substore contains substore of bridge BridgeT using enable_if
     */
    template <typename... XBridges>
    multi_substore_proxy_(multi_substore<XBridges...>& store): _substore(store.template substore<BridgeT>()) { }

    template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    tmpl_substore_type& substore(){ return _substore; }

    void lock() {
        _substore.lock();
    }

    private:
        tmpl_substore_type& _substore;
};


template <typename BridgeT, typename... Bridges>
struct multi_substore_proxy: multi_substore_proxy_<BridgeT, multi_substore_proxy<Bridges...>>{
    using base = multi_substore_proxy_<BridgeT, multi_substore_proxy<Bridges...>>;

    template <typename... XBridges>
    multi_substore_proxy(multi_substore<XBridges...>& store): base(store) {}
};

template <typename BridgeT>
struct multi_substore_proxy<BridgeT>: multi_substore_proxy_<BridgeT>{
    using base = multi_substore_proxy_<BridgeT>;

    template <typename... XBridges>
    multi_substore_proxy(multi_substore<XBridges...>& store): base(store) {}
};

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_SUBSTORE_TMPL_MULTISTORE_H
