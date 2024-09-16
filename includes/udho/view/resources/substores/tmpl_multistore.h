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

// multi_substore

#ifndef __DOXYGEN__

template <typename BridgeT, typename TailT = void>
struct multi_substore_{
    using bridge_type = BridgeT;
    using tmpl_substore_type = udho::view::resources::tmpl::substore<BridgeT>;
    using readonly_substore_proxy_type = udho::view::resources::tmpl::substore_readonly_proxy<BridgeT>;
    using tail_type = TailT;
    template <typename XBridgeT>
    using substore_type = std::conditional_t< std::is_same_v<BridgeT, XBridgeT>, tmpl_substore_type, typename tail_type::template substore_type<XBridgeT>>;
    template <typename XBridgeT>
    using readonly_substore_type = std::conditional_t< std::is_same_v<BridgeT, XBridgeT>, readonly_substore_proxy_type, typename tail_type::template readonly_substore_type<XBridgeT>>;

    template <typename... BridgesT>
    multi_substore_(bridge_type& bridge, BridgesT&&... bridges): _substore(bridge), tail_type(bridges...) {}

    template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    tmpl_substore_type& substore(){ return _substore; }
    template <typename XBridgeT, std::enable_if_t<!std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    typename tail_type::template substore_type<XBridgeT>& substore() { return _tail.template substore<XBridgeT>(); }

    template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    readonly_substore_proxy_type readonly_substore() const { return readonly_substore_proxy_type{_substore}; }
    template <typename XBridgeT, std::enable_if_t<!std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    typename tail_type::template readonly_substore_type<XBridgeT> readonly_substore() const { return _tail.template readonly_substore<XBridgeT>(); }

    template <typename XBridgeT>
    auto substore() const { return readonly_substore<XBridgeT>(); }

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
    using readonly_substore_proxy_type = udho::view::resources::tmpl::substore_readonly_proxy<BridgeT>;
    using tail_type = void;

    template <typename XBridgeT>
    using substore_type = std::conditional_t< std::is_same_v<BridgeT, XBridgeT>, tmpl_substore_type, void>;
    template <typename XBridgeT>
    using readonly_substore_type = std::conditional_t< std::is_same_v<BridgeT, XBridgeT>, readonly_substore_proxy_type, void >;

    multi_substore_(bridge_type& bridge): _substore(bridge) {}

    template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    tmpl_substore_type& substore(){ return _substore; }

    template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
    readonly_substore_proxy_type readonly_substore() const { return readonly_substore_proxy_type{_substore}; }

    template <typename XBridgeT>
    readonly_substore_proxy_type substore() const { return readonly_substore<XBridgeT>(); }

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

#else

/**
 * @brief Multiple substores each with a different bridge.
 * Provides a unified interface to manage a collection of substores, each tailored to a specific bridge type.
 * @tparam Bridges Variadic template parameters representing view bridges
 */
template <typename... Bridges>
struct multi_substore{
    template <typename XBridgeT>
    using substore_type = auto;

    template <typename XBridgeT>
    using readonly_substore_type = auto;

    /**
     * @brief Accesses the substore associated with a specific bridge type
     * @tparam XBridgeT The bridge type for which the substore is requested.
     * @return Reference to the requested substore type.
     */
    template <typename XBridgeT>
    auto& substore();

    /**
     * @brief Readonly access the substore associated with a specific bridge type
     * @tparam XBridgeT The bridge type for which the substore is requested.
     * @return Readonly access to the requested substore type.
     */
    template <typename XBridgeT>
    auto substore() const;

    void lock();
};

#endif // __DOXYGEN__


#ifndef __DOXYGEN__ // multi_substore_readonly

template <typename BridgeT = void, typename... Bridges>
class multi_substore_readonly;

template <>
class multi_substore_readonly<void>{
    template <typename BridgeT, typename... Bridges>
    friend class multi_substore_readonly;

    template <typename XBridgeT>
    using substore_readonly_proxy_type = void;

    public:
        template <typename... XBridges>
        explicit multi_substore_readonly(const multi_substore<XBridges...>&) {}
};

template <typename BridgeT, typename... Bridges>
class multi_substore_readonly{

    template <typename XBridgeT, typename... XBridges>
    friend class multi_substore_readonly;

    using head_type = substore_readonly_proxy<BridgeT>;
    using tail_type = multi_substore_readonly<Bridges...>;
    template <typename XBridgeT>
    using substore_readonly_proxy_type = std::conditional_t< std::is_same_v<XBridgeT, BridgeT>, substore_readonly_proxy<XBridgeT>, typename tail_type::template substore_readonly_proxy_type<XBridgeT>>;

    head_type _head;
    tail_type _tail;

    public:

        template <typename... XBridges>
        explicit multi_substore_readonly(const multi_substore<XBridges...>& store): _head(store.template substore<BridgeT>()), _tail(store) {}

        template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
        const substore_readonly_proxy_type<XBridgeT>& substore() const{ return _head; }

        template <typename XBridgeT, std::enable_if_t<!std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
        const substore_readonly_proxy_type<XBridgeT>& substore() const { return _tail.template substore<XBridgeT>(); }

        template <typename XBridgeT, std::enable_if_t<std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
        substore_readonly_proxy_type<XBridgeT>& substore() { return _head; }

        template <typename XBridgeT, std::enable_if_t<!std::is_same_v<XBridgeT, BridgeT>>* = nullptr>
        substore_readonly_proxy_type<XBridgeT>& substore() { return _tail.template substore<XBridgeT>(); }
};

#else

/**
 * @brief readonly interface to multi substore
 * @tparam Bridges... set of bridges of which a readonly interface is requested (defaults to void)
 */
template <typename... Bridges>
struct multi_substore_readonly{
    /**
     * @brief construct a readonly interface to an existing multi_substore
     * The multi_substore must contain the bridges provided as the template parameters of multi_substore_readonly
     * @param const reference to the multi_substore
     * @tparam XBridges... set of bridges of the multi_substore (XBridges... must be a superset of Bridges...)
     */
    template <typename... XBridges>
    explicit multi_substore_readonly(const multi_substore<XBridges...>& store);

    /**
     * @brief Readonly access the substore associated with a specific bridge type
     * @tparam XBridgeT The bridge type for which the substore is requested.
     * @return Readonly access to the requested substore type.
     */
    template <typename XBridgeT>
    const substore_readonly_proxy<XBridgeT>& substore() const;

    /**
     * @brief Readonly access the substore associated with a specific bridge type
     * @tparam XBridgeT The bridge type for which the substore is requested.
     * @return Readonly access to the requested substore type.
     */
    template <typename XBridgeT>
    substore_readonly_proxy<XBridgeT>& substore();
};

#endif // multi_substore_readonly

template <typename... Bridges>
struct multi_substore_readonly_prefixed{
    template <typename... XBridges>
    multi_substore_readonly_prefixed(const multi_substore_readonly<XBridges...>& multi_substore, const std::string& prefix): _prefix(prefix), _multi_substore(multi_substore) {}

    template <typename XBridgeT>
    auto substore() const{ return substore_readonly_proxy_prefixed{_multi_substore.template substore<XBridgeT>(), _prefix}; }

    private:
        std::string _prefix;
        const multi_substore_readonly<Bridges...>& _multi_substore;
};

}

}
}
}

#endif // UDHO_VIEW_RESOURCES_SUBSTORE_TMPL_MULTISTORE_H
