#ifndef UDHO_MANIFOLD_COMPONENTS_PROTOCOL_H
#define UDHO_MANIFOLD_COMPONENTS_PROTOCOL_H

#include <udho/manifold/features.h>
#include <udho/utils/string_view.h>
#include <udho/net/common.h>
#include <udho/net/protocols/protocols.h>
#include <udho/manifold/config.h>

namespace udho{
namespace manifold{

namespace components{

template <typename ProtocolT, typename StreamT = udho::net::types::socket>
struct protocol{
    using reader_type = typename ProtocolT::reader;
    using writer_type = typename ProtocolT::writer;
    using stream_type = StreamT;

    using features    = udho::manifold::features<
        udho::manifold::feature::header_reader,
        udho::manifold::feature::body_reader,
        udho::manifold::feature::header_writer,
        udho::manifold::feature::body_writer
    >;

    UDHO_CONFIG_PARAM(header_time_limit,      std::size_t,  1);    // maximum time spent (in seconds) for parsing only the header part of an HTTP request
    UDHO_CONFIG_PARAM(headeer_memory_limit,   std::size_t,  1024); // maximum number of bytes that can be used for parsing only the header part of an HTTP request
    UDHO_CONFIG_PARAM(body_time_limit,        std::size_t,  10);   // maximum time spent (in seconds) for reading the body part of an HTTP request
    UDHO_CONFIG_PARAM(body_memory_limit,      std::size_t,  4096); // maximum number of bytes that can be used for reading the body part of an HTTP request

    using params      = udho::manifold::params<
        header_time_limit,
        headeer_memory_limit,
        body_time_limit,
        body_memory_limit
    >;

    static constexpr const udho::utils::string_view name = "protocol";
};

}

template <typename ProtocolT, typename StreamT>
struct facet<components::protocol<ProtocolT, StreamT>, udho::manifold::feature::header_reader>{
    using component_type  = components::protocol<ProtocolT, StreamT>;
    using reader_type     = typename component_type::reader_type;
    using writer_type     = typename component_type::writer_type;
    using stream_type     = typename component_type::stream_type;
    using request_type    = udho::net::types::headers::request;
    using reader_ptr_type = std::shared_ptr<reader_type>;
    using config_type     = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config): _component(component), _config(config) {}

    template <typename... Components, typename NextT>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        using result = udho::manifold::feature::header_reader::result;

        reader_ptr_type reader{new reader_type{stream}};
        reader->start([this, next{std::move(next)}](request_type&& request, std::error_code ec, std::size_t bytes_transferred) mutable {
            if(!ec) {
                next.pass(result{std::move(request)});
            } else {
                next.fail(std::system_error{ec});
            }
        });
    }

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        eval(journal, std::forward<NextT>(next), stream);
    }
private:
    component_type& _component;
    const config_type& _config;
private:
    struct http_reader{};
};

}
}

#endif // UDHO_MANIFOLD_COMPONENTS_PROTOCOL_H
