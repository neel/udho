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

namespace places {
namespace segments {
    struct files{};
    struct assets{};
}
}


namespace placeholders = l::placeholders;

using minimal = l::basic_placeholder<
    l::spot<placeholders::segments::header>,
    l::multispot<places::segments::files>,
    l::spot<places::segments::assets>,
    l::spot<placeholders::segments::footer>
>;

namespace places{
    static segments::files files;
    static segments::assets assets;
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
            basic_presenter_::present(p::header, *_doc[p::header], stream);

        stream <<       "<div class='tab-container'>";
        stream <<           "<div class='tab-buttons'>";
        stream <<           "</div>";

        bool content_active = false;
        for(auto i = 0; i < _doc[places::files].count(); ++i){
            std::string tab_content = !content_active ? "tab-content" : "tab-content active-content";
            content_active = true;

            stream << "<div class='"+ tab_content +"'>";
            stream << _doc[places::files][i];
            stream << "</div>";
        }

        if(_doc[places::assets].exists()){
            std::string tab_content = !content_active ? "tab-content" : "tab-content active-content";
            content_active = true;

            stream << "<div class='"+ tab_content +"'>";
            stream << *_doc[places::assets];
            stream << "</div>";
        }

        stream <<       "</div>";
        stream <<   "</div>";
        stream <<     R"SCRIPT(<script>
                        document.addEventListener('DOMContentLoaded', () => {
                            const tabButtonsContainer = document.querySelector('.tab-buttons');
                            const tabContents = document.querySelectorAll('.tab-content');

                            // Generate buttons based on existing content
                            tabContents.forEach(contentDiv => {
                                const listingContainer = contentDiv.querySelector('.listing-container:first-child');
                                if (!listingContainer || !listingContainer.id) return;

                                const button = document.createElement('button');
                                button.className = 'tab-btn';
                                button.textContent = listingContainer.id.charAt(0).toUpperCase() + listingContainer.id.slice(1); // Capitalize first letter
                                button.setAttribute('data-target', listingContainer.id);

                                // Set initial active tab
                                if (contentDiv.classList.contains('active-content')) {
                                    button.classList.add('active-tab');
                                }

                                tabButtonsContainer.appendChild(button);
                            });

                            // Add click handlers
                            tabButtonsContainer.addEventListener('click', (e) => {
                                if (!e.target.classList.contains('tab-btn')) return;

                                // Remove active classes
                                document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active-tab'));
                                document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active-content'));

                                // Set new active
                                const targetId = e.target.dataset.target;
                                e.target.classList.add('active-tab');
                                const targetContent = document.getElementById(targetId).closest('.tab-content');
                                if (targetContent) {
                                    targetContent.classList.add('active-content');
                                }
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
    layout.properties(places::files).classes("files").view("lua://udho/listing");
    layout.properties(places::assets).classes("assets").view("lua://udho/assets");
    layout.properties(p::header).classes("header") .view("lua://udho/header");
    layout.properties(p::footer).classes("footer") .view("lua://udho/status");

    return layout;
}

}
}
}
}

#endif // UDHO_PAGES_LAYOUTS_H

