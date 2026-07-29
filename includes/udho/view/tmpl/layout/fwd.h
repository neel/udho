#ifndef UDHO_VIEW_TMPL_LAYOUT_FWD_H
#define UDHO_VIEW_TMPL_LAYOUT_FWD_H

namespace udho{
namespace view{
namespace tmpl{
namespace layout{

struct meta_tags;

struct document_preamble;

template <typename PlaceholderT>
struct basic_document;

template <typename ContextT, typename DocumentT, typename PresenterT>
struct basic_layout;

template <typename DocumentT, typename PresenterT>
struct basic_layout_impl;

namespace proxy{

template <typename ContainerT>
struct content;

template <typename ContainerT>
struct const_content;

template <typename DocumentT>
struct basic_presenter;

}

}
}
}
}

#endif // UDHO_VIEW_TMPL_LAYOUT_FWD_H
