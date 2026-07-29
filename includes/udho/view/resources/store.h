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
#include <udho/view/resources/asset/const_store.h>
#include <udho/view/resources/asset/const_substore.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/data/data.h>
#include <scn/scn.h>

#include <udho/view/bridges/script.h>

namespace udho{
namespace view{
namespace resources{

/**
 * @addtogroup DoxyG_view_resources
 * @{
 */

template <typename... Bridges>
struct store;

template <typename... Bridges>
struct prefixed_store;

template <typename... Bridges>
struct const_store_prefixed;

template <typename... XBridges>
struct const_store;

template <>
struct store<>;

template <>
struct prefixed_store<>;


/**
 * @brief Prefix-bound insertion interface for resource store.
 *
 * Resources inserted through this proxy are registered using the prefix
 * supplied during construction.
 */
template <>
struct prefixed_store<>{
    using store_type = store<>;

    /**
     * @brief Constructs a prefix-bound store proxy.
     * @param store Resource store receiving inserted assets.
     * @param prefix Prefix assigned to each inserted asset.
     */
    explicit prefixed_store(store_type& store, const std::string& prefix): _store(store), _prefix(prefix) {}

    /// @brief Copying a prefix-bound store proxy is disabled.
    prefixed_store(const prefixed_store&) = delete;

    /// @brief Move-constructs a prefix-bound store proxy.
    prefixed_store(prefixed_store&& other): _store(other._store), _prefix(std::move(other._prefix)) {}

    /**
     * @brief Adds an asset using this proxy's prefix.
     * @tparam AssetType Asset category.
     * @param res Asset transferred to the underlying store.
     */
    template <asset::type AssetType>
    void add(std::unique_ptr<asset::basic_resource<AssetType>>&& res);

    /**
     * @brief Adds an asset through an lvalue prefix proxy.
     * @tparam AssetType Asset category.
     * @param pstore Prefix-bound destination store.
     * @param res Asset transferred to the underlying store.
     * @return The supplied prefix proxy.
     */
    template <asset::type AssetType>
    friend prefixed_store<>& operator<<(prefixed_store<>& pstore, std::unique_ptr<asset::basic_resource<AssetType>>&& res){
        pstore.add(std::move(res));
        return pstore;
    }

    // template <asset::type AssetType>
    // friend prefixed_store<>& operator<<(prefixed_store<>& pstore, asset::basic_resource<AssetType>& res){
    //     pstore.add(res);
    //     return pstore;
    // }

    /**
     * @brief Adds an asset through a temporary prefix proxy.
     * @tparam AssetType Asset category.
     * @param pstore Prefix-bound destination store.
     * @param res Asset transferred to the underlying store.
     * @return The supplied temporary prefix proxy.
     */
    template <asset::type AssetType>
    friend prefixed_store<>&& operator<<(prefixed_store<>&& pstore, std::unique_ptr<asset::basic_resource<AssetType>>&& res){
        pstore.add(std::move(res));
        return std::forward<prefixed_store<>>(pstore);
    }

    // template <asset::type AssetType>
    // friend prefixed_store<>&& operator<<(prefixed_store<>&& pstore, asset::basic_resource<AssetType>& res){
    //     pstore.add(res);
    //     return std::forward<prefixed_store<>>(pstore);
    // }

    private:
        store_type& _store;
        std::string _prefix;
};

template <>
struct store<>{
    template <typename... XBridges>
    friend struct const_store;

    using const_store_type          = const_store<>;
    using asset_store_type          = udho::view::resources::asset::store;

    /**
     * @brief construct the resource store with the foreign language bridges required for evaluation for the view templates
     * @param bridges... references to the bridges
     */
    store() {}

    prefixed_store<> operator[](const std::string& prefix){
        return prefixed_store<>{*this, prefix};
    }

    /**
     * @brief gets reference to the asset store
     * @return reference to the asset store
     */
    asset_store_type& assets() { return _assets; }

    /**
     * @brief lock the storage
     * @warning The store should be locked before it is used for reading operations such as accessing/rendering views and assets.
     *          Once locked no other views or assets can be added to the store.
     */
    void lock() {
        _assets.lock();
    }

    private:
        asset_store_type         _assets;
};

template <asset::type AssetType>
void prefixed_store<>::add(std::unique_ptr<asset::basic_resource<AssetType>>&& res){
    _store.assets().add(_prefix, std::move(res));
}

/**
 * @brief The resource store combines storage for view templates written in foreign languages (such as lua) as well as assets (e.g. js, css, images etc..)
 * @tparam Bridges... the foreign language bridges for view executaion of the views
 *
 * @code
 * udho::view::data::bridges::lua lua;
 * lua.init();
 *
 * udho::view::resources::store<udho::view::data::bridges::lua> store{lua};
 *
 * boost::filesystem::path view_path = "/path/to/lua/view_x.lua";
 * std::string             js_str    = "console.log('Hello World')";
 *
 * store.tmpl<udho::view::data::bridges::lua>().add("primary", udho::view::resources::tmpl::resource("view_x", view_path));
 * store.assets().add("primary", udho::view::resources::asset::js("hello.js", js_str.begin(), js_str.end()));
 *
 * store.lock();
 *
 * udho::view::resources::const_store<udho::view::data::bridges::lua> cstore{store};
 *
 * // Now access the views and assets as follows
 *
 * udho::view::resources::tmpl::const_substore<udho::view::data::bridges::lua> lviews = cstore.tmpl<udho::view::data::bridges::lua>();
 * udho::view::resources::tmpl::proxy<udho::view::data::bridges::lua> view_x = lviews.view("primary", "view_x");
 *
 * std::cout << view_x(data, context) << std::endl; // Call the view_x as a function with the data and the context
 * @endcode
 */
template <typename... Bridges>
struct store{
    template <typename... XBridges>
    friend struct const_store;

    using const_store_type          = const_store<Bridges...>;
    using asset_store_type          = udho::view::resources::asset::store;
    using tmpl_multi_substore_type  = udho::view::resources::tmpl::store<Bridges...>;

    /**
     * @brief construct the resource store with the foreign language bridges required for evaluation for the view templates
     * @param bridges... references to the bridges
     */
    store(Bridges&... bridges): _tmpls(bridges...) {}

    /**
     * @brief access a substore dedicated for one particular bridge
     * @tparam Bridge the requested Bridge
     * @return udho::view::resources::tmpl::substore<Bridge> substore for the specific Bridge
     */
    template <typename Bridge>
    typename tmpl_multi_substore_type::template substore_type<Bridge>& tmpl() { return _tmpls.template substore<Bridge>(); }

    /**
     * @brief Adds a view template resource to the bundle and prepares it for use by compiling it through the bridge.
     * @param prefix The prefix used in resource identification.
     * @param res The resource to add and compile.
     */
    template <typename Bridge>
    void add(const std::string& prefix, udho::view::resources::tmpl::resource&& res){
        tmpl<Bridge>().add(prefix, std::forward<udho::view::resources::tmpl::resource>(res));
    }

    prefixed_store<Bridges...> operator[](const std::string& prefix){
        return prefixed_store<Bridges...>{*this, prefix};
    }

    /**
     * @brief gets reference to the asset store
     * @return reference to the asset store
     */
    asset_store_type& assets() { return _assets; }

    /**
     * @brief lock the storage
     * @warning The store should be locked before it is used for reading operations such as accessing/rendering views and assets.
     *          Once locked no other views or assets can be added to the store.
     */
    void lock() {
        _tmpls.lock();
        _assets.lock();
    }

    private:
        asset_store_type         _assets;
        tmpl_multi_substore_type _tmpls;
};

/**
 * @brief Prefix-bound insertion interface for a resource store.
 *
 * View template resources and assets inserted through this proxy are
 * registered using the prefix supplied during construction.
 *
 * @tparam Bridges Foreign-language bridges supported by the resource store.
 */
template <typename... Bridges>
struct prefixed_store{
    using store_type = store<Bridges...>;

    /**
     * @brief Constructs a prefix-bound store proxy.
     * @param store Resource store receiving inserted resources.
     * @param prefix Prefix assigned to each inserted resource.
     */
    explicit prefixed_store(store_type& store, const std::string& prefix): _store(store), _prefix(prefix) {}

    /// @brief Copying a prefix-bound store proxy is disabled.
    prefixed_store(const prefixed_store&) = delete;

    /// @brief Move-constructs a prefix-bound store proxy.
    prefixed_store(prefixed_store&& other): _store(other._store), _prefix(std::move(other._prefix)) {}

    /**
     * @brief Adds a bridged view template resource using this proxy's prefix.
     * @tparam Bridge Bridge used to compile or execute the view template.
     * @param res Bridged view template resource.
     */
    template <typename Bridge>
    void add(udho::view::resources::tmpl::bridged<Bridge>&& res){
        _store.template add<Bridge>(_prefix, std::move(res.resource()));
    }

    /**
     * @brief Adds an asset using this proxy's prefix.
     * @tparam AssetType Asset category.
     * @param res Asset transferred to the underlying store.
     */
    template <asset::type AssetType>
    void add(std::unique_ptr<asset::basic_resource<AssetType>>&& res){
        _store.assets().add(_prefix, std::move(res));
    }

    /**
     * @brief Adds a bridged view template through an lvalue prefix proxy.
     * @tparam Bridge Bridge associated with the view template.
     * @param pstore Prefix-bound destination store.
     * @param res Bridged view template resource.
     * @return The supplied prefix proxy.
     */
    template <typename Bridge>
    friend prefixed_store<Bridges...>& operator<<(prefixed_store<Bridges...>& pstore, udho::view::resources::tmpl::bridged<Bridge>&& res){
        pstore.add(std::forward<udho::view::resources::tmpl::bridged<Bridge>>(res));
        return pstore;
    }

    /**
     * @brief Adds a bridged view template through a temporary prefix proxy.
     * @tparam Bridge Bridge associated with the view template.
     * @param pstore Prefix-bound destination store.
     * @param res Bridged view template resource.
     * @return The supplied temporary prefix proxy.
     */
    template <typename Bridge>
    friend prefixed_store<Bridges...>&& operator<<(prefixed_store<Bridges...>&& pstore, udho::view::resources::tmpl::bridged<Bridge>&& res){
        pstore.add(std::forward<udho::view::resources::tmpl::bridged<Bridge>>(res));
        return std::forward<prefixed_store<Bridges...>>(pstore);
    }

    /**
     * @brief Adds an asset through an lvalue prefix proxy.
     * @tparam AssetType Asset category.
     * @param pstore Prefix-bound destination store.
     * @param res Asset transferred to the underlying store.
     * @return The supplied prefix proxy.
     */
    template <asset::type AssetType>
    friend prefixed_store<Bridges...>& operator<<(prefixed_store<Bridges...>& pstore, std::unique_ptr<asset::basic_resource<AssetType>>&& res){
        pstore.add(std::move(res));
        return pstore;
    }

    // template <asset::type AssetType>
    // friend prefixed_store<Bridges...>& operator<<(prefixed_store<Bridges...>& pstore, asset::basic_resource<AssetType>& res){
    //     pstore.add(res);
    //     return pstore;
    // }

    /**
     * @brief Adds an asset through a temporary prefix proxy.
     * @tparam AssetType Asset category.
     * @param pstore Prefix-bound destination store.
     * @param res Asset transferred to the underlying store.
     * @return The supplied temporary prefix proxy.
     */
    template <asset::type AssetType>
    friend prefixed_store<Bridges...>&& operator<<(prefixed_store<Bridges...>&& pstore, std::unique_ptr<asset::basic_resource<AssetType>>&& res){
        pstore.add(std::move(res));
        return std::forward<prefixed_store<Bridges...>>(pstore);
    }

    // template <asset::type AssetType>
    // friend prefixed_store<Bridges...>&& operator<<(prefixed_store<Bridges...>&& pstore, asset::basic_resource<AssetType>& res){
    //     pstore.add(res);
    //     return std::forward<prefixed_store<Bridges...>>(pstore);
    // }

    private:
        store_type& _store;
        std::string _prefix;
};


namespace detail {
    template <int I, typename Tuple>
    struct view_bridge_auto_resolver;

    template <int I, typename... Ts>
    struct view_bridge_auto_resolver<I, std::tuple<Ts...>> {
        using store_type = const_store<Ts...>;
        const store_type& _store;

        view_bridge_auto_resolver(const store_type& store) : _store(store) {}

        template <typename DataT, typename... Args>
        udho::view::resources::results apply(const std::string& lang, const std::string& prefix, const std::string& name, DataT&& data, Args&&... args) {
            using bridge_type = typename std::tuple_element<I, std::tuple<Ts...>>::type;
            using proxy_type  = udho::view::resources::tmpl::proxy<bridge_type>;

            if (bridge_type::name() == lang) {
                proxy_type proxy = _store.template view<bridge_type>(prefix, name);
                return proxy(std::forward<DataT>(data), std::forward<Args>(args)...);
            }
            return view_bridge_auto_resolver<I + 1, std::tuple<Ts...>>(_store).template apply<DataT, Args...>(lang, prefix, name, std::forward<DataT>(data), std::forward<Args>(args)...);
        }

        const udho::view::data::bridges::view_header& header(const std::string& lang, const std::string& prefix, const std::string& name) const {
            using bridge_type = typename std::tuple_element<I, std::tuple<Ts...>>::type;

            if (bridge_type::name() == lang) {
                return _store.template header<bridge_type>(prefix, name);
            }
            return view_bridge_auto_resolver<I + 1, std::tuple<Ts...>>(_store).header(lang, prefix, name);
        }
    };

    template <typename... Ts>
    struct view_bridge_auto_resolver<sizeof...(Ts), std::tuple<Ts...>>{
        using store_type = const_store<Ts...>;
        const store_type& _store;

        view_bridge_auto_resolver(const store_type& store) : _store(store) {}

        template <typename DataT, typename... Args>
        udho::view::resources::results apply(const std::string& lang, const std::string& prefix, const std::string& name, DataT&& data, Args&&... args) {
            throw std::runtime_error{"Requested language "+lang+" not present in the store"};
        }
        const udho::view::data::bridges::view_header& header(const std::string& lang, const std::string& prefix, const std::string& name) const {
            throw std::runtime_error{"Requested language "+lang+" not present in the store"};
        }
    };

    inline bool parse_view_address(const std::string& subject, std::string& o_bridge, std::string& o_prefix, std::string& o_view){
        static std::string bridge_sep = "://";
        std::string::const_iterator it = std::find_first_of(subject.cbegin(), subject.cend(), bridge_sep.begin(), bridge_sep.end());
        if(it != subject.cend()){
            std::string bridge{subject.cbegin(), it};
            std::advance(it, bridge_sep.size());
            std::string::const_iterator pos = std::find(it, subject.cend(), '/');
            if(pos != subject.cend()){
                std::string prefix{it, pos};
                std::string view{pos+1, subject.cend()};

                o_bridge = bridge;
                o_prefix = prefix;
                o_view   = view;

                return true;
            }
        }
        return false;
    }
}

/**
 * @brief Once a resource store is constructed, it is accessed through a const_store.
 * This ensures that the resources are added to the store only once during the initialization and never again.
 * The const_store only supports readonly operations on the resource store (including rendering of the views).
 * The resource store has to be locked before it can be passed to the const_store. This ensures that there won't
 * be any write operation on the resource store once the server starts serving the resources. This also makes it
 * easy in terms of handling multithreaded environment, because even though there could no concurrent reads, there
 * won't be any concurrent write operations.
 *
 * @tparam XBridges The bridges accessible by the const_store.
 */
template <typename... XBridges>
struct const_store{
    using self_type = const_store<XBridges...>;
    using asset_substore_readonly_js     = udho::view::resources::asset::const_substore<asset::type::js>;
    using asset_substore_readonly_css    = udho::view::resources::asset::const_substore<asset::type::css>;
    using asset_substore_readonly_img    = udho::view::resources::asset::const_substore<asset::type::img>;
    using asset_substore_readonly_type   = udho::view::resources::asset::const_store;
    using tmpl_const_multi_substore_type = udho::view::resources::tmpl::const_store<XBridges...>;
    using view_autoresolver_type         = detail::view_bridge_auto_resolver<0, std::tuple<XBridges...>>;

    static constexpr const std::size_t bridges_count = sizeof...(XBridges);

    /**
     * @brief construct a const_store from a resource store
     * @tparam Bridges A suuperset of Bridges
     * @param store a resource store supporting superset of bridges
     */
    template <typename... Bridges>
    const_store(const store<Bridges...>& store): _tmpls_proxy(store._tmpls), _assets(store._assets), _assets_js(_assets), _assets_css(_assets), _assets_img(_assets) { }

    /**
     * @brief a const interface to a substore for view templates
     * @tparam XBridgeT The intended bridge
     * @return returns a const_substore specialized for accessing view templates for a specified bridge
     */
    template <typename XBridgeT>
    udho::view::resources::tmpl::const_substore<XBridgeT> tmpl() const { return _tmpls_proxy.template substore<XBridgeT>(); }

    /**
     * @brief access a view through a bridge by the prefix and the name
     * @tparam XBridgeT the bridge on which the intended view template is registered.
     * @param prefix prefix of the view template
     * @param name name of the biew template
     */
    template <typename XBridgeT>
    udho::view::resources::tmpl::proxy<XBridgeT> view(const std::string& prefix, const std::string& name) const {
        udho::view::resources::tmpl::const_substore<XBridgeT> tmpl_substore = tmpl<XBridgeT>();
        return tmpl_substore.view(prefix, name);
    }

    /**
     * @brief renders a view and returns result while matching the bridge at runtime.
     * @tparam DataT type of the data passed to the view template
     * @tparam Args... types of the additional arguments passed to teh view template
     * @param lang name of the bridge e.g. lua
     * @param prefix view prefix
     * @param name view name
     * @param data data passed to the view
     * @param args... additional arguments
     */
    template <typename DataT, typename... Args>
    udho::view::resources::results render(const std::string& lang, const std::string& prefix, const std::string& name, DataT&& data, Args&&... args) const{
        view_autoresolver_type renderer{*this};
        return renderer.apply(lang, prefix, name, std::forward<DataT>(data), std::forward<Args>(args)...);
    }
    /**
     * @brief Renders a view and returns the result while matching the bridge at runtime.
     *
     * This function parses the `view_address` string to extract the language, prefix, and name, and then delegates the
     * rendering to another overload of `render` function which takes lang, prefix and path as seperate arguments.
     * Throws exception if parsing fails.
     *
     * @tparam DataT Type of the data passed to the view template.
     * @tparam Args... Types of the additional arguments passed to the view template.
     * @param view_address The view address in the format `lang://prefix/name`.
     * @param data Data passed to the view.
     * @param args... Additional arguments passed to the view.
     * @return udho::view::resources::results The result of rendering the view.
     * @throws std::runtime_error If the `view_address` cannot be parsed.
     */
    template <typename DataT, typename... Args>
    udho::view::resources::results render(std::string view_address, DataT&& data, Args&&... args) const{
        std::string lang, prefix, name;
        bool parsed = detail::parse_view_address(view_address, lang, prefix, name);

        if (parsed) {
            return render<DataT, Args...>(lang, prefix, name, std::forward<DataT>(data), std::forward<Args>(args)...);
        } else {
            throw std::runtime_error{"Failed to parse view address " + view_address};
        }
    }

    /**
     * @brief Returns metadata for a view registered on a specific bridge.
     * @tparam XBridgeT Bridge on which the view is registered.
     * @param prefix View prefix.
     * @param name View name.
     * @return Parsed metadata header for the view.
     */
    template <typename XBridgeT>
    const udho::view::data::bridges::view_header& header(const std::string& prefix, const std::string& name) const {
        udho::view::resources::tmpl::const_substore<XBridgeT> tmpl_substore = tmpl<XBridgeT>();
        return tmpl_substore.header(prefix, name);
    }

    /**
     * @brief Returns view metadata while selecting the bridge at runtime.
     * @param lang Bridge language name.
     * @param prefix View prefix.
     * @param name View name.
     * @return Parsed metadata header for the view.
     * @throws std::runtime_error If the requested bridge is unavailable.
     */
    const udho::view::data::bridges::view_header& header(const std::string& lang, const std::string& prefix, const std::string& name) const {
        view_autoresolver_type renderer{*this};
        return renderer.header(lang, prefix, name);
    }

    /**
     * @brief Returns metadata for a view identified by a complete address.
     * @param view_address Address in `language://prefix/name` form.
     * @return Parsed metadata header for the view.
     * @throws std::runtime_error If the address cannot be parsed or its bridge
     *         is unavailable.
     */
    const udho::view::data::bridges::view_header& header(std::string view_address) const{
        std::string lang, prefix, name;
        bool parsed = detail::parse_view_address(view_address, lang, prefix, name);

        if (parsed) {
            return header(lang, prefix, name);
        } else {
            throw std::runtime_error{"Failed to parse view address " + view_address};
        }
    }


    /**
     * @brief const reference to the assets store
     */
    const asset_substore_readonly_type& assets() const { return _assets; }

    /**
     * @brief const reference to the asset substore specific for javascript
     */
    const asset_substore_readonly_js&  js()  const { return _assets_js;  }
    /**
     * @brief const reference to the asset substore specific for stylesheets
     */
    const asset_substore_readonly_css& css() const { return _assets_css; }
    /**
     * @brief const reference to the asset substore specific for images
     */
    const asset_substore_readonly_img& img() const { return _assets_img; }

    /**
     * @brief Returns the label of the view bridge.
     * @return Bridge collection label.
     */
    std::string bridges_label() const {
        return _tmpls_proxy.label();
    }

    /**
     * @brief resources::const_store<XBridges...> is exposed to lua with the following properties
     * +------+------------------------+
     * | js   | property               |
     * | css  | property               |
     * | img  | property               |
     * | view | function(prefix, name) |
     * +------+------------------------+
     */
    friend auto metatype(udho::view::data::type<const_store<XBridges...>>){
        using namespace udho::view::data;

        return assoc("resources_const_store"),
            fvar("assets", &self_type::assets),
            fvar("js",     &self_type::js),
            fvar("css",    &self_type::css),
            fvar("img",    &self_type::img),
            fvar("bridges_label", &self_type::bridges_label);
    }

    // const_store_prefixed<XBridges...> operator[] (const std::string& prefix) const { return const_store_prefixed<XBridges...>{*this, prefix}; }

    private:
        tmpl_const_multi_substore_type _tmpls_proxy;
        asset_substore_readonly_type   _assets;
        asset_substore_readonly_js     _assets_js;
        asset_substore_readonly_css    _assets_css;
        asset_substore_readonly_img    _assets_img;

};

template <>
struct const_store<>{
    using self_type = const_store<>;
    using asset_substore_readonly_js     = udho::view::resources::asset::const_substore<asset::type::js>;
    using asset_substore_readonly_css    = udho::view::resources::asset::const_substore<asset::type::css>;
    using asset_substore_readonly_img    = udho::view::resources::asset::const_substore<asset::type::img>;
    using asset_substore_readonly_type   = udho::view::resources::asset::const_store;

    static constexpr const std::size_t bridges_count = 0;

    /**
     * @brief construct a const_store from a resource store
     * @tparam Bridges A suuperset of Bridges
     * @param store a resource store supporting superset of bridges
     */
    const_store(const store<>& store): _assets(store._assets), _assets_js(_assets), _assets_css(_assets), _assets_img(_assets) { }

    /**
     * @brief const reference to the assets store
     */
    const asset_substore_readonly_type& assets() const { return _assets; }

    /**
     * @brief const reference to the asset substore specific for javascript
     */
    const asset_substore_readonly_js&  js()  const { return _assets_js;  }
    /**
     * @brief const reference to the asset substore specific for stylesheets
     */
    const asset_substore_readonly_css& css() const { return _assets_css; }
    /**
     * @brief const reference to the asset substore specific for images
     */
    const asset_substore_readonly_img& img() const { return _assets_img; }

    /**
     * @brief resources::const_store<XBridges...> is exposed to lua with the following properties
     * +------+------------------------+
     * | js   | property               |
     * | css  | property               |
     * | img  | property               |
     * | view | function(prefix, name) |
     * +------+------------------------+
     */
    friend auto metatype(udho::view::data::type<const_store<>>){
        using namespace udho::view::data;

        return assoc("resources_const_store"),
            fvar("assets", &self_type::assets),
            fvar("js",   &self_type::js),
            fvar("css",  &self_type::css),
            fvar("img",  &self_type::img);
    }

    // const_store_prefixed<XBridges...> operator[] (const std::string& prefix) const { return const_store_prefixed<XBridges...>{*this, prefix}; }

    private:
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


/** @} */

}
}
}

#endif // UDHO_VIEW_RESOURCES_STORE_H
