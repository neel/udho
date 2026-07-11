#ifndef UDHO_WWW_TERMINAL_H
#define UDHO_WWW_TERMINAL_H

#include <udho/manifold/terminal.h>
#include <udho/www/label.h>
#include <udho/www/components/handler.h>
#include <udho/exceptions/exceptions.h>
#include <udho/www/pages.h>
#include <boost/exception/diagnostic_information.hpp>
#include <cpptrace/cpptrace.hpp>

namespace udho {
namespace manifold {

// { terminal

/**
 * @brief Error-page type policy for a www runtime label.
 *
 * Maps a label to the page types used for rendering server and client errors.
 * Specialize this template to customize error-page rendering for a particular
 * label.
 *
 * @tparam Label www label type.
 *
 * @ingroup www
 */
template <typename Label>
struct error{
    /**
     * @brief Server-error page template used by the label.
     *
     * @tparam OstreamT Output stream type used by the error page.
     */
    template <typename OstreamT>
    using server_error = udho::www::pages::server_error<OstreamT>;

    /**
     * @brief Client-error page template used by the label.
     *
     * @tparam ContextT Request context type used by the error page.
     */
    template <typename ContextT>
    using client_error = udho::www::pages::client_error<ContextT>;
};


/**
 * @brief www terminal policy for HTTP connection flow handling.
 *
 * This terminal specialization keeps successful flows alive for the next
 * request, renders HTTP/client/server error pages, aborts flows on stream-end
 * or cancellation, and routes user-code exceptions through the active response
 * stream.
 *
 * @tparam StreamT Stream type used by the runtime.
 * @tparam Tag www label tag.
 * @tparam ExtraComponents Extra components appended to the www label.
 *
 * @ingroup www
 */
template <typename StreamT, typename Tag, typename... ExtraComponents>
struct basic_terminal<www::basic_label<StreamT, Tag, ExtraComponents...>, StreamT> {
    using label_type        = www::basic_label<StreamT, Tag, ExtraComponents...>;
    using stream_type       = StreamT;
    using ostream_type      = udho::net::basic_ostream<StreamT>;
    using runtime_type      = basic_runtime<label_type, StreamT>;
    using handler_type      = udho::www::components::basic_handler<StreamT>;
    using flow_type         = typename runtime_type::flow_type;
    using composition_type  = typename runtime_type::composition_type;
    using journal_type      = typename flow_type::journal_type;
    using configs_type      = typename runtime_type::configs_type;
    using portal_type       = typename udho::manifold::detail::get_portal_type<composition_type>::type;
    using context_type      = typename udho::manifold::detail::get_context_for_portal<StreamT, portal_type>::type;
    using trace_type        = udho::exceptions::captured::trace_type;
    using pages_type        = error<label_type>;

    template <typename OstreamT>
    using server_error = typename pages_type::template server_error<OstreamT>;

    template <typename ContextT>
    using client_error = typename pages_type::template client_error<ContextT>;

    basic_terminal() = delete;
    basic_terminal(const basic_terminal&) = delete;

    /**
     * @brief Constructs a terminal bound to runtime state.
     *
     * The terminal stores references to the runtime composition, configs, and
     * current journal. These references are used while handling re-entry and
     * errors.
     *
     * @param composition Runtime component composition.
     * @param configs Runtime configuration collection.
     * @param journal Journal associated with the current flow.
     *
     * @warning The referenced objects must outlive the terminal.
     */
    basic_terminal(composition_type& composition, configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

    basic_terminal(basic_terminal&&) = delete;

    /**
     * @brief reenter is synchronously called after successful evaluation of all facet pipelines in all stages
     *        to determine whether to process next request or abort.
     * @param stream
     * @return bool
     * @note Call originates from basic_flow<LabelT, StreamT>::reenter()
     */
    bool reenter(stream_type& stream) {
        return true;
    }

    /**
     * @brief prepare's the flow for reentry in both success and failure circumstances
     * @param stream
     * @param args
     */
    template <typename... Args>
    void prepare(stream_type& stream, Args&&... args) { }

    /**
     * @brief error function is called to handle the error situation occurred during evaluation of some facet.
     *
     * This function asynchronously determines the either of the following two pathways
     *
     * 1. flow.restart() respond, reset and process the next request in the same socket
     * 2. flow.abort()   remove the flow, terminate the socket
     *
     * @param success
     * @param flow
     * @param stream
     * @param args
     * @pre flow is owned by runtime and outlives the invocation of this callback
     * @post flow either restarts or aborts
     * @note Call originate from basic_flow<LabelT, StreamT>::error which originates from failure handler
     *       passed to facet triggered by calling next.fail(...)
     */
    template <typename... Args>
    void internal_error(udho::manifold::evaluation_result success, flow_type& flow, stream_type& stream, Args&&... args){
        if(success.has_exception()) {
            try{
                success.rethrow();
            } catch(const udho::http::error& error) {
                // std::cout << "exception: " << error.what() << std::endl;
                handle_http_error(flow, error, success.capex().trace(), stream, std::forward<Args>(args)...);
            } catch(const std::system_error& error) {
                std::cout << "std::system_error: " << error.what() << std::endl;
                handle_error(flow, error.code(), success.capex().trace(), stream, std::forward<Args>(args)...);
            } catch(const boost::system::system_error& error) {
                std::cout << "boost::system::system_error: " << error.what() << std::endl;
                handle_error(flow, error.code(), success.capex().trace(), stream, std::forward<Args>(args)...);
            } catch(const boost::exception& bex) {
                std::cout << "boost exception: " << boost::diagnostic_information_what(bex) << std::endl;
                handle_error(flow, bex, success.capex().trace(), stream, std::forward<Args>(args)...);
            } catch(const std::exception& ex) {
                std::cout << "exception: " << ex.what() << std::endl;
                handle_error(flow, ex, success.capex().trace(), stream, std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief Handles an exception captured from user route/action code.
     *
     * The function retrieves the active response stream from the handler and renders
     * a server-error page using the captured exception and resolved stack trace.
     *
     * @tparam Args Additional runtime argument types.
     * @param capex Captured exception from user code.
     * @param flow Flow that encountered the exception.
     * @param stream Active stream.
     * @param args Additional runtime arguments.
     */
    template <typename... Args>
    void user_error(const udho::exceptions::captured& capex, flow_type& flow, stream_type& stream, Args&&... args){
        handler_type& handler = _composition.template get<handler_type>().component();
        ostream_type& ostream = handler.ostream(flow.id()); // Expect ostream to exist

        assert(ostream.has_exception());

        try{
            capex.rethrow();
        } catch(const std::exception& exception) {
            server_error<ostream_type> server_error(ostream);
            cpptrace::stacktrace stacktrace = capex.trace().resolve();
            server_error(exception, stacktrace);
        }
    }

private:

    template <typename... Args>
    void handle_http_error(flow_type& flow, const udho::http::error& error, const trace_type& trace, stream_type& stream, Args&&... args) {
        ostream_type& ostream = get_ostream(flow, true, stream, std::forward<Args>(args)...);

        if(error.status_class() == boost::beast::http::status_class::client_error) {
            portal_type portal(_composition, _configs, _journal);
            context_type context(ostream, portal, flow.id());

            client_error<context_type> error_page(context);
            error_page(error.status(), error.what());
        } else if(error.status_class() == boost::beast::http::status_class::server_error) {
            server_error<ostream_type> server_error(ostream);
            cpptrace::stacktrace stacktrace = trace.resolve();
            server_error(error, stacktrace);
         } else {
            ostream.status(error.status());
            ostream << error.what();
            ostream.finish();
        }
    }

    template <typename... Args>
    void handle_error(flow_type& flow, const boost::system::error_code& error, const trace_type& trace, stream_type& stream, Args&&... args) {
        if(error == boost::beast::http::error::end_of_stream) {
            flow.abort();
        } else {
            ostream_type& ostream = get_ostream(flow, false, stream, std::forward<Args>(args)...);

            server_error<ostream_type> server_error(ostream);
            cpptrace::stacktrace stacktrace = trace.resolve();
            server_error(error, stacktrace);
        }
    }

    template <typename... Args>
    void handle_error(flow_type& flow, const std::error_code& error, const trace_type& trace, stream_type& stream, Args&&... args) {
        if(error.value() == boost::system::errc::operation_canceled) {
            // most likely before of timeout while waiting for HTTP headers
            flow.abort();
        } else {
            ostream_type& ostream = get_ostream(flow, false, stream, std::forward<Args>(args)...);

            server_error<ostream_type> server_error(ostream);
            cpptrace::stacktrace stacktrace = trace.resolve();
            server_error(error, stacktrace);
        }
    }

    template <typename... Args>
    void handle_error(flow_type& flow, const boost::exception& exception, const trace_type& trace, stream_type& stream, Args&&... args) {
        ostream_type& ostream = get_ostream(flow, false, stream, std::forward<Args>(args)...);

        server_error<ostream_type> server_error(ostream);
        cpptrace::stacktrace stacktrace = trace.resolve();
        server_error(exception, stacktrace);
    }

    template <typename... Args>
    void handle_error(flow_type& flow, const std::exception& exception, const trace_type& trace, stream_type& stream, Args&&... args) {
        ostream_type& ostream = get_ostream(flow, false, stream, std::forward<Args>(args)...);

        server_error<ostream_type> server_error(ostream);
        cpptrace::stacktrace stacktrace = trace.resolve();
        server_error(exception, stacktrace);
    }

private:

    template <typename... Args>
    ostream_type& get_ostream(flow_type& flow, bool restart, stream_type& stream, Args&&... args) {
        handler_type& handler = _composition.template get<handler_type>().component();
        if(handler.exists(flow.id())) {
            return handler.ostream(flow.id());
        }

        auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
        auto lambda = [&flow, restart, &stream, args_tuple = std::move(args_tuple)](boost::system::error_code error, std::size_t bytes_written){
            if(error) {
                // TODO Error while writing to socket
                return;
            }

            if(restart) {
                std::apply(
                    [&](auto&&... args) {
                        flow.restart(stream, std::forward<Args>(args)...);
                    },
                    args_tuple
                );
            } else {
                flow.abort();
            }
        };

        auto ex_lambda = [&flow, restart, &stream, args_tuple = std::move(args_tuple)](ostream_type& ostream){
            ostream.finish();
        };

        ostream_type& ostream = handler.add(flow.id(), stream, std::move(lambda), std::move(ex_lambda));
        return ostream;
    }

private:
    composition_type&   _composition;
    configs_type&       _configs;
    const journal_type& _journal;
};

// } terminal


}
}

#endif // UDHO_WWW_TERMINAL_H
