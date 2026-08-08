#ifndef UDHO_VIEW_TMPL_LAYOUT_RENDERER_H
#define UDHO_VIEW_TMPL_LAYOUT_RENDERER_H

#include <type_traits>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/store.h>
#include <udho/view/bridges/header.h>
#include <udho/view/tmpl/layout/repr.h>
#include <udho/www/components/resources.h>
#include <boost/type_traits/has_left_shift.hpp>
#include <cassert>


namespace udho{
namespace view{
namespace tmpl{
namespace layout{

/**
 * @addtogroup DoxyG_view_tmpl_layout
 * @{
 */


namespace helper{
template <typename T>
struct is_streamable {
    static constexpr bool value = boost::has_left_shift<std::ostream, T>::value;
};

template <typename T>
constexpr bool is_streamable_v = is_streamable<T>::value;

/**
 * @brief Obtain the placeholder proxy type associated with a layout key.
 *
 * Also exposes whether the placeholder accepts multiple values.
 *
 * @tparam LayoutT Layout type.
 * @tparam KeyT Placeholder key type.
 */
template <typename LayoutT, typename KeyT>
struct proxy_type{
    using type = typename LayoutT::placeholders_type::template proxy_type<KeyT>;

    static_assert(!std::is_void<type>::value, "Placeholder Key out of range in Layout");

    /**
     * @brief Whether the selected placeholder accepts multiple values.
     */
    static bool constexpr multiple = type::multiple;
};

}

/**
 * @brief Imports asset requirements declared by a rendered view.
 *
 * View headers distinguish between externally included and embedded
 * JavaScript and CSS resources. This helper transfers those requirements to
 * the corresponding layout asset loaders.
 *
 * @tparam LayoutT Layout receiving the asset selections.
 */
template <typename LayoutT>
struct header_renderer{
    using layout_type  = LayoutT;

    layout_type&        _layout;

    /**
     * @brief Construct a header renderer for a layout.
     * @param layout Layout whose asset loaders will be updated.
     */
    header_renderer(layout_type& layout): _layout(layout) {}

    /**
     * @brief Apply the asset requirements from a rendered view header.
     *
     * Included resources are added as external assets and embedded resources are
     * added as inline assets.
     *
     * @param header Header produced by the rendered view.
     *
     * @throws std::out_of_range If the view header refers to an asset that was
     *         not registered in the resource store.
     */
    void apply(const udho::view::data::bridges::view_header& header){
        const udho::view::data::bridges::view_header::includes_& includes = header.includes;
        const udho::view::data::bridges::view_header::includes_& embeds   = header.embeds;

        auto includes_js    = includes.js();
        auto includes_css   = includes.css();
        auto embeds_js      = embeds.js();
        auto embeds_css     = embeds.css();

        for(const auto& js:  includes_js) { _layout.js() .add( js.prefix,  js.name, false); }
        for(const auto& css: includes_css){ _layout.css().add(css.prefix, css.name, false); }
        for(const auto& js:  embeds_js)   { _layout.js() .add( js.prefix,  js.name, true);  }
        for(const auto& css: embeds_css)  { _layout.css().add(css.prefix, css.name, true);  }
    }
};




/**
 * @brief Rendering proxy returned when accessing a layout placeholder.
 *
 * The appropriate specialization is selected according to whether the
 * placeholder accepts one value or multiple values.
 *
 * @tparam KeyT Placeholder key type.
 * @tparam LayoutT Owning layout type.
 * @tparam IsMultiple Whether the placeholder accepts multiple values.
 */
template <typename KeyT, typename LayoutT, bool IsMultiple = helper::proxy_type<LayoutT, KeyT>::multiple >
struct renderer;

/**
 * @brief Renderer for a multi-valued layout placeholder.
 *
 * Rendering appends one new value to the placeholder. When a view is mapped
 * through the placeholder properties, the input data is rendered through the
 * resource store and the resulting view output is appended. Otherwise,
 * stream-insertable input is converted directly to text.
 *
 * Asset requirements declared by mapped views are transferred to the layout.
 *
 * @tparam KeyT Placeholder key type.
 * @tparam LayoutT Owning layout type.
 */
template <typename KeyT, typename LayoutT>
struct renderer<KeyT, LayoutT, true>: header_renderer<LayoutT>{
    using layout_type  = LayoutT;
    using key_type     = KeyT;
    using context_type = typename LayoutT::context_type;
    using portal_type  = typename context_type::portal_type;
    using composition_type = typename portal_type::composition_type;
    using resource_component_type = typename composition_type::template component_at<udho::www::feature::resources_storage, 0>;
    using store_type   = typename resource_component_type::store_type;
    using header_renderer_type = header_renderer<LayoutT>;

    context_type&       _ctx;
    layout_type&        _layout;
    key_type            _key;
    const store_type&   _store;

    /**
     * @brief Construct a renderer for a multi-valued placeholder.
     *
     * @param ctx Request/rendering context.
     * @param layout Owning layout.
     * @param key Placeholder key.
     */
    renderer(context_type& ctx, layout_type& layout, const key_type& key): header_renderer_type(layout), _ctx(ctx), _layout(layout), _key(key), _store(ctx.portal().resources()) {}

    /**
     * @brief Render and append a value to the placeholder.
     *
     * If a view address is configured for the placeholder, `d` is passed to that
     * view through the resource store. Otherwise, `d` is converted using stream
     * insertion when supported.
     *
     * For a non-streamable, unmapped value, an empty string is appended.
     *
     * @tparam Data Input data type.
     * @param d Data to render or convert.
     * @return Reference to this renderer.
     *
     * @throws std::out_of_range If a mapped view or one of its declared assets
     *         cannot be resolved.
     */
    template <typename Data>
    renderer& render(Data&& d){
        using proxy_type = decltype(_layout.document()[_key]);
        assert(proxy_type::multiple);

        proxy_type proxy = _layout.document()[_key];

        std::string view_addr = _layout.properties(_key).view();
        if(!view_addr.empty()){
            _render(proxy, view_addr, std::forward<Data>(d));
        } else {
            std::stringstream str_stream;
            if constexpr (helper::is_streamable_v<Data>) {
                str_stream << d;
            }

            proxy += str_stream.str();
        }
        return *this;
    }

    template <typename Data>
    renderer& render(const std::string& view_addr, Data&& d){
        using proxy_type = decltype(_layout.document()[_key]);
        assert(proxy_type::multiple);

        proxy_type proxy = _layout.document()[_key];

        if(!view_addr.empty()){
            _render(proxy, view_addr, std::forward<Data>(d));
        }

        return *this;
    }


    /**
     * @brief Render and append a value to the placeholder.
     *
     * Equivalent to calling render().
     *
     * @tparam Data Input data type.
     * @param d Data to render or convert.
     * @return Reference to this renderer.
     */
    template <typename Data>
    renderer& operator+=(Data&& d) {
        return render(std::forward<Data>(d));
    }

    /**
     * @brief Test whether this placeholder contains at least one value.
     * @return `true` when one or more values have been appended.
     */
    bool exists() const {
        return _layout.document()[_key].exists();
    }

    /**
     * @brief Return the number of values stored in this placeholder.
     */
    std::size_t count() const {
        return _layout.document()[_key].count();
    }

private:
    template <typename ProxyT, typename Data>
    renderer& _render(ProxyT& proxy, const std::string& view_addr, Data&& d){
        using data_type = std::decay_t<Data>;

        using proxy_type = decltype(_layout.document()[_key]);
        static_assert(std::is_same_v<proxy_type, std::decay_t<ProxyT>>);

        static_assert((_store.bridges_count > 0 || udho::view::tmpl::layout::has_repr_v<data_type> || helper::is_streamable_v<data_type>), "Cannot render a view from no-bridge resource store using data neither has udho::view::tmpl::layout::repr<Data> specialization nor streamable");

        if constexpr (_store.bridges_count > 0 ) {
            udho::view::resources::results results = _store.render(view_addr, std::forward<Data>(d), _ctx);
            proxy += results.str();

            const udho::view::data::bridges::view_header& header = _store.header(view_addr);
            header_renderer_type::apply(header);
        } else {
            if constexpr (udho::view::tmpl::layout::has_repr_v<data_type>) {
                udho::view::tmpl::layout::repr<data_type> repr(d);
                repr.include(_layout.css());
                repr.include(_layout.js());
                proxy += repr(_ctx);
            } else {
                static_assert(helper::is_streamable_v<data_type>);

                std::stringstream str_stream;
                str_stream << d;

                proxy += str_stream.str();
            }
        }

        return *this;
    }

};

/**
 * @brief Renderer for a single-valued layout placeholder.
 *
 * Rendering assigns the produced text to the placeholder. When a view is
 * mapped through the placeholder properties, the input data is rendered
 * through the resource store. Otherwise, stream-insertable input is converted
 * directly to text.
 *
 * Asset requirements declared by mapped views are transferred to the layout.
 *
 * @tparam KeyT Placeholder key type.
 * @tparam LayoutT Owning layout type.
 */
template <typename KeyT, typename LayoutT>
struct renderer<KeyT, LayoutT, false>: private header_renderer<LayoutT>{
    using layout_type  = LayoutT;
    using key_type     = KeyT;
    using context_type = typename LayoutT::context_type;
    using portal_type  = typename context_type::portal_type;
    using composition_type = typename portal_type::composition_type;
    using resource_component_type = typename composition_type::template component_at<udho::www::feature::resources_storage, 0>;
    using store_type   = typename resource_component_type::store_type;
    using header_renderer_type = header_renderer<LayoutT>;
    using placeholders_type = typename layout_type::placeholders_type;
    using proxy_type = typename placeholders_type::template proxy_type<key_type>;
    using const_proxy_type = typename placeholders_type::template const_proxy_type<key_type>;

    context_type&       _ctx;
    layout_type&        _layout;
    key_type            _key;
    const store_type&   _store;

    /**
     * @brief Construct a renderer for a single-valued placeholder.
     *
     * @param ctx Request/rendering context.
     * @param layout Owning layout.
     * @param key Placeholder key.
     */
    renderer(context_type& ctx, layout_type& layout, const key_type& key): header_renderer_type(layout), _ctx(ctx), _layout(layout), _key(key), _store(ctx.portal().resources()) {}

    /**
     * @brief Render and assign a value to the placeholder.
     *
     * If a view address is configured for the placeholder, `d` is passed to that
     * view through the resource store. Otherwise, `d` is converted using stream
     * insertion when supported.
     *
     * For a non-streamable, unmapped value, an empty string is assigned.
     *
     * @tparam Data Input data type.
     * @param d Data to render or convert.
     * @return Reference to this renderer.
     *
     * @throws std::out_of_range If a mapped view or one of its declared assets
     *         cannot be resolved.
     */
    template <typename Data>
    renderer& render(Data&& d){
        using proxy_type = decltype(_layout.document()[_key]);
        assert(!proxy_type::multiple);

        proxy_type proxy = _layout.document()[_key];

        const udho::view::tmpl::layout::proxy::placeholder_properties& p = _layout.properties(_key);
        std::string view_addr = p.view();
        if(!view_addr.empty()){
            _render(proxy, view_addr, std::forward<Data>(d));
        } else {
            std::stringstream str_stream;
            if constexpr (helper::is_streamable_v<Data>) {
                str_stream << d;
            }

            proxy = str_stream.str();
        }
        return *this;
    }

    template <typename Data>
    renderer& render(const std::string& view_addr, Data&& d){
        using proxy_type = decltype(_layout.document()[_key]);
        assert(!proxy_type::multiple);

        proxy_type proxy = _layout.document()[_key];

        if(!view_addr.empty()){
            _render(proxy, view_addr, std::forward<Data>(d));
        }

        return *this;
    }

    /**
     * @brief Render and assign a value to the placeholder.
     *
     * Equivalent to calling render().
     *
     * @tparam Data Input data type.
     * @param d Data to render or convert.
     * @return Reference to this renderer.
     */
    template <typename Data>
    renderer& operator=(Data&& d) {
        return render(std::forward<Data>(d));
    }

    /**
     * @brief Test whether the placeholder contains a value.
     */
    bool exists() const {
        return _layout.document()[_key].exists();
    }

    /**
     * @brief Return the number of values represented by the placeholder.
     *
     * For a single-valued placeholder the result is normally either zero or one.
     */
    std::size_t count() const {
        return _layout.document()[_key].count();
    }

private:
    template <typename ProxyT, typename Data>
    renderer& _render(ProxyT& proxy, const std::string& view_addr, Data&& d){
        using data_type = std::decay_t<Data>;

        using proxy_type = decltype(_layout.document()[_key]);
        static_assert(std::is_same_v<proxy_type, std::decay_t<ProxyT>>);

        static_assert((_store.bridges_count > 0 || udho::view::tmpl::layout::has_repr_v<data_type> || helper::is_streamable_v<data_type>), "Cannot render a view from no-bridge resource store using data neither has udho::view::tmpl::layout::repr<Data> specialization nor streamable");

        if constexpr (_store.bridges_count > 0 ) {
            udho::view::resources::results results = _store.render(view_addr, std::forward<Data>(d), _ctx);
            proxy = results.str();

            const udho::view::data::bridges::view_header& header = _store.header(view_addr);
            header_renderer_type::apply(header);
        } else {
            if constexpr (udho::view::tmpl::layout::has_repr_v<data_type>) {
                udho::view::tmpl::layout::repr<data_type> repr(d);
                repr.include(_layout.css());
                repr.include(_layout.js());
                proxy = repr(_ctx);
            } else {
                static_assert(helper::is_streamable_v<data_type>);

                std::stringstream str_stream;
                str_stream << d;

                proxy = str_stream.str();
            }
        }

        return *this;
    }
};

/** @} */

}
}
}
}


#endif // UDHO_VIEW_TMPL_LAYOUT_RENDERER_H
