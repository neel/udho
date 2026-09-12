// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_SIMPLE_LAYOUT_H
#define UDHO_EXAMPLES_SIMPLE_LAYOUT_H

#include <udho/view/tmpl/layout/document.h>
#include <udho/view/tmpl/layout/layout.h>
#include <udho/view/tmpl/layout/placeholder.h>

namespace simple {
namespace layouts {

namespace layout = udho::view::tmpl::layout;

namespace segments {
struct masthead {};
struct greeting {};
struct details {};
struct footer {};
}

namespace places {
inline segments::masthead masthead;
inline segments::greeting greeting;
inline segments::details details;
inline segments::footer footer;
}

using placeholders = layout::basic_placeholder<
    layout::spot<segments::masthead>,
    layout::spot<segments::greeting>,
    layout::spot<segments::details>,
    layout::spot<segments::footer>
>;

template <typename ContextT>
using page = layout::basic_layout<
    ContextT,
    layout::basic_document<placeholders>,
    layout::default_presenter<layout::basic_document<placeholders>>
>;

template <typename ContextT>
page<ContextT> hello(ContextT context) {
    auto result = layout::create<placeholders>(context);

    result.preamble()
        .title("Hello from udho")
        .doclang("en");

    result.properties(places::masthead).classes("masthead").id("page-header");
    result.properties(places::greeting).classes("greeting").id("greeting");
    result.properties(places::details).classes("details");
    result.properties(places::footer).classes("footer");

    return result;
}

}
}

#endif // UDHO_EXAMPLES_SIMPLE_LAYOUT_H
