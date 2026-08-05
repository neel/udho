#ifndef UDHO_PAGES_LAYOUTS_H
#define UDHO_PAGES_LAYOUTS_H

#include <udho/view/data.h>
#include <udho/view/meta.h>
#include <udho/view/tmpl/layout/loader.h>
#include <udho/view/tmpl/layout/placeholder.h>
#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/presenter.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/pages/data.h>

namespace udho{
namespace pages{
namespace system{
namespace layouts{

namespace l = udho::view::tmpl::layout;

namespace places {
namespace segments {
    /**
     * @brief Placeholder tag for the system-page headline.
     * @ingroup DoxyG_pages
     */
    struct headline{};
    /**
     * @brief Placeholder tag for listing content.
     * @ingroup DoxyG_pages
     */
    struct listing{};
    /**
     * @brief Placeholder tag for route content.
     * @ingroup DoxyG_pages
     */
    struct routes{};
}
}


namespace placeholders = l::placeholders;

/**
 * @brief Placeholder set used by system listing and client-error pages.
 * @ingroup DoxyG_pages
 */
using minimal = l::basic_placeholder<
    l::spot<placeholders::segments::header>,
    l::spot<places::segments::headline>,
    l::spot<places::segments::routes>,
    l::spot<places::segments::listing>,
    l::spot<placeholders::segments::footer>
>;

namespace places{
    static segments::headline headline;
    static segments::listing  listing;
    static segments::routes   routes;
}

/**
 * @brief Presents the populated sections of a system-page document.
 * @tparam DocumentT Layout document type.
 * @ingroup DoxyG_pages
 */
template <class DocumentT>
struct presenter: l::default_presenter<DocumentT, presenter<DocumentT>>{
    using default_presenter_ = l::default_presenter<DocumentT, presenter<DocumentT>>;
    using basic_presenter_   = typename default_presenter_::basic_presenter_;

    /**
     * @brief Constructs a presenter for a document.
     * @param doc Document to present.
     */
    presenter(const DocumentT& doc): default_presenter_(doc), _doc(doc) {}

    using default_presenter_::operator();

    /**
     * @brief Writes the system-page sections to a stream.
     * @tparam Stream Output stream type.
     * @param stream Destination stream.
     */
    template <typename Stream>
    void render(Stream& stream) const {
        namespace p = l::placeholders;

        stream <<   "<div class='system'>";
        if(_doc[p::header].exists()){
            std::string value = *_doc[p::header];
            stream << std::move(value);
        }

        if(_doc[places::headline].exists()){
            std::string value = *_doc[places::headline];
            stream << std::move(value);
        }

        if(_doc[places::routes].exists()) {
            std::string value = *_doc[places::routes];
            stream << std::move(value);
        }

        if(_doc[places::listing].exists()) {
            std::string value = *_doc[places::listing];
            stream << std::move(value);
        }

        default_presenter_::present(placeholders::footer, stream);

        stream <<   "</div>";
    }

    private:
    const DocumentT& _doc;
};

/**
 * @brief System-page layout type for a rendering context.
 * @tparam ContextT Rendering context type.
 * @ingroup DoxyG_pages
 */
template <typename ContextT>
using sys = udho::view::tmpl::layout::basic_layout<
                ContextT,
                udho::view::tmpl::layout::basic_document<minimal>,
                presenter<l::basic_document<minimal>>
            >;

/**
 * @brief Creates the configured system-page listing layout.
 * @tparam ContextT Rendering context type.
 * @param context Rendering context.
 * @ingroup DoxyG_pages
 */
template <typename ContextT>
sys<ContextT> listing(ContextT context) {
    namespace p = l::placeholders;

    using minimal_presenter = presenter<l::basic_document<minimal>>;

    sys<ContextT> layout = l::create<minimal, ContextT, minimal_presenter>(context);

    layout.preamble().title("Udho System");
    layout.preamble().classes("main");
    layout.properties(places::listing)  .view("lua://udho/listing_page") .classes("files")  ;
    layout.properties(places::headline)                                  .classes("routes") ;
    layout.properties(places::routes)   .view("lua://udho/routes_page")  .classes("routes") ;
    layout.properties(p::header)        .view("lua://udho/header")       .classes("header") ;
    layout.properties(p::footer)        .view("lua://udho/status")       .classes("footer") ;

    return layout;
}

}
}
}
}

#endif // UDHO_PAGES_LAYOUTS_H
