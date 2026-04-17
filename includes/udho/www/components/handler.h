#ifndef UDHO_WWW_COMPONENTS_HANDLER_H
#define UDHO_WWW_COMPONENTS_HANDLER_H

#include <map>
#include <functional>
#include <udho/utils/format.h>
#include <udho/www/features.h>
#include <udho/net/ostream.h>
#include <udho/url/summary.h>
#include <udho/manifold/portal.h>

namespace udho{
namespace www{

namespace components{

/**
 * @brief Flow-scoped registry of response streams and callbacks.
 *
 * This component stores one @ref responder per flow id. The responder owns an
 * @ref ostream_type bound to the flow stream and to user-provided completion and
 * exception callbacks.
 *
 * - responders are indexed by flow id;
 * - @ref add creates a responder the first time a flow id is seen;
 * - later calls to @ref add with the same flow id return the existing ostream
 *   instead of replacing the responder;
 * - the router summary is stored by value and exposed through @ref summary.
 *
 * @tparam StreamT Stream type associated with a flow.
 */
template <typename StreamT>
struct basic_handler{
    using features = udho::manifold::features<>;
    static constexpr const char* name = "handler";
    using params      = udho::manifold::params<>;

    using stream_type   = StreamT;
    using ostream_type  = udho::net::basic_ostream<stream_type>;

    /**
     * @brief Response helper associated with a single flow.
     *
     * A responder wraps an @ref ostream_type and forwards write-completion and
     * error-completion to user-supplied callbacks.
     *
     * The responder is keyed by flow id in @ref basic_handler, so all requests
     * that are processed sequentially through the same flow reuse the same
     * responder entry while that flow remains alive.
     */
    struct responder{
        /// Callback invoked when the output operation finishes.
        using callback_type             = std::function<void (boost::system::error_code, std::size_t)>;
        /// Callback invoked when the output stream reports an exception path.
        using exception_callback_type   = std::function<void (ostream_type&)>;

        /**
         * @brief Construct a responder for a flow stream.
         *
         * The internal output stream is initialized with internal hooks that
         * forward completion to @ref on_finish and exception handling to
         * @ref on_exception.
         *
         * @tparam FinishCallback Completion callback type.
         * @tparam ExceptionCallback Exception callback type.
         * @param stream Stream associated with the flow.
         * @param callback Callback invoked on output completion.
         * @param ex_callback Callback invoked on exception handling.
         */
        template <typename FinishCallback, typename ExceptionCallback>
        responder(stream_type& stream, FinishCallback&& callback, ExceptionCallback&& ex_callback)
            : _ostream(stream,
                    std::bind(&responder::on_finish, this, std::placeholders::_1, std::placeholders::_2),
                    std::bind(&responder::on_exception, this, std::placeholders::_1)
                )
            , _callback(callback_type{std::move(callback)}), _ex_callback(exception_callback_type{std::move(ex_callback)})
        {}
        responder(const responder&) = delete;
        responder(responder&&) = delete;

        /**
         * @brief Access the output stream associated with this responder.
         * @return Reference to the wrapped output stream.
         */
        ostream_type& ostream() { return _ostream; }
    private:

        /**
         * @brief Internal completion hook.
         *
         * Forwards the completion result to the user callback and then resets the
         * wrapped output stream.
         *
         * @param ec Completion status.
         * @param bytes_written Number of bytes reported as written.
         */
        void on_finish(boost::system::error_code ec, std::size_t bytes_written) {
            _callback(ec, bytes_written);
            _ostream.reset();
        }

        /**
         * @brief Internal exception hook.
         *
         * Forwards exception handling to the user-provided exception callback.
         *
         * @param ostream Output stream associated with this responder.
         */
        void on_exception(ostream_type& ostream){
            _ex_callback(ostream);
        }
    private:
        ostream_type  _ostream;
        callback_type _callback;
        exception_callback_type _ex_callback;
    };

    /// Container storing responders indexed by flow id.
    using collection_type = std::map<std::size_t, responder>;

    /**
     * @brief Construct a handler with router summary information.
     * @param summary Router summary stored by value.
     */
    basic_handler(const udho::url::summary::router& summary): _summary(summary) {}

    /**
     * @brief Get or create the responder stream for a flow.
     *
     * If a responder already exists for the given flow id, the existing output
     * stream is returned and the newly supplied callbacks are ignored.
     *
     * Otherwise, a new responder is created for that flow id and bound to the
     * supplied stream and callbacks.
     *
     * @tparam FinishCallback Completion callback type.
     * @tparam ExceptionCallback Exception callback type.
     * @param id Flow id.
     * @param stream Stream associated with the flow.
     * @param callback Completion callback used when creating a new responder.
     * @param ex_callback Exception callback used when creating a new responder.
     * @return Reference to the output stream associated with @p id.
     */
    template <typename FinishCallback, typename ExceptionCallback>
    ostream_type& add(std::size_t id, stream_type& stream, FinishCallback&& callback, ExceptionCallback&& ex_callback){
        auto responder_it = _responders.find(id);
        if(responder_it != _responders.end()) {
            assert(responder_it->first == id);
            return responder_it->second.ostream();
        }

        bool success = false;
        std::tie(responder_it, success) = _responders.emplace(std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(stream, std::forward<FinishCallback>(callback), std::forward<ExceptionCallback>(ex_callback))
        );
        assert(success);
        assert(responder_it->first == id);
        return responder_it->second.ostream();
    }

    /**
     * @brief Access the responder associated with a flow id.
     *
     * @pre A responder for @p id must already exist.
     *
     * @param id Flow id.
     * @return Reference to the stored responder.
     */
    responder& responder(std::size_t id) {
        auto responder_it = _responders.find(id);
        assert(responder_it != _responders.end());
        assert(responder_it->first == id);

        return responder_it->second;
    }

    /**
     * @brief Access the output stream associated with a flow id.
     *
     * @pre A responder for @p id must already exist.
     *
     * @param id Flow id.
     * @return Reference to the responder output stream.
     */
    ostream_type& ostream(std::size_t id) {
        return responder(id).ostream();
    }

    /**
     * @brief Return the router summary associated with this handler.
     * @return Const reference to the stored router summary.
     */
    const udho::url::summary::router& summary() const { return _summary; }

private:
    collection_type _responders;
    udho::url::summary::router _summary;
};

} // components
} // www

namespace manifold{

template <typename StreamT, typename JournalT>
struct accessor<udho::www::components::basic_handler<StreamT>, JournalT>: basic_accessor<udho::www::components::basic_handler<StreamT>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::basic_handler<StreamT>, JournalT>;
    using component_type        = udho::www::components::basic_handler<StreamT>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;

    using basic_accessor_type::basic_accessor_type;

    const udho::url::summary::router& routes() const {
        return basic_accessor_type::component().summary();
    }

    const udho::url::summary::mount_point& route(const std::string& name) const {
        return routes()[name];
    }
    template <typename Char, Char... C>
    const udho::url::summary::mount_point& route(udho::hazo::string::str<Char, C...>&& hstr) const {
        return route(hstr.str());
    }

};


} // manifold
} // udho

#endif // UDHO_WWW_COMPONENTS_HANDLER_H
