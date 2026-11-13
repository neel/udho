// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_EXAMPLES_PLANETS_LAYOUT_H
#define UDHO_EXAMPLES_PLANETS_LAYOUT_H

#include <udho/view/tmpl/layout/layout.h>

namespace planets {
namespace layouts {

namespace layout = udho::view::tmpl::layout;

namespace segments {
struct central {};
}

namespace places {
inline segments::central central;
}

using placeholders = layout::basic_placeholder<layout::spot<segments::central>>;
using document_type = layout::basic_document<placeholders>;

// The presenter owns the page structure; actions only populate the central view.
struct presenter: layout::default_presenter<document_type, presenter> {
    using base_type = layout::default_presenter<document_type, presenter>;

    explicit presenter(const document_type& doc): base_type(doc) {}

    template <typename Stream>
    void render(Stream& stream) const {
        stream << "<header class=\"header\"><h1>Planets with a custom layout</h1></header>"
               << "<main class=\"content\">";
        this->present(places::central, stream);
        stream << "</main>"
               << "<footer class=\"footer\"><small>Planets example made with a custom Udho layout</small></footer>";
    }
};

}
}

#endif // UDHO_EXAMPLES_PLANETS_LAYOUT_H
