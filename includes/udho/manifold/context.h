#ifndef UDHO_MANIFOLD_CONTEXT_H
#define UDHO_MANIFOLD_CONTEXT_H

#include <udho/manifold/portal.h>
#include <udho/manifold/components/handler.h>
#include <udho/manifold/components/stream.h>

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
struct basic_context {
    using stream_type               = StreamT;
    using ostream_type              = udho::manifold::basic_ostream<stream_type>;
    using portal_type               = udho::manifold::portal<Components...>;
    using basic_context_type        = basic_context<StreamT, Components...>;

    template <typename, typename...>
    friend struct basic_context;

public:
    basic_context(ostream_type& stream, const portal_type& portal, std::size_t id)
        : _ostream(stream), _portal(portal), _flow_id(id)
    {}

    template <typename... OtherComponents>
    basic_context(basic_context<OtherComponents...>& other): _ostream(other._ostream), _portal(other._portal), _flow_id(other._flow_id) { }

    portal_type& portal() { return _portal; }
public:
    template <typename... Args>
    void write(Args&&... args) {
        _ostream.write(std::forward<Args>(args)...);
    }

    udho::net::types::transfer::encoding encoding() const { return _ostream.encoding(); }
    udho::net::types::transfer::compression compression() const { return _ostream.compression(); }
    void encoding(udho::net::types::transfer::encoding enc) { _ostream.encoding(enc); }
    void compression(udho::net::types::transfer::compression cmp) { _ostream.compression(cmp); }

    template <typename T>
    friend basic_context_type& operator<<(basic_context_type& ctx, T&& value) {
        ctx.write(std::forward<T>(value));
        return ctx;
    }

public:
    void disable_buffering() {
        // sync cookies, session, csrf etc..
        _ostream.disable_buffering();
    }

public:
    void finish() {
        // sync cookies, session, csrf etc.. if not synced already
        _ostream.finish();
    }

    ~basic_context() { }

private:
    ostream_type&                       _ostream;
    portal_type                         _portal;
    std::size_t                         _flow_id;
    udho::net::types::headers::response _response;
    boost::beast::multi_buffer          _buffer;
};

}
}

#endif // UDHO_MANIFOLD_CONTEXT_H
