#ifndef UDHO_MANIFOLD_CONTEXT_H
#define UDHO_MANIFOLD_CONTEXT_H

#include <udho/manifold/portal.h>
#include <udho/manifold/components/handler.h>

namespace udho{
namespace manifold{

template <typename StreamT, typename... Components>
struct basic_context;

namespace detail{

template <typename StreamT, typename PortalT>
struct get_context_for_portal;

template <typename StreamT, typename... Components>
struct get_context_for_portal<StreamT, udho::manifold::portal<Components...>>{
    using type = basic_context<StreamT, Components...>;
};

}

template <typename StreamT, typename... Components>
struct basic_context{
    using stream_type               = StreamT;
    using portal_type               = udho::manifold::portal<Components...>;

    template <typename, typename...>
    friend struct basic_context;

public:
    basic_context(stream_type& stream, const portal_type& portal, std::size_t id): _stream(stream), _portal(portal), _flow_id(id), _output_buffering(false) {}

    template <typename... OtherComponents>
    basic_context(basic_context<OtherComponents...>& other): _stream(other._stream), _portal(other._portal), _flow_id(other._flow_id), _output_buffering(other._output_buffering) { }

    portal_type& portal() { return _portal; }

    bool output_buffering() const { return _output_buffering; }
    void output_buffering(bool flag) { _output_buffering = flag; }

public:
    void finish() {
        udho::manifold::components::handler& handler = _portal._composition_view.template get<udho::manifold::components::handler>().component();
        handler.invoke(_flow_id);
    }

    ~basic_context() { }
public:

private:
    stream_type&                        _stream;
    portal_type                         _portal;
    std::size_t                         _flow_id;
    udho::net::types::headers::response _response;
    boost::beast::multi_buffer          _buffer;
    bool                                _output_buffering;
};

}
}

#endif // UDHO_MANIFOLD_CONTEXT_H
