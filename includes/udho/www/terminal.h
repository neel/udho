#ifndef UDHO_WWW_TERMINAL_H
#define UDHO_WWW_TERMINAL_H

#include <udho/manifold/terminal.h>
#include <udho/www/label.h>
#include <udho/www/components/handler.h>
#include <udho/exceptions/exceptions.h>
#include <udho/www/pages.h>

namespace udho {
namespace manifold {

// { terminal

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

    basic_terminal() = delete;
    basic_terminal(const basic_terminal&) = delete;

    basic_terminal(composition_type& composition, configs_type& configs, const journal_type& journal)
        : _composition(composition), _configs(configs), _journal(journal) {}

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
    void error(udho::manifold::evaluation_result success, flow_type& flow, stream_type& stream, Args&&... args){
        if(success.has_exception()) {
            try{
                success.rethrow();
            } catch(const udho::http::error& error) {
                // std::cout << "exception: " << error.what() << std::endl;
                handle_http_error(flow, error, stream, std::forward<Args>(args)...);
            } catch(const std::exception& ex) {
                std::cout << "exception: " << ex.what() << std::endl;
                handle_error(flow, ex, success.capex().trace(), stream, std::forward<Args>(args)...);
            }
        }
    }

    template <typename... Args>
    void captured_error(const udho::exceptions::captured& capex, flow_type& flow, stream_type& stream, Args&&... args){
        handler_type& handler = _composition.template get<handler_type>().component();
        ostream_type& ostream = handler.ostream(flow.id()); // Expect ostream to exist

        assert(ostream.has_exception());

        try{
            capex.rethrow();
        } catch(const std::exception& exception) {
            udho::www::pages::server_error<ostream_type> server_error(ostream);
            server_error(exception, capex.trace());
        }
    }

private:

    template <typename... Args>
    void handle_http_error(flow_type& flow, const udho::http::error& error, stream_type& stream, Args&&... args) {
        ostream_type& ostream = get_ostream(flow, true, stream, std::forward<Args>(args)...);

        portal_type portal(_composition, _configs, _journal);
        context_type context(ostream, portal, flow.id());

        if(error.status() == boost::beast::http::status::not_found) {
            udho::www::pages::not_found<context_type> error_page(context);
            error_page(error.what());
        } else {
            ostream.status(error.status());
            ostream << error.what();
            ostream.finish();
        }
    }

    template <typename... Args>
    void handle_error(flow_type& flow, const std::exception& error, const boost::stacktrace::stacktrace& trace, stream_type& stream, Args&&... args) {
        flow.abort();
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
