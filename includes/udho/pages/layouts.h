#ifndef UDHO_PAGES_LAYOUTS_H
#define UDHO_PAGES_LAYOUTS_H

#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/tmpl/layout/loader.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/presenter.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/net/context.h>

namespace udho{
namespace pages{
namespace system{
namespace layouts{

namespace l = udho::view::tmpl::layout;

template <class DocumentT>
struct presenter: l::default_presenter<DocumentT, presenter<DocumentT>>{
    using default_presenter_ = l::default_presenter<DocumentT, presenter<DocumentT>>;
    using basic_presenter_   = typename default_presenter_::basic_presenter_;

    presenter(const DocumentT& doc): default_presenter_(doc) {}

    using default_presenter_::operator();
    template <typename KeyT, typename Stream>
    void operator()(const KeyT& key, const std::string& str, Stream& stream, std::size_t i, std::size_t len) const {
        basic_presenter_::present(key, str, stream, i, len);
    }

    template <typename KeyT, typename Stream>
    void operator()(const KeyT& key, const std::string& str, Stream& stream) const {
        basic_presenter_::present(key, str, stream);
    }

    template <typename Stream>
    void render(Stream& stream) const {
        stream << "<body>";
        stream <<   "<div class='system'>";
        basic_presenter_::document().apply(*this, stream);
        stream <<   "</div>";
        stream << "</body>";
    }
};

namespace placeholders = l::placeholders;

using minimal = l::basic_placeholder<
    l::spot<l::placeholders::segments::header>,
    l::spot<l::placeholders::segments::central>,
    l::spot<l::placeholders::segments::footer>
>;



template <typename ContextT>
auto listing(ContextT context) {
    namespace p = l::placeholders;

    using minimal_presenter = presenter<l::basic_document<minimal>>;

    auto layout = l::create<minimal, ContextT, minimal_presenter>(context);

    layout.preamble().title("Udho System");
    layout.preamble().classes("main");
    layout.properties(p::central).classes("central").view("lua://udho/listing");
    layout.properties(p::header) .classes("header") .view("lua://udho/header");
    layout.properties(p::footer) .classes("footer") .view("lua://udho/status");

    return layout;
}

}
}
}
}

#endif // UDHO_PAGES_LAYOUTS_H

