#ifndef UDHO_VIEW_LAYOUT_LAYOUT_H
#define UDHO_VIEW_LAYOUT_LAYOUT_H

#include <type_traits>
#include <udho/view/tmpl/layout/loader.h>
#include <udho/view/tmpl/layout/property_map.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/presenter.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/store.h>
#include <udho/view/bridges/header.h>
#include <udho/www/components/resources.h>
#include <boost/type_traits/has_left_shift.hpp>
#include <cassert>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

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

template <typename ContextT, typename DocumentT, typename PresenterT>
struct basic_layout;

template <typename DocumentT, typename PresenterT>
struct basic_layout_impl;

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
template <typename KeyT, typename LayoutT, bool IsMultiple = helper::proxy_type<LayoutT, KeyT>::multiple>
struct renderer;


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
            if constexpr (_store.bridges_count > 0 ) {
                udho::view::resources::results results = _store.render(view_addr, std::forward<Data>(d), _ctx);
                proxy += results.str();

                const udho::view::data::bridges::view_header& header = _store.header(view_addr);
                header_renderer_type::apply(header);
            } else {
                assert(0 == 1 && "trying to render a view from non-view resource store");
            }
        } else {
            std::stringstream str_stream;
            if constexpr (helper::is_streamable_v<Data>) {
                str_stream << d;
            }

            proxy += str_stream.str();
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

        const proxy::placeholder_properties& p = _layout.properties(_key);
        std::string view_addr = p.view();
        if(!view_addr.empty()){
            if constexpr (_store.bridges_count > 0 ) {
                udho::view::resources::results results = _store.render(view_addr, std::forward<Data>(d), _ctx);
                proxy = results.str();

                const udho::view::data::bridges::view_header& header = _store.header(view_addr);
                header_renderer_type::apply(header);
            } else {
                assert(0 == 1 && "trying to render a view from non-view resource store");
            }
        } else {
            std::stringstream str_stream;
            if constexpr (helper::is_streamable_v<Data>) {
                str_stream << d;
            }

            proxy = str_stream.str();
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

    // typename proxy_type::value_type& operator*(){
    //     return _layout.document()[_key].value();
    // }

    // typename proxy_type::value_type& operator->(){
    //     return _layout.document()[_key].value();
    // }

    // const typename proxy_type::value_type& operator*() const{
    //     return _layout.document()[_key].value();
    // }

    // const typename proxy_type::value_type& operator->() const{
    //     return _layout.document()[_key].value();
    // }
};

/**
 * @brief Pimpl storage for a layout document and its presenter.
 *
 * The presenter is constructed with a reference to the stored document.
 * Keeping the two objects in the same implementation object guarantees that
 * the presenter's document reference remains valid for the implementation
 * object's lifetime.
 *
 * @tparam PlaceholderT Placeholder container used by the document.
 * @tparam PresenterT Presenter used to compose the document.
 */
template <typename PlaceholderT, typename PresenterT>
struct basic_layout_impl<basic_document<PlaceholderT>, PresenterT>{
    using placeholder_type  = PlaceholderT;
    using document_type     = basic_document<PlaceholderT>;
    using placeholders_type = typename document_type::placeholders_type;
    using presenter_type    = PresenterT;
    using loader_js         = typename document_type::loader_js;
    using loader_css        = typename document_type::loader_css;
    using mapped_view       = std::tuple<std::string, std::string>;
    using on_delete_f       = std::function<void ()>;

    static_assert(std::is_base_of<basic_presenter<document_type>, PresenterT>::value);

    /**
     * @brief Construct the document and its presenter.
     *
     * @tparam Bridges Bridges supported by the resource store.
     * @param store Resource store used to initialize the document asset loaders.
     */
    template <typename... Bridges>
    basic_layout_impl(const udho::view::resources::const_store<Bridges...>& store): _document(store), _presenter(_document) {}

    /**
     * @brief Access the stored document.
     */
    document_type& document() { return _document; }

    /**
     * @brief Access the stored document.
     */
    const document_type& document() const { return _document; }

    /**
     * @brief Access the presenter associated with the stored document.
     */
    presenter_type& presenter() { return _presenter; }

    /**
     * @brief Access the presenter associated with the stored document.
     */
    const presenter_type& presenter() const { return _presenter; }


    private:
        document_type   _document;
        presenter_type  _presenter;

};

/**
 * @brief Basic layout integrates the presenter with the document.
 * @tparam ContextT Request/rendering context type.
 * @tparam PlaceholderT Placeholder container used by the document.
 * @tparam PresenterT Presenter used to compose the final output.
 *
 * ## Adding Menus
 *
 * ## Unmapped Placeholders
 * If no view is mapped to a placeholder then the following code will try to convert data to string and append it as
 * contents of that placeholder.
 * @code
 * layout[placeholders::central] = data
 * layout[placeholders::central] += data
 * @endcode
 *
 * ## Asset Loading
 * When the layout evaluates the views it collects the asset requests such as ja, css etc.. from the view meta tags,
 * which is then passed on to the preamble which is supposed to resolve them accordingly by generating corresponding
 * meta tags, script tags, style tags etc..
 *
 * ## Mapped Placeholders
 * Views can be mapped to placeholders
 * @code
 * layout[placeholders::central].view("lua://prefix/name")
 * @endcode
 * Once mapped a C++ object (with metatype bindings) can be passed to it.
 * If the placeholder is meant for accomodating multiple values then use += operator, otherwise using = operator.
 * @code
 * layout[placeholders::central] = data
 * layout[placeholders::central] += data
 * @endcode
 * The view function used for mapping views with the placeholders also returns the same proxy object from the operator[].
 * @code
 * layout[placeholders::central].view<lua>(prefix, name) = data
 * layout[placeholders::central].view<lua>(prefix, name) += data
 * @endcode
 */
template <typename ContextT, typename PlaceholderT, typename PresenterT>
struct basic_layout<ContextT, basic_document<PlaceholderT>, PresenterT>{
    using placeholder_type          = PlaceholderT;
    using document_type             = basic_document<placeholder_type>;
    using placeholders_type         = typename document_type::placeholders_type;
    using presenter_type            = PresenterT;
    using basic_layout_impl_type    = basic_layout_impl<document_type, presenter_type>;
    using loader_js                 = typename basic_layout_impl_type::loader_js;
    using loader_css                = typename basic_layout_impl_type::loader_css;
    using context_type              = ContextT;
    using mapped_view               = std::tuple<std::string, std::string>;
    using basic_layout_pimpl_type   = std::shared_ptr<basic_layout_impl_type>;
    using basic_layout_             = basic_layout<ContextT, basic_document<placeholder_type>, PresenterT>;

    template <typename KeyT, typename LayoutT, bool>
    friend struct renderer;

    /**
     * @brief Construct a layout for a rendering context.
     *
     * Creates a shared implementation containing a document initialized from the
     * context's resource store and a presenter bound to that document.
     *
     * @param ctx Rendering context written to when the layout is finalized.
     */
    explicit basic_layout(context_type ctx): _context(ctx), _pimpl(std::make_shared<basic_layout_impl_type>(ctx.portal().resources())), _finished(false) { }

    /**
     * @brief Copy a layout.
     *
     * The copy retains its own context and completion flag but shares the
     * document and presenter implementation with `other`.
     *
     * @param other Layout to copy.
     *
     * @note Mutations made through either copy affect the shared document.
     *       The `_finished` flag is copied by value rather than shared.
     */
    basic_layout(const basic_layout_& other): _context(other._context), _pimpl(other._pimpl), _finished(other._finished) {}

    /**
     * @brief Obtain a renderer for a placeholder.
     *
     * The returned renderer supports assignment for single-valued placeholders
     * and append operations for multi-valued placeholders.
     *
     * @tparam Key Placeholder key type.
     * @param key Placeholder key.
     * @return Renderer specialized for the placeholder's multiplicity.
     */
    template <typename Key>
    auto operator[](const Key& key){
        using renderer_type = renderer<Key, basic_layout_>;
        return renderer_type{_context, std::ref(*this), key};
    }

    /**
     * @brief Obtain a renderer for a placeholder through a const layout.
     *
     * @tparam Key Placeholder key type.
     * @param key Placeholder key.
     * @return Renderer specialized for the placeholder's multiplicity.
     *
     * @note The current renderer interface performs mutations. Consider whether
     *       a distinct read-only renderer should be returned by this overload.
     */
    template <typename Key>
    auto operator[](const Key& key) const {
        using renderer_type = renderer<Key, basic_layout_>;
        return renderer_type{_context, std::ref(*this), key};
    }

    /**
     * @brief Access the document's JavaScript asset loader.
     */
    loader_js& js() { return _pimpl->document().js(); }

    /**
     * @brief Access the document's CSS asset loader.
     */
    loader_css& css() { return _pimpl->document().css(); }

    /**
     * @brief Access the document's JavaScript asset loader.
     */
    const loader_js& js() const { return _pimpl->document().js(); }
    /**
     * @brief Access the document's CSS asset loader.
     */
    const loader_css& css() const { return _pimpl->document().css(); }

    /**
     * @brief Access the document preamble.
     */
    layout::document_preamble& preamble() { return _pimpl->document().preamble(); }
    /**
     * @brief Access the document preamble.
     */
    const layout::document_preamble& preamble() const { return _pimpl->document().preamble(); }

    /**
     * @brief Access the presentation properties of a placeholder.
     *
     * Properties include its HTML tag configuration, attributes, classes, ID,
     * and optional mapped-view address.
     *
     * @tparam Key Placeholder key type.
     * @param key Placeholder key.
     * @return Mutable placeholder properties.
     */
    template <typename Key>
    proxy::placeholder_properties& properties(const Key& key){ return _pimpl->document().properties(key); }
    /**
     * @brief Access the presentation properties of a placeholder.
     *
     * @tparam Key Placeholder key type.
     * @param key Placeholder key.
     * @return Read-only placeholder properties.
     */
    template <typename Key>
    const proxy::placeholder_properties& properties(const Key& key) const { return _pimpl->document().properties(key); }

    private:

        /**
         * @brief finish the layout by presenting it to the stream associated with the context
         * @details calls the operator() overload of the presenter with the context as the only argument.
         *          afterwards finishes the context
         */
        void finish() {
            if(!_finished){
                udho::net::ostream_view ostream_view = _context.ostream().view();
                _pimpl->presenter()(ostream_view);
                _finished = true;
                ostream_view.finish();
            }
        }
    public:

        /**
         * @brief Present and finalize the layout.
         *
         * Equivalent to calling the private finish() operation.
         *
         * Repeated calls on the same layout instance have no effect after the first
         * finalization.
         */
        void operator()() {
            finish();
        }

    private:

        /**
         * @brief Access the document held by the shared implementation.
         */
        document_type& document() { return _pimpl->document(); }
        /**
         * @brief Access the document held by the shared implementation.
         */
        const document_type& document() const { return _pimpl->document(); }

        /**
         * @brief Access the presenter held by the shared implementation.
         */
        presenter_type& presenter() { return _pimpl->presenter(); }
        /**
         * @brief Access the presenter held by the shared implementation.
         */
        const presenter_type& presenter() const { return _pimpl->presenter(); }

    private:
        context_type    _context;
        basic_layout_pimpl_type _pimpl;
        bool _finished;

};

/**
 * @brief Standard layout using standard_document and its default presenter.
 *
 * @tparam ContextT Request/rendering context type.
 * @tparam PresenterT Presenter type. Defaults to the standard document's default presenter.
 */
template <typename ContextT, typename PresenterT = default_presenter<standard_document>>
using standard_layout = basic_layout<ContextT, standard_document, PresenterT>;

/**
 * @brief Create a layout for a placeholder container and context.
 *
 * @tparam PlaceholderT Placeholder container used by the layout document.
 * @tparam ContextT Rendering context type.
 * @tparam PresenterT Presenter type. Defaults to a placeholder-based
 *                    default presenter for the generated document type.
 *
 * @param ctx Rendering context associated with the layout.
 * @return Newly constructed layout.
 */
template <typename PlaceholderT, typename ContextT, typename PresenterT = default_presenter<basic_document<PlaceholderT>>>
basic_layout<ContextT, basic_document<PlaceholderT>, PresenterT> create(ContextT ctx){
    return basic_layout<ContextT, basic_document<PlaceholderT>, PresenterT>(ctx);
}




}
}
}
}

#endif // UDHO_VIEW_LAYOUT_LAYOUT_H
