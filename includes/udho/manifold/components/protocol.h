#ifndef UDHO_MANIFOLD_COMPONENTS_PROTOCOL_H
#define UDHO_MANIFOLD_COMPONENTS_PROTOCOL_H

#include <udho/manifold/features.h>
#include <udho/utils/string_view.h>
#include <udho/net/common.h>
#include <udho/net/protocols/protocols.h>
#include <udho/manifold/config.h>
#include <udho/manifold/portal.h>
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
        udho::manifold::feature::body_reader
    >;

    UDHO_CONFIG_PARAM(header_time_limit,      std::size_t,  1);      // maximum time spent (in seconds) for parsing only the header part of an HTTP request
    UDHO_CONFIG_PARAM(header_memory_limit,    std::size_t,  1024);   // maximum number of bytes that can be used for parsing only the header part of an HTTP request
    UDHO_CONFIG_PARAM(body_time_limit,        std::size_t,  10);     // maximum time spent (in seconds) for reading the body part of an HTTP request
    UDHO_CONFIG_PARAM(body_memory_limit,      std::size_t,  4096);   // maximum number of bytes allowed for the HTTP request body
    UDHO_CONFIG_PARAM(field_memory_limit,     std::size_t,  1024);   // maximum number of bytes allowed for a form field HTTP in the request body
    UDHO_CONFIG_PARAM(contiguous_buffer,      bool,         true);   // use flat_buffer if contiguous_buffer is true, otherwise use multi_buffer

    using params      = udho::manifold::params<
        header_time_limit,
        header_memory_limit,
        body_time_limit,
        body_memory_limit,
        field_memory_limit,
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
    using http = udho::manifold::components::protocol<udho::net::protocols::http<StreamT>, StreamT>;
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

template <typename ProtocolT, typename StreamT, typename JournalT>
struct accessor<components::protocol<ProtocolT, StreamT>, JournalT>: basic_accessor<components::protocol<ProtocolT, StreamT>, JournalT>{
    using basic_accessor_type   = basic_accessor<components::protocol<ProtocolT, StreamT>, JournalT>;
    using component_type        = components::protocol<ProtocolT, StreamT>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;
    using request_type          = udho::net::types::headers::request;

    using basic_accessor_type::basic_accessor_type;

    const request_type& request() const {
        return basic_accessor_type::journal().template at<udho::manifold::feature::header_reader>();
    }

    const udho::manifold::feature::body_reader::result& body() const {
        return basic_accessor_type::journal().template at<udho::manifold::feature::body_reader>();
    }
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

        if(request.method() == boost::beast::http::verb::get) {
            next.skip();
            return;
        } else {
            using result_type = udho::manifold::feature::body_reader::result;

            std::size_t timeout_secs   = _config[component_type::body_time_limit::val].value();     // Mitigate CWE-400 w.r.t. time consumed (slowloris attack)
            std::size_t memory_limit   = _config[component_type::body_memory_limit::val].value();   // Mitigate CWE-400, CWE-770; read until eof not allowed unless eof comes before memort_limit exhausts
            std::size_t field_limit    = _config[component_type::field_memory_limit::val].value();
            bool use_contiguous_buffer = _config[component_type::contiguous_buffer::val].value();   // overridable by user

            std::string content_type = request.count(boost::beast::http::field::content_type)
                                           ? request.at(boost::beast::http::field::content_type)
                                           : "application/octet-stream";

            reader_ptr_type reader = _component.reader(_id);

            udho::net::detail::body_parser_config config;
            config
                .total_content_limit(memory_limit)
                .field_content_limit(field_limit)
                .total_timeout(std::chrono::seconds(timeout_secs));

            if(use_contiguous_buffer) {
                reader->upload_to_flat_buffer(request, [this, next{std::move(next)}, &content_type, use_contiguous_buffer](udho::net::protocols::body_reader_result<boost::beast::flat_buffer>&& bresult, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                    udho::manifold::feature::body_reader::result result(content_type, std::move(bresult), ec, bytes_transferred);
                    next(std::move(result), !ec); // The operator() overload on next forwards that call to pass or fail depending on !ec
                }, config);
            } else {
                reader->upload_to_multi_buffer(request, [this, next{std::move(next)}, &content_type, use_contiguous_buffer](udho::net::protocols::body_reader_result<boost::beast::multi_buffer>&& bresult, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                    udho::manifold::feature::body_reader::result result(content_type, std::move(bresult), ec, bytes_transferred);
                    next(std::move(result), !ec); // The operator() overload on next forwards that call to pass or fail depending on !ec
                }, config);
            }
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
