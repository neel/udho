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

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

template <typename ContextT, typename DocumentT, typename PresenterT>
struct basic_layout;

template <typename KeyT, typename LayoutT, bool IsMultiple = LayoutT::placeholders_type::template proxy_type<KeyT>::multiple>
struct renderer;

template <typename KeyT, typename LayoutT>
struct renderer<KeyT, LayoutT, true>{
    using layout_type  = LayoutT;
    using key_type     = KeyT;
    using context_type = typename LayoutT::context_type;
    using store_type   = typename context_type::resource_store;

    context_type&       _ctx;
    layout_type&        _layout;
    key_type            _key;
    const store_type&   _store;

    renderer(context_type& ctx, layout_type& layout, const key_type& key): _ctx(ctx), _layout(layout), _key(key), _store(ctx.resources()) {}

    template <typename Data>
    renderer& render(const Data& d){
        using proxy_type = decltype(_layout._document[_key]);
        assert(proxy_type::multiple);

        proxy_type& proxy = _layout._document[_key];

        std::string view_addr = _layout.properties().view();
        if(!view_addr.empty()){
            udho::view::resources::results results = _store.render(view_addr, d, _ctx);
            proxy += results.str();
            // TODO capture the list of assets requested
        } else {
            std::stringstream str_stream;
            str_stream << d;

            proxy += str_stream.str();
        }
        return *this;
    }

    template <typename Data>
    renderer& operator=(const Data& d) {
        return render(d);
    }
};


template <typename KeyT, typename LayoutT>
struct renderer<KeyT, LayoutT, false>{
    using layout_type  = LayoutT;
    using key_type     = KeyT;
    using context_type = typename LayoutT::context_type;
    using store_type   = typename context_type::resource_store;

    context_type&       _ctx;
    layout_type&        _layout;
    key_type            _key;
    const store_type&   _store;

    renderer(context_type& ctx, layout_type& layout, const key_type& key): _ctx(ctx), _layout(layout), _key(key), _store(ctx.resources()) {}

    template <typename Data>
    renderer& render(const Data& d){
        using proxy_type = decltype(_layout._document[_key]);
        assert(proxy_type::multiple);

        proxy_type& proxy = _layout._document[_key];

        std::string view_addr = _layout.properties().view();
        if(!view_addr.empty()){
            udho::view::resources::results results = _store.render(view_addr, d, _ctx);
            proxy = results.str();
            // TODO capture the list of assets requested
        } else {
            std::stringstream str_stream;
            str_stream << d;

            proxy = str_stream.str();
        }
        return *this;
    }

    template <typename Data>
    renderer& operator=(const Data& d) {
        return render(d);
    }
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
    using placeholder_type  = PlaceholderT;
    using document_type     = basic_document<PlaceholderT>;
    using placeholders_type = typename document_type::placeholders_type;
    using presenter_type    = PresenterT;
    using loader_js         = typename document_type::loader_js;
    using loader_css        = typename document_type::loader_css;
    using context_type      = ContextT;
    using mapped_view       = std::tuple<std::string, std::string>;
    using basic_layout_     = basic_layout<ContextT, basic_document<PlaceholderT>, PresenterT>;

    template <typename KeyT, typename LayoutT>
    friend struct renderer;

    static_assert(std::is_base_of<basic_presenter<document_type>, PresenterT>::value);

    template <typename... Bridges>
    basic_layout(context_type ctx, const udho::view::resources::const_store<Bridges...>& store): _context(ctx), _document(store), _presenter(_document) {}

    loader_js& js() { return _document.js(); }
    loader_css& css() { return _document.js(); }

    const loader_js& js() const { return _document.js(); }
    const loader_css& css() const { return _document.js(); }

    layout::document_preamble& preamble() { return _document.preamble(); }
    const layout::document_preamble& preamble() const { return _document.preamble(); }

    template <typename Key>
    auto operator[](const Key& key){
        using renderer_type = renderer<Key, basic_layout_>;
        return renderer_type{_context, *this, key};
    }
    template <typename Key>
    auto operator[](const Key& key) const {
        using renderer_type = renderer<Key, basic_layout_>;
        return renderer_type{_context, *this, key};
    }

    template <typename Key>
    auto properties(const Key& key){ return _document[key]; }
    template <typename Key>
    auto properties(const Key& key) const { return _document[key]; }

    template <typename Stream>
    Stream& operator()(Stream& stream) const {
        return _presenter(stream);
    }

    private:
        document_type   _document;
        presenter_type  _presenter;
        context_type    _context;

};

template <typename ContextT, typename PresenterT = default_presenter<standard_document>>
using standard_layout = basic_layout<ContextT, standard_document, PresenterT>;

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_LAYOUT_H
