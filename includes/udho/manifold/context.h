#ifndef UDHO_MANIFOLD_CONTEXT_H
#define UDHO_MANIFOLD_CONTEXT_H

#include <udho/manifold/portal.h>
// #include <udho/manifold/components/handler.h>
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

// { generic feature set to component set

// {{ storage
template <typename... Components>
struct internal_component_storage_container{
    template <typename... XComponents>
    using append = internal_component_storage_container<Components..., XComponents...>;
    template <typename... XComponents>
    using prepend = internal_component_storage_container<XComponents..., Components...>;

    template <typename XComponent>
    using has = std::disjunction<std::is_same<XComponent, Components>...>;
};
// }}

// {{ merge

template <typename OutStorageT, typename ComponentStorage>
struct _internal_component_storage_unique;

template <typename OutStorageT>
struct _internal_component_storage_unique<OutStorageT, internal_component_storage_container<>>{
    using type = OutStorageT;
};

template <typename OutStorageT, typename Component, typename... Components>
struct _internal_component_storage_unique<OutStorageT, internal_component_storage_container<Component, Components...>> {
    using current_result_type = std::conditional_t<
        OutStorageT::template has<Component>::value,
        OutStorageT,
        typename OutStorageT::template append<Component>
    >;
    using rest_type = _internal_component_storage_unique<current_result_type, internal_component_storage_container<Components...>>;
    using type = typename rest_type::type;
};

template <typename ComponentStorage>
using internal_component_storage_unique = _internal_component_storage_unique<internal_component_storage_container<>, ComponentStorage>;

// {{ concat
template <typename... Storages>
struct internal_component_storage_concat;

template <>
struct internal_component_storage_concat<> {
    using type = internal_component_storage_container<>;
};

template <typename... C>
struct internal_component_storage_concat<internal_component_storage_container<C...>> {
    using type = internal_component_storage_container<C...>;
};

template <typename... L, typename... R, typename... Rest>
struct internal_component_storage_concat<internal_component_storage_container<L...>, internal_component_storage_container<R...>, Rest... > {
    using type = typename internal_component_storage_concat<internal_component_storage_container<L..., R...>, Rest...>::type;
};

template <typename... ComponentStorage>
struct internal_component_storage_merge{
    using type        = typename internal_component_storage_unique<typename internal_component_storage_concat<ComponentStorage...>::type>::type;
};
// }}

// {{ extract all components providing a single feature
//    use internal_get_all_components_storage_for_feature

template <typename Feature, std::size_t Idx, typename CompositionT, typename Current = typename CompositionT::template component_at<Feature, Idx>>
struct _internal_get_all_components_storage_for_feature;

// terminal: component_at returns void => stop, produce empty container
template <typename Feature, std::size_t Idx, typename CompositionT>
struct _internal_get_all_components_storage_for_feature<Feature, Idx, CompositionT, void> {
    using type = internal_component_storage_container<>;
};

// recursive: Current is a real component => prepend it and continue
template <typename Feature, std::size_t Idx, typename CompositionT, typename Current>
struct _internal_get_all_components_storage_for_feature {
    using tail_type = typename _internal_get_all_components_storage_for_feature<Feature, Idx + 1, CompositionT>::type;

    using type = typename tail_type::template prepend<Current>;
};

template <typename Feature, typename CompositionT>
using internal_get_all_components_storage_for_feature = _internal_get_all_components_storage_for_feature<Feature, 0, CompositionT>;
// }}

template <typename CompositionT>
struct internal_get_components_storage_for_features;

template <typename... Components>
struct internal_get_components_storage_for_features<udho::manifold::composition<Components...>> {
    using composition_type = udho::manifold::composition<Components...>;

    template <typename... Features>
    struct for_features{
        using type = typename internal_component_storage_merge<typename internal_get_all_components_storage_for_feature<Features, composition_type>::type...>::type;
    };
};

// }

}

/**
 * @brief context passed to the user specified handlers that respond to the HTTP requests.
 *
 * `basic_context` combines an output stream, a portal, and the current
 * flow identifier. It provides the write-facing interface used by handlers or
 * component code, while also exposing the portal for controlled access to
 * components, configurations, and journal results.
 *
 * @tparam StreamT Underlying stream type used by `basic_ostream`.
 * @tparam Components Component types exposed through the context portal.
 *
 * @see portal
 * @see udho::net::basic_ostream
 */
template <typename StreamT, typename... Components>
struct basic_context {
    /**
     * @brief Underlying stream type.
     */
    using stream_type               = StreamT;
    /**
     * @brief Output stream wrapper used by the context.
     */
    using ostream_type              = udho::net::basic_ostream<stream_type>;
    /**
     * @brief Portal type exposed by the context.
     */
    using portal_type               = udho::manifold::portal<Components...>;
    using self_type                 = basic_context<StreamT, Components...>;

    template <typename, typename...>
    friend struct basic_context;

    /**
     * @brief Checks whether the context portal exposes a component type.
     *
     * @tparam ComponentQ Component type to query.
     */
    template <typename ComponentQ>
    using has = typename portal_type::template has<ComponentQ>;

public:

    /**
     * @brief Constructs a context from an output stream, portal, and flow id.
     *
     * @param stream Output stream used by `write()`, `operator<<()`, and response
     *        finalization helpers.
     * @param portal Portal exposing component, config, and journal access.
     * @param id Flow identifier associated with this context.
     */
    basic_context(ostream_type& stream, const portal_type& portal, std::size_t id)
        : _ostream(stream), _portal(portal), _flow_id(id)
    {}

    /**
     * @brief Deleted conversion to Boost.Asio executor.
     *
     * This prevents accidental use of a context object as an executor.
     */
    operator boost::asio::executor() const = delete;

    /**
     * @brief construct subset context from another superset context.
     * @pre The other context must have the same or more Components that this context
     * @pre Both source and sink contexts must have the same stream
     * @param other
     */
    template <typename... OtherComponents>
    basic_context(udho::manifold::basic_context<StreamT, OtherComponents...>& other): _ostream(other._ostream), _portal(other._portal), _flow_id(other._flow_id) { }

    /**
     * @brief Gets the mutable portal.
     *
     * @return Reference to the portal exposed by this context.
     */
    portal_type& portal() { return _portal; }

    /**
     * @brief Gets the output stream.
     *
     * @return Reference to the context output stream.
     */
    ostream_type& ostream() { return _ostream; }
public:

    /**
     * @brief Writes values to the context output stream.
     *
     * Arguments are forwarded to `basic_ostream::write()`.
     *
     * @tparam Args Argument types accepted by the output stream.
     * @param args Values to write.
     */
    template <typename... Args>
    void write(Args&&... args) {
        _ostream.write(std::forward<Args>(args)...);
    }


    /**
     * @brief Gets the current transfer encoding mode.
     *
     * @return Transfer encoding configured on the output stream.
     */
    inline udho::net::types::transfer::encoding encoding() const { return _ostream.encoding(); }
    /**
     * @brief Gets the current transfer compression mode.
     *
     * @return Transfer compression configured on the output stream.
     */
    inline udho::net::types::transfer::compression compression() const { return _ostream.compression(); }
    /**
     * @brief Sets the transfer encoding mode.
     *
     * @param enc Transfer encoding to apply to the output stream.
     */
    inline void encoding(udho::net::types::transfer::encoding enc) { _ostream.encoding(enc); }
    /**
     * @brief Sets the transfer compression mode.
     *
     * @param cmp Transfer compression to apply to the output stream.
     */
    inline void compression(udho::net::types::transfer::compression cmp) { _ostream.compression(cmp); }

    /**
     * @brief Streams a value into the context output stream.
     *
     * This is a convenience wrapper around `write()`.
     *
     * @tparam T Value type.
     * @param ctx Context receiving the value.
     * @param value Value to write.
     * @return Reference to `ctx`.
     */
    template <typename T>
    friend self_type& operator<<(self_type& ctx, T&& value) {
        ctx.write(std::forward<T>(value));
        return ctx;
    }

public:

    /**
     * @brief Disables output buffering for the underlying stream.
     *
     * After buffering is disabled, subsequent writes are forwarded according to the
     * stream's immediate-output behavior.
     *
     * @see basic_ostream<StreamT>::disable_buffering()
     */
    inline void disable_buffering() {
        // sync cookies, session, csrf etc..
        _ostream.disable_buffering();
    }

public:
    /**
     * @brief Finalizes the output stream.
     *
     * This flushes or completes the stream according to `basic_ostream::finish()`.
     * It should be called when response generation is complete.
     *
     * @see basic_ostream<StreamT>::finish()
     */
    inline void finish() {
        // sync cookies, session, csrf etc.. if not synced already
        _ostream.finish();
    }

    /**
     * @brief Captures an already propagated exception in the output stream.
     *
     * @param captured Captured exception object to move into the stream.
     *
     * @see basic_ostream<StreamT>::exception(udho::exceptions::captured&&)
     */
    inline void capture(udho::exceptions::captured&& captured) {
        _ostream.exception(std::move(captured));
    }
    /**
     * @brief Captures the current exception in the output stream.
     */
    inline void capture() {
        capture(udho::exceptions::captured::propagate());
    }

public:
    /**
     * @brief Provides view-data metadata for a context.
     *
     * Exposes the context, portal metadata, flow id, routes, and resources to the
     * view-data reflection system.
     *
     * @param type Tag identifying `self_type`.
     * @return View-data metadata association for `self_type`.
     */
    friend auto metatype(udho::view::data::type<self_type>){
        using namespace udho::view::data;

        return assoc("context"),
               metatype(udho::view::data::type<portal_type>()),
               cvar("flow_id",     &self_type::_flow_id),
               fvar("routes",      &self_type::_url_routes),
               fvar("resources",   &self_type::_resources_store)
        ;
    }

    /**
     * @brief Gets the flow identifier associated with this context.
     *
     * @return Flow id.
     */
    std::size_t flow_id() const { return _flow_id; }

private:
    const auto& _url_routes() const {
        return _portal.routes();
    }

    const auto& _resources_store() const {
        return _portal.resources();
    }

private:
    ostream_type&                       _ostream;
    portal_type                         _portal;
    std::size_t                         _flow_id;
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
