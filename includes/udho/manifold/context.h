#ifndef UDHO_MANIFOLD_CONTEXT_H
#define UDHO_MANIFOLD_CONTEXT_H

#include <udho/manifold/portal.h>
#include <udho/manifold/components/handler.h>
#include <udho/net/ostream.h>
#include <udho/view/data/data.h>
#include <boost/asio/is_executor.hpp>

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

template <typename StreamT, typename PortalT>
struct get_context_for_composition;

template <typename StreamT, typename... Components>
struct get_context_for_composition<StreamT, udho::manifold::composition<Components...>>{
    using type = basic_context<StreamT, Components...>;
};

}

template <typename StreamT, typename... Components>
struct basic_context {
    using stream_type               = StreamT;
    using ostream_type              = udho::net::basic_ostream<stream_type>;
    using portal_type               = udho::manifold::portal<Components...>;
    using self_type                 = basic_context<StreamT, Components...>;

    template <typename, typename...>
    friend struct basic_context;

    template <typename ComponentQ>
    using has = typename portal_type::template has<ComponentQ>;

public:
    basic_context(ostream_type& stream, const portal_type& portal, std::size_t id)
        : _ostream(stream), _portal(portal), _flow_id(id)
    {}

    operator boost::asio::executor() const = delete;

    /**
     * @brief construct subset context from another superset context.
     * @pre The other context must have the same or more Components that this context
     * @pre Both source and sink contexts must have the same stream
     * @param other
     */
    template <typename... OtherComponents>
    basic_context(udho::manifold::basic_context<StreamT, OtherComponents...>& other): _ostream(other._ostream), _portal(other._portal), _flow_id(other._flow_id) { }

    portal_type& portal() { return _portal; }
    ostream_type& ostream() { return _ostream; }
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
    friend self_type& operator<<(self_type& ctx, T&& value) {
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

public:
    friend auto metatype(udho::view::data::type<self_type>){
        using namespace udho::view::data;

        return assoc("context"),
               cvar("flow_id",     &self_type::_flow_id),
               fvar("routes",      &self_type::routes),
               fvar("resources",   &self_type::resources)
        ;
    }

    std::size_t flow_id() const { return _flow_id; }

private:
    const auto& routes() const {
        return _portal.routes();
    }

    const auto& resources() const {
        return _portal.resources();
    }

private:
    ostream_type&                       _ostream;
    portal_type                         _portal;
    std::size_t                         _flow_id;
    udho::net::types::headers::response _response;
    boost::beast::multi_buffer          _buffer;
};

}
}

// namespace boost::asio {
//     template <typename StreamT, typename... Components>
//     struct is_executor<udho::manifold::basic_context<StreamT, Components...>> : std::false_type {};
// }

// namespace boost::asio::execution {
//     template <typename StreamT, typename... Components>
//     struct is_executor<udho::manifold::basic_context<StreamT, Components...>> : std::false_type {};
// }

#endif // UDHO_MANIFOLD_CONTEXT_H
