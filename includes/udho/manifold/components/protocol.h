#ifndef UDHO_MANIFOLD_COMPONENTS_PROTOCOL_H
#define UDHO_MANIFOLD_COMPONENTS_PROTOCOL_H

#include <udho/manifold/features.h>
#include <udho/utils/string_view.h>
#include <udho/net/common.h>
#include <udho/net/protocols/protocols.h>
#include <udho/manifold/config.h>
#include <boost/beast/core/buffers_to_string.hpp>

namespace udho{
namespace manifold{

namespace components{

template <typename ProtocolT, typename StreamT = udho::net::types::socket>
struct protocol{
    using reader_type       = typename ProtocolT::reader;
    using writer_type       = typename ProtocolT::writer;
    using reader_ptr_type   = std::shared_ptr<reader_type>;
    using stream_type       = StreamT;
    using readers_collection_type = std::unordered_map<std::size_t, reader_ptr_type>;

    using features    = udho::manifold::features<
        udho::manifold::feature::header_reader,
        udho::manifold::feature::body_reader// ,
        // udho::manifold::feature::header_writer,
        // udho::manifold::feature::body_writer
    >;

    UDHO_CONFIG_PARAM(header_time_limit,      std::size_t,  1);      // maximum time spent (in seconds) for parsing only the header part of an HTTP request
    UDHO_CONFIG_PARAM(header_memory_limit,    std::size_t,  1024);   // maximum number of bytes that can be used for parsing only the header part of an HTTP request
    UDHO_CONFIG_PARAM(body_time_limit,        std::size_t,  10);     // maximum time spent (in seconds) for reading the body part of an HTTP request
    UDHO_CONFIG_PARAM(body_memory_limit,      std::size_t,  4096);   // maximum number of bytes that can be used for reading the body part of an HTTP request
    UDHO_CONFIG_PARAM(contiguous_buffer,      bool,         true);   // use flat_buffer if contiguous_buffer is true, otherwise use multi_buffer

    using params      = udho::manifold::params<
        header_time_limit,
        header_memory_limit,
        body_time_limit,
        body_memory_limit,
        contiguous_buffer
    >;

    static constexpr const udho::utils::string_view name = "protocol";

public:
    reader_ptr_type& reader(std::size_t id, stream_type& stream){
        std::scoped_lock<std::mutex> lock(_mutex);
        auto it = _readers.find(id);
        if(it != _readers.end()) {
            return it->second;
        } else {
            reader_ptr_type reader{new reader_type{stream}};
            auto res = _readers.emplace(id, reader);
            assert(res.second);
            return res.first->second;
        }
    }

    reader_ptr_type& reader(std::size_t id){
        std::scoped_lock<std::mutex> lock(_mutex);
        auto it = _readers.find(id);
        if(it == _readers.end()) {
            throw std::runtime_error{udho::utils::format("reader doesn't exist for the provided id {}", id)};
        }

        return it->second;
    }

    bool remove(std::size_t id) {
        std::scoped_lock<std::mutex> lock(_mutex);
        auto it = _readers.find(id);
        if (it == _readers.end())
            return false;

        _readers.erase(it);
        return true;
    }

private:
    readers_collection_type _readers;
    std::mutex              _mutex;
};

namespace protocols {
    template <typename StreamT>
    using http2 = udho::manifold::components::protocol<udho::net::protocols::http2<StreamT>, StreamT>;
}

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

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config), _id(id) {}

    template <typename... Components, typename NextT>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        using result = udho::manifold::feature::header_reader::result;

        std::size_t timeout_secs = _config[component_type::header_time_limit::val].value();

        reader_ptr_type reader = _component.reader(_id, stream);
        reader->start([this, next{std::move(next)}](request_type&& request, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            if(!ec) {
                next.pass(result{std::move(request)});
            } else {
                std::cout << "header_reader facet: " << ec.message() << std::endl;
                next.fail(std::system_error{ec});
            }
        }, timeout_secs);
    }

    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        std::cout << "-> facet<components::protocol<ProtocolT, StreamT>, udho::manifold::feature::header_reader>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }
private:
    component_type&     _component;
    const config_type&  _config;
    std::size_t         _id;
private:
    struct http_reader{};
};

template <typename ProtocolT, typename StreamT>
struct facet<components::protocol<ProtocolT, StreamT>, udho::manifold::feature::body_reader>{
    using component_type  = components::protocol<ProtocolT, StreamT>;
    using reader_type     = typename component_type::reader_type;
    using writer_type     = typename component_type::writer_type;
    using stream_type     = typename component_type::stream_type;
    using reader_ptr_type = std::shared_ptr<reader_type>;
    using config_type     = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config), _id(id) {}

    template <typename... Components, typename NextT>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        const udho::manifold::feature::header_reader::result& request = journal.template at<udho::manifold::feature::header_reader>();
        const udho::manifold::feature::identifier::result& identifier = journal.template at<udho::manifold::feature::identifier>();

        using result_type = udho::manifold::feature::body_reader::result;

        std::size_t timeout_secs   = _config[component_type::body_time_limit::val].value();     // Mitigate CWE-400 w.r.t. time consumed (slowloris attack)
        std::size_t memort_limit   = _config[component_type::body_memory_limit::val].value();   // Mitigate CWE-400, CWE-770; read until eof not allowed unless eof comes before memort_limit exhausts
        bool use_contiguous_buffer = _config[component_type::contiguous_buffer::val].value();   // overridable by user

        // struct reader_raii{
        //     reader_raii(component_type& comp, std::size_t id): _component(comp), _id(id) {}
        //     ~reader_raii(){
        //         _component.remove(_id);
        //     }
        // private:
        //     component_type& _component;
        //     std::size_t     _id;
        // };

        std::string content_type = request.count(boost::beast::http::field::content_type)
                                       ? request.at(boost::beast::http::field::content_type)
                                       : "application/octet-stream";

        // For now don't use content type to decide flat or multi buffer.

        reader_ptr_type reader = _component.reader(_id);

        if(use_contiguous_buffer) {
            reader->upload_to_flat_buffer(request, [this, next{std::move(next)}, &content_type, use_contiguous_buffer](boost::beast::flat_buffer&& buffer, std::error_code ec, std::size_t bytes_transferred) mutable {
                // reader_raii gaurd(_component, _id);
                std::cout << boost::beast::buffers_to_string(buffer.data()) << std::endl;
                udho::manifold::feature::body_reader::result result(content_type, use_contiguous_buffer, std::move(buffer), boost::beast::multi_buffer{});
                next(std::move(result), !ec); // The operator() overload on next forwards that call to pass or fail depending on !ec
            }, timeout_secs, memort_limit);
        } else {
            reader->upload_to_multi_buffer(request, [this, next{std::move(next)}, &content_type, use_contiguous_buffer](boost::beast::multi_buffer&& buffer, std::error_code ec, std::size_t bytes_transferred) mutable {
                // reader_raii gaurd(_component, _id);
                std::cout << boost::beast::buffers_to_string(buffer.data()) << std::endl;
                udho::manifold::feature::body_reader::result result(content_type, use_contiguous_buffer, boost::beast::flat_buffer{}, std::move(buffer));
                next(std::move(result), !ec); // The operator() overload on next forwards that call to pass or fail depending on !ec
            }, timeout_secs, memort_limit);
        }
    }


    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        std::cout << "-> facet<components::protocol<ProtocolT, StreamT>, udho::manifold::feature::body_reader>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }
private:
    component_type&     _component;
    const config_type&  _config;
    std::size_t         _id;
};

}
}

#endif // UDHO_MANIFOLD_COMPONENTS_PROTOCOL_H
