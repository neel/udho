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
#include <udho/pages/data.h>

namespace udho{
namespace pages{
namespace system{
namespace layouts{

namespace l = udho::view::tmpl::layout;

namespace places {
namespace segments {
    struct listing{};
}
}


namespace placeholders = l::placeholders;

using minimal = l::basic_placeholder<
    l::spot<placeholders::segments::header>,
    l::spot<places::segments::listing>,
    l::spot<placeholders::segments::footer>
>;

namespace places{
    static segments::listing listing;
}

template <class DocumentT>
struct presenter: l::default_presenter<DocumentT, presenter<DocumentT>>{
    using default_presenter_ = l::default_presenter<DocumentT, presenter<DocumentT>>;
    using basic_presenter_   = typename default_presenter_::basic_presenter_;

    presenter(const DocumentT& doc): default_presenter_(doc), _doc(doc) {}

    using default_presenter_::operator();

    template <typename Stream>
    void render(Stream& stream) const {
        namespace p = l::placeholders;

        bool tab_active = false;

        stream << "<body>";
        stream <<   "<div class='system'>";
        if(_doc[p::header].exists())
            stream << *_doc[p::header];

        if(_doc[places::listing].exists())
        stream << *_doc[places::listing];

        if(_doc[placeholders::footer].exists())
            default_presenter_::present(placeholders::footer, *_doc[placeholders::footer], stream);
        stream <<   "</div>";

        stream <<     R"SCRIPT(<script>
            document.addEventListener('DOMContentLoaded', () => {
                // Add JS-enabled class to body
                document.body.classList.add('js-enabled');

                // Initialize tab system for each .system container
                document.querySelectorAll('.system').forEach(system => {
                    const tabContainer = system.querySelector('.tab-container');
                    const buttons = tabContainer.querySelectorAll('.tab-btn');
                    const contents = tabContainer.querySelectorAll('.tab-content');
                    const headings = tabContainer.querySelectorAll('.listing-heading');

                    // Hide headings and show buttons
                    headings.forEach(heading => heading.style.display = 'none');
                    tabContainer.querySelector('.tab-buttons').style.display = 'flex';

                    // Set initial active state
                    const firstContent = contents[0];
                    const firstButton = buttons[0];

                    contents.forEach(content => content.classList.remove('active-content'));
                    buttons.forEach(button => button.classList.remove('active-tab'));

                    if (firstContent) firstContent.classList.add('active-content');
                    if (firstButton) firstButton.classList.add('active-tab');

                    // Add click handlers
                    tabContainer.querySelector('.tab-buttons').addEventListener('click', (e) => {
                        if (!e.target.classList.contains('tab-btn')) return;

                        const targetId = e.target.dataset.target;
                        const targetContent = tabContainer.querySelector(`#${targetId}`);

                        // Update buttons
                        buttons.forEach(button => button.classList.remove('active-tab'));
                        e.target.classList.add('active-tab');

                        // Update contents
                        contents.forEach(content => content.classList.remove('active-content'));
                        if (targetContent) targetContent.classList.add('active-content');
                    });
                });
            });
        </script>)SCRIPT";
        stream << "</body>";
    }

    private:
    const DocumentT& _doc;
};

template <typename ContextT>
using sys = udho::view::tmpl::layout::basic_layout<
                ContextT,
                udho::view::tmpl::layout::basic_document<minimal>,
                presenter<l::basic_document<minimal>>
            >;

template <typename ContextT>
sys<ContextT> listing(ContextT context) {
    namespace p = l::placeholders;

    using minimal_presenter = presenter<l::basic_document<minimal>>;

    sys<ContextT> layout = l::create<minimal, ContextT, minimal_presenter>(context);

    layout.preamble().title("Udho System");
    layout.preamble().classes("main");
    layout.properties(places::listing).classes("files").view("lua://udho/listing_page");
    layout.properties(p::header).classes("header") .view("lua://udho/header");
    layout.properties(p::footer).classes("footer") .view("lua://udho/status");

    return layout;
}

}
}
}
}

#endif // UDHO_PAGES_LAYOUTS_H

