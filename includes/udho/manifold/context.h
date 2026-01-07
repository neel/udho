#ifndef UDHO_MANIFOLD_CONTEXT_H
#define UDHO_MANIFOLD_CONTEXT_H

#include <udho/manifold/portal.h>

namespace udho{
namespace manifold{

template <typename... Components>
struct context: public udho::net::stream, public udho::manifold::portal<Components...>{

};

}
}

#endif // UDHO_MANIFOLD_CONTEXT_H
