#ifndef UDHO_VIEW_LAYOUT_LAYOUT_H
#define UDHO_VIEW_LAYOUT_LAYOUT_H

#include <type_traits>
#include <udho/view/tmpl/layout/loader.h>
#include <udho/view/tmpl/layout/property_map.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/presenter.h>
#include <udho/view/resources/fwd.h>
#include <udho/view/resources/asset/store.h>

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

template <typename DocumentT, typename PresenterT>
struct basic_layout;

template <typename PlaceholderT, typename PresenterT>
struct basic_layout<basic_document<PlaceholderT>, PresenterT>{
    using placeholder_type  = PlaceholderT;
    using document_type     = basic_document<PlaceholderT>;
    using presenter_type    = PresenterT;
    using loader_js         = typename document_type::loader_js;
    using loader_css        = typename document_type::loader_css;

    static_assert(std::is_base_of<basic_presenter<document_type>, PresenterT>::value);

    template <typename... Bridges>
    basic_layout(const udho::view::resources::const_store<Bridges...>& store): _document(store), _presenter(_document) {}

    loader_js& js() { return _document.js(); }
    loader_css& css() { return _document.js(); }

    const loader_js& js() const { return _document.js(); }
    const loader_css& css() const { return _document.js(); }

    layout::document_preamble& preamble() { return _document.preamble(); }
    const layout::document_preamble& preamble() const { return _document.preamble(); }

    template <typename Key>
    auto operator[](const Key& key){ return _document[key]; }
    template <typename Key>
    auto operator[](const Key& key) const { return _document[key]; }

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

};

template <typename PresenterT = default_presenter<standard_document>>
using standard_layout_ = basic_layout<standard_document, PresenterT>;

using standard_layout = standard_layout_<>;

}
}
}
}

#endif // UDHO_VIEW_LAYOUT_LAYOUT_H
