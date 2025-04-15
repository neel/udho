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

#include <boost/type_traits/has_left_shift.hpp>

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

template <typename LayoutT, typename KeyT>
struct proxy_type{
    using type = typename LayoutT::placeholders_type::template proxy_type<KeyT>;

    static_assert(!std::is_void<type>::value, "Placeholder Key out of range in Layout");

    static bool constexpr multiple = type::multiple;
};

}

template <typename ContextT, typename DocumentT, typename PresenterT>
struct basic_layout;

template <typename DocumentT, typename PresenterT>
struct basic_layout_impl;

template <typename KeyT, typename LayoutT, bool IsMultiple = helper::proxy_type<LayoutT, KeyT>::multiple>
struct renderer;

template <typename LayoutT>
struct header_renderer{
    using layout_type  = LayoutT;

    layout_type&        _layout;

    header_renderer(layout_type& layout): _layout(layout) {}
    void apply(const udho::view::data::bridges::view_header& header){
        const udho::view::data::bridges::view_header::includes_& includes = header.includes;

        auto includes_js = includes.js();
        auto includes_css = includes.css();

        for(const auto& js: includes_js){
            _layout.js().add(js.prefix, js.name);
        }
        for(const auto& css: includes_css){
            _layout.css().add(css.prefix, css.name);
        }
    }
};

template <typename KeyT, typename LayoutT>
struct renderer<KeyT, LayoutT, true>: header_renderer<LayoutT>{
    using layout_type  = LayoutT;
    using key_type     = KeyT;
    using context_type = typename LayoutT::context_type;
    using store_type   = typename context_type::resource_store;
    using header_renderer_type = header_renderer<LayoutT>;

    context_type&       _ctx;
    layout_type&        _layout;
    key_type            _key;
    const store_type&   _store;

    renderer(context_type& ctx, layout_type& layout, const key_type& key): header_renderer_type(layout), _ctx(ctx), _layout(layout), _key(key), _store(ctx.resources()) {}

    template <typename Data>
    renderer& render(Data&& d){
        using proxy_type = decltype(_layout.document()[_key]);
        assert(proxy_type::multiple);

        proxy_type proxy = _layout.document()[_key];

        std::string view_addr = _layout.properties(_key).view();
        if(!view_addr.empty()){
            udho::view::resources::results results = _store.render(view_addr, std::forward<Data>(d), _ctx);
            proxy += results.str();

            const udho::view::data::bridges::view_header& header = _store.header(view_addr);
            header_renderer_type::apply(header);
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
    renderer& operator+=(Data&& d) {
        return render(std::forward<Data>(d));
    }

    bool exists() const {
        return _layout.document()[_key].exists();
    }
    std::size_t count() const {
        return _layout.document()[_key].count();
    }
};


template <typename KeyT, typename LayoutT>
struct renderer<KeyT, LayoutT, false>: private header_renderer<LayoutT>{
    using layout_type  = LayoutT;
    using key_type     = KeyT;
    using context_type = typename LayoutT::context_type;
    using store_type   = typename context_type::resource_store;
    using header_renderer_type = header_renderer<LayoutT>;
    using placeholders_type = typename layout_type::placeholders_type;
    using proxy_type = typename placeholders_type::template proxy_type<key_type>;
    using const_proxy_type = typename placeholders_type::template const_proxy_type<key_type>;

    context_type&       _ctx;
    layout_type&        _layout;
    key_type            _key;
    const store_type&   _store;

    renderer(context_type& ctx, layout_type& layout, const key_type& key): header_renderer_type(layout), _ctx(ctx), _layout(layout), _key(key), _store(ctx.resources()) {}

    template <typename Data>
    renderer& render(Data&& d){
        using proxy_type = decltype(_layout.document()[_key]);
        assert(!proxy_type::multiple);

        proxy_type proxy = _layout.document()[_key];

        const proxy::placeholder_properties& p = _layout.properties(_key);
        std::string view_addr = p.view();
        if(!view_addr.empty()){
            udho::view::resources::results results = _store.render(view_addr, std::forward<Data>(d), _ctx);
            proxy = results.str();

            const udho::view::data::bridges::view_header& header = _store.header(view_addr);
            header_renderer_type::apply(header);
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
    renderer& operator=(Data&& d) {
        return render(std::forward<Data>(d));
    }

    bool exists() const {
        return _layout.document()[_key].exists();
    }
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

    template <typename... Bridges>
    basic_layout_impl(const udho::view::resources::const_store<Bridges...>& store, on_delete_f on_delete): _document(store), _presenter(_document), _on_delete(on_delete) {}

    document_type& document() { return _document; }
    const document_type& document() const { return _document; }

    presenter_type& presenter() { return _presenter; }
    const presenter_type& presenter() const { return _presenter; }

    ~basic_layout_impl() {
        _on_delete();
    }

    private:
        document_type   _document;
        presenter_type  _presenter;
        on_delete_f     _on_delete;

};

/**
 * @brief Basic layout integrates the presenter with the document.
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
 * layout[placeholders::central].view<lua>(prefix, name)
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
    using placeholder_type   = PlaceholderT;
    using document_type      = basic_document<PlaceholderT>;
    using placeholders_type  = typename document_type::placeholders_type;
    using presenter_type     = PresenterT;
    using basic_layout_impl_type  = basic_layout_impl<document_type, presenter_type>;
    using loader_js          = typename basic_layout_impl_type::loader_js;
    using loader_css         = typename basic_layout_impl_type::loader_css;
    using context_type       = ContextT;
    using mapped_view        = std::tuple<std::string, std::string>;
    using basic_layout_pimpl_type = std::shared_ptr<basic_layout_impl_type>;
    using basic_layout_             = basic_layout<ContextT, basic_document<PlaceholderT>, PresenterT>;

    template <typename KeyT, typename LayoutT, bool>
    friend struct renderer;

    basic_layout(context_type ctx): _context(ctx), _pimpl(std::make_shared<basic_layout_impl_type>(ctx.resources(), std::bind(&basic_layout_::on_delete, this))), _finished(false) { }
    basic_layout(const basic_layout_& other): _context(other._context), _pimpl(other._pimpl), _finished(other._finished) {}

    template <typename Key>
    auto operator[](const Key& key){
        using renderer_type = renderer<Key, basic_layout_>;
        return renderer_type{_context, std::ref(*this), key};
    }
    template <typename Key>
    auto operator[](const Key& key) const {
        using renderer_type = renderer<Key, basic_layout_>;
        return renderer_type{_context, std::ref(*this), key};
    }

    loader_js& js() { return _pimpl->document().js(); }
    loader_css& css() { return _pimpl->document().css(); }

    const loader_js& js() const { return _pimpl->document().js(); }
    const loader_css& css() const { return _pimpl->document().css(); }

    layout::document_preamble& preamble() { return _pimpl->document().preamble(); }
    const layout::document_preamble& preamble() const { return _pimpl->document().preamble(); }

    template <typename Key>
    proxy::placeholder_properties& properties(const Key& key){ return _pimpl->document().properties(key); }
    template <typename Key>
    const proxy::placeholder_properties& properties(const Key& key) const { return _pimpl->document().properties(key); }

    void finish() {
        if(!_finished){
            _pimpl->presenter()(_context);
            _finished = true;
            _context.finish();
        }
    }

    void operator()() {
        finish();
    }

    private:
        void on_delete() {
            // layout_imple being deleted
            std::cout << "layout is being deleted" << std::endl;
            finish();
        }

    private:
        document_type& document() { return _pimpl->document(); }
        const document_type& document() const { return _pimpl->document(); }

        presenter_type& presenter() { return _pimpl->presenter(); }
        const presenter_type& presenter() const { return _pimpl->presenter(); }

    private:
        context_type    _context;
        basic_layout_pimpl_type _pimpl;
        bool _finished;

};

template <typename ContextT, typename PresenterT = default_presenter<standard_document>>
using standard_layout = basic_layout<ContextT, standard_document, PresenterT>;

template <typename PlaceholderT, typename ContextT, typename PresenterT = default_presenter<basic_document<PlaceholderT>>>
basic_layout<ContextT, basic_document<PlaceholderT>, PresenterT> create(ContextT ctx){
    return basic_layout<ContextT, basic_document<PlaceholderT>, PresenterT>{ctx};
}




}
}
}
}

#endif // UDHO_VIEW_LAYOUT_LAYOUT_H
