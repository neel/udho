#ifndef UDHO_VIEW_TRACKING_CONTEXT_H
#define UDHO_VIEW_TRACKING_CONTEXT_H

#include <list>
#include <string>
#include <udho/manifold/context.h>

namespace udho{
namespace view{

/**
 * @brief Borrows a context and the list of views executed during a render.
 *
 * Copies preserve both references. The context and list must outlive the
 * synchronous rendering call, including all nested view calls.
 */
template <typename... Components>
class tracking_context{
public:
    using context_type = udho::manifold::basic_context<Components...>;
    using portal_type = typename context_type::portal_type;
    using views_type = std::list<std::string>;

    tracking_context(context_type& context, views_type& views) noexcept
        : _context(context), _views(views) {}

    tracking_context(const tracking_context&) = default;

    // Match basic_context's protection against Asio's generic executor
    // conversion when the wrapper is passed through overloaded call sites.
    operator boost::asio::executor() const = delete;

    context_type& context() noexcept { return _context; }
    const context_type& context() const noexcept { return _context; }
    views_type& views() noexcept { return _views; }
    const views_type& views() const noexcept { return _views; }

    portal_type& portal() { return context().portal(); }
    const portal_type& portal() const { return context().portal(); }

private:
    context_type& _context;
    views_type& _views;
};

}
}

#endif // UDHO_VIEW_TRACKING_CONTEXT_H
