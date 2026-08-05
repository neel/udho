#ifndef UDHO_WWW_COMPONENTS_PROTOCOL_H
#define UDHO_WWW_COMPONENTS_PROTOCOL_H

#include <udho/www/features.h>
#include <udho/utils/string_view.h>
#include <udho/net/common.h>
#include <udho/net/protocols/protocols.h>
#include <udho/manifold/config.h>
#include <udho/manifold/portal.h>
#include <boost/beast/core/buffers_to_string.hpp>
#include <udho/www/components/params.h>
#include <udho/logging/macros.h>

/** @addtogroup DoxyG_www_components
 *  @{
 */

namespace udho{
namespace www{

namespace components{

/**
 * @brief Component managing protocol readers indexed by flow id.
 * @tparam ProtocolT Protocol type providing reader and writer types.
 * @tparam StreamT Stream type used by the protocol.
 * @ingroup DoxyG_www_components
 */
template <typename ProtocolT, typename StreamT = udho::net::types::socket>
struct protocol{
    using reader_type       = typename ProtocolT::reader;
    using writer_type       = typename ProtocolT::writer;
    using reader_ptr_type   = std::shared_ptr<reader_type>;
    using stream_type       = StreamT;
    using readers_collection_type = std::unordered_map<std::size_t, reader_ptr_type>;

    using features    = udho::manifold::features<
        udho::www::feature::header_reader,
        udho::www::feature::body_reader
    >;

    using params      = udho::manifold::params<
        udho::www::params::protocol::header_time_limit,
        udho::www::params::protocol::header_memory_limit,
        udho::www::params::protocol::body_time_limit,
        udho::www::params::protocol::body_memory_limit,
        udho::www::params::protocol::field_memory_limit,
        udho::www::params::protocol::contiguous_buffer
    >;

    static constexpr const udho::utils::string_view name = "protocol";

public:
    /**
     * @brief Returns the existing reader for a flow or creates one for the stream.
     * @param id Flow identifier.
     * @param stream Stream used when a reader is created.
     */
    reader_ptr_type& reader(std::size_t id, stream_type& stream){
        std::scoped_lock<std::mutex> lock(_mutex);

        namespace p = udho::logging::params;
        auto it = _readers.find(id);
        if(it != _readers.end()) {
            UDHO_LOG_DEBUG("udho::www::components::protocol", "reader reused", p::flow_id(id), p::socket_id(udho::utils::misc::native_handle(stream)));

            return it->second;
        } else {
            reader_ptr_type reader{new reader_type{stream}};
            auto res = _readers.emplace(id, reader);
            assert(res.second);

            UDHO_LOG_DEBUG("udho::www::components::protocol", "reader created", p::flow_id(id), p::socket_id(udho::utils::misc::native_handle(stream)));

            return res.first->second;
        }
    }

    /**
     * @brief Returns the reader registered for a flow id.
     * @param id Flow identifier.
     */
    reader_ptr_type& reader(std::size_t id){
        std::scoped_lock<std::mutex> lock(_mutex);
        auto it = _readers.find(id);
        if(it == _readers.end()) {
            throw std::runtime_error{udho::utils::format("reader doesn't exist for the provided id {}", id)};
        }

        return it->second;
    }

    /**
     * @brief Removes a registered reader and terminates its stream.
     * @param id Flow identifier.
     */
    bool remove(std::size_t id) {
        std::scoped_lock<std::mutex> lock(_mutex);
        auto it = _readers.find(id);
        if (it == _readers.end())
            return false;

        namespace p = udho::logging::params;
        UDHO_LOG_DEBUG("udho::www::components::protocol", "reader removed", p::flow_id(id), p::request_id("req"), p::socket_id(udho::utils::misc::native_handle(it->second->stream())));

        udho::utils::misc::detail::terminate_stream(it->second->stream());

        _readers.erase(it);
        return true;
    }

    ~protocol() {
        namespace p = udho::logging::params;

        if(_readers.size() > 0) {
            UDHO_LOG_WARNING("udho::www::components::protocol", "component deallocating with active readers", p::request_id("req"));
        }
    }

private:
    readers_collection_type _readers;
    std::mutex              _mutex;
};

namespace protocols {
    /**
     * @brief HTTP protocol component for a stream type.
     * @tparam StreamT Stream type.
     */
    template <typename StreamT>
    using http = udho::www::components::protocol<udho::net::protocols::http<StreamT>, StreamT>;
}

} // components
} // www

namespace manifold{

/**
 * @brief Reads a request header through the protocol reader.
 * @tparam ProtocolT Protocol type.
 * @tparam StreamT Stream type.
 * @ingroup DoxyG_www_components
 */
template <typename ProtocolT, typename StreamT>
struct facet<udho::www::components::protocol<ProtocolT, StreamT>, udho::www::feature::header_reader>{
    using component_type  = udho::www::components::protocol<ProtocolT, StreamT>;
    using reader_type     = typename component_type::reader_type;
    using writer_type     = typename component_type::writer_type;
    using stream_type     = typename component_type::stream_type;
    using request_type    = udho::net::types::headers::request;
    using reader_ptr_type = std::shared_ptr<reader_type>;
    using config_type     = udho::manifold::config<component_type>;

    /**
     * @brief Constructs the facet.
     * @param component Protocol component.
     * @param config Component configuration.
     * @param id Flow identifier.
     */
    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config), _id(id) {}

    /**
     * @brief Starts asynchronous header reading.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        using result = udho::www::feature::header_reader::result;

        std::size_t timeout_secs = _config[udho::www::params::protocol::header_time_limit::val].value();

        reader_ptr_type reader = _component.reader(_id, stream);
        reader->start([this, next{std::move(next)}, reader](request_type&& request, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
            if(!ec || ec == boost::asio::error::eof) {
                next.pass(result{std::move(request)});
            } else {
                // std::cout << "header_reader facet: " << ec.message() << std::endl;

                namespace p = udho::logging::params;
                if (ec == boost::asio::error::operation_aborted) {
                    UDHO_LOG_DEBUG("udho::www::components::protocol::facet::header_reader", "HTTP header reader encountered timeout" , p::flow_id(_id));
                } else if (ec == boost::beast::http::error::end_of_stream){
                    UDHO_LOG_WARNING("udho::www::components::protocol::facet::header_reader", "HTTP header reader encountered end of stream", p::flow_id(_id));
                } else {
                    UDHO_LOG_WARNING("udho::www::components::protocol::facet::header_reader", "Removing reader designated for flow due to error " + ec.message(), p::flow_id(_id));
                }

                next.fail(ec);
                // TODO Failure path is async
                //      Hence sync removal of the flow is inappropriate
                //      Move it to the destructor
                // _component.remove(_id);
            }
        }, timeout_secs);
    }

    /**
     * @brief Invokes header reading.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        std::cout << "-> facet<components::protocol<ProtocolT, StreamT>, udho::www::feature::header_reader>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }

    ~facet() {
        std::cout << __FUNCTION__ << std::endl;
        _component.remove(_id);
    }
private:
    component_type&     _component;
    const config_type&  _config;
    std::size_t         _id;
private:
    struct http_reader{};
};

/**
 * @brief Portal accessor for the parsed request and body.
 * @tparam ProtocolT Protocol type.
 * @tparam StreamT Stream type.
 * @tparam JournalT Journal view type.
 * @ingroup DoxyG_www_components
 */
template <typename ProtocolT, typename StreamT, typename JournalT>
struct accessor<udho::www::components::protocol<ProtocolT, StreamT>, JournalT>: basic_accessor<udho::www::components::protocol<ProtocolT, StreamT>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::protocol<ProtocolT, StreamT>, JournalT>;
    using component_type        = udho::www::components::protocol<ProtocolT, StreamT>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;
    using request_type          = udho::net::types::headers::request;

    using basic_accessor_type::basic_accessor_type;

    /** @brief Returns the parsed request header from the journal. */
    const request_type& request() const {
        return basic_accessor_type::journal().template at<udho::www::feature::header_reader>();
    }

    /** @brief Returns the body-reader result from the journal. */
    const udho::www::feature::body_reader::result& body() const {
        return basic_accessor_type::journal().template at<udho::www::feature::body_reader>();
    }
};

/**
 * @brief Reads non-GET request bodies through the protocol reader.
 * @tparam ProtocolT Protocol type.
 * @tparam StreamT Stream type.
 * @ingroup DoxyG_www_components
 */
template <typename ProtocolT, typename StreamT>
struct facet<udho::www::components::protocol<ProtocolT, StreamT>, udho::www::feature::body_reader>{
    using component_type  = udho::www::components::protocol<ProtocolT, StreamT>;
    using reader_type     = typename component_type::reader_type;
    using writer_type     = typename component_type::writer_type;
    using stream_type     = typename component_type::stream_type;
    using reader_ptr_type = std::shared_ptr<reader_type>;
    using config_type     = udho::manifold::config<component_type>;

    /**
     * @brief Constructs the facet.
     * @param component Protocol component.
     * @param config Component configuration.
     * @param id Flow identifier.
     */
    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config), _id(id) {}

    /**
     * @brief Reads or skips the request body according to the request method.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        const udho::www::feature::header_reader::result& request = journal.template at<udho::www::feature::header_reader>();
        const udho::www::feature::identifier::result& identifier = journal.template at<udho::www::feature::identifier>();

        if(request.method() == boost::beast::http::verb::get) {
            next.skip();
            return;
        } else {
            using result_type = udho::www::feature::body_reader::result;

            std::size_t timeout_secs   = _config[udho::www::params::protocol::body_time_limit::val].value();     // Mitigate CWE-400 w.r.t. time consumed (slowloris attack)
            std::size_t memory_limit   = _config[udho::www::params::protocol::body_memory_limit::val].value();   // Mitigate CWE-400, CWE-770; read until eof not allowed unless eof comes before memort_limit exhausts
            std::size_t field_limit    = _config[udho::www::params::protocol::field_memory_limit::val].value();
            bool use_contiguous_buffer = _config[udho::www::params::protocol::contiguous_buffer::val].value();   // overridable by user

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
                    udho::www::feature::body_reader::result result(content_type, std::move(bresult), ec, bytes_transferred);
                    next(std::move(result), !ec); // The operator() overload on next forwards that call to pass or fail depending on !ec
                }, config);
            } else {
                reader->upload_to_multi_buffer(request, [this, next{std::move(next)}, &content_type, use_contiguous_buffer](udho::net::protocols::body_reader_result<boost::beast::multi_buffer>&& bresult, boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                    udho::www::feature::body_reader::result result(content_type, std::move(bresult), ec, bytes_transferred);
                    next(std::move(result), !ec); // The operator() overload on next forwards that call to pass or fail depending on !ec
                }, config);
            }
        }
    }


    /**
     * @brief Invokes body reading.
     * @tparam Components Components represented by the journal.
     * @tparam NextT Continuation type.
     * @param journal Current flow journal.
     * @param next Pipeline continuation.
     * @param stream Current stream.
     */
    template <typename... Components, typename NextT>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, stream_type& stream) const {
        std::cout << "-> facet<components::protocol<ProtocolT, StreamT>, udho::www::feature::body_reader>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }

    ~facet() {
        std::cout << __FUNCTION__ << std::endl;
        _component.remove(_id);
    }
private:
    component_type&     _component;
    const config_type&  _config;
    std::size_t         _id;
};

} // manifold
} // udho

/** @} */

#endif // UDHO_WWW_COMPONENTS_PROTOCOL_H
