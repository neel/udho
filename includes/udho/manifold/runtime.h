#ifndef UDHO_MANIFOLD_RUNTIME_H
#define UDHO_MANIFOLD_RUNTIME_H

#include <functional>
#include <list>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <udho/manifold/fwd.h>
#include <udho/manifold/pipeline.h>
#include <udho/manifold/portal.h>
#include <udho/manifold/context.h>
#include <udho/logging/macros.h>
#include <udho/utils/format.h>
#include <udho/utils/misc.h>

namespace udho{
namespace manifold{

/**
 * @ingroup DoxyG_manifold
 * @{
 */

/**
 * @brief Runtime manager for pipeline executions
 *
 * Manages the lifecycle of flows providing:
 * - Composition management
 *   - constructs the composition through the constructor
 * - Thread-safe Flow spawning and tracking
 *   - creates a flow with a reference to this runtime, composition and baseline configurations
 *   - manages a collection of active flows
 *   - terminated flows are removed from the collection
 * - Baseline configuration loading/saving
 *   - default constructs the default configuration
 *   - possible to load from json after instantiation with default values
 *
 * @tparam LabelT The label type identifying the pipeline configuration
 */
template <typename LabelT, typename StreamT>
struct basic_runtime{
    using label_type        = LabelT;
    using stream_type       = StreamT;
    using sketch_type       = sketch<label_type>;
    using composition_type  = typename sketch_type::composition_type;
    using order_type        = typename sketch_type::order_type;
    using configs_type      = typename composition_type::configs_type;
    using hook_type         = std::function<void (std::size_t)>;
    using hooks_list_type   = std::list<hook_type>;
    using flow_type         = basic_flow<label_type, stream_type>;
    using collection_type   = std::unordered_map<std::size_t, flow_type>;
    using hooks_assoc_type  = std::unordered_map<std::size_t, hooks_list_type>;
    using portal_type       = typename detail::get_portal_type<composition_type>::type;
    using context_type      = typename detail::get_context_for_portal<StreamT, portal_type>::type;

    friend struct basic_flow<label_type, stream_type>;

    static constexpr std::size_t Count = detail::composition_max_stage<composition_type>::value +1;

    /// @name Pipeline Type Aliases
    /// @{
    template <int Stage>
    using pipeline_at          = pipeline<composition_type, order_type, Count, Stage>;
    using start_pipeline_type  = pipeline_at<-1>;
    using finish_pipeline_type = pipeline_at<Count>;
    /// @}

    /**
     * @brief Constructs a runtime
     *
     * @param args Components used to construct the composition
     */
    template <typename... Args>
    static composition_type compose(Args&&... args) { return composition_type::compose(std::forward<Args>(args)...); }

    basic_runtime(basic_runtime&&) = delete;
    basic_runtime& operator=(basic_runtime&&) = delete;

    basic_runtime(composition_type&& composition): _composition(std::move(composition)) {}

    template <typename... Components>
    basic_runtime(Components&&... components): _composition(composition_type::compose(std::forward<Components>(components)...)) {}

    /// @name Composition Access
    /// @{
    composition_type& composition() { return _composition; }
    const composition_type& composition() const { return _composition; }
    /// @}

    /// @name Configuration
    /// @{
    const configs_type& baseline() const {return _baseline; }
    configs_type& baseline() {return _baseline; }

    /**
     * @brief Loads baseline configuration from JSON
     *
     * @param json JSON object containing configuration for all components
     */
    void load(const nlohmann::json& json){
        _baseline.load(json);
    }
    /// @}

    /// @name Flow Management
    /// @{

    /**
     * @brief Spawns a new flow for pipeline execution
     *
     * Creates a new flow instance associated with this runtime.
     * The flow is tracked internally until it completes.
     *
     * @return Shared pointer to the new flow
     */
    flow_type& spawn(stream_type&& stream) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto pair = _flows.emplace(std::piecewise_construct, std::forward_as_tuple(flow_type::counter()), std::forward_as_tuple(*this, std::move(stream)));

        if(!pair.second) {
            throw std::runtime_error("Failed to spawn new flow");
        }

        auto it = pair.first;
        flow_type& res = it->second;
        return res;
    }

    /**
     * @brief Gets the current number of active flows
     *
     * @return Number of flows currently being tracked
     */
    std::size_t count() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _flows.size();
    }

    /**
     * @brief Registers a callback invoked after a flow is removed.
     * @param flow_id Flow whose removal should trigger the callback.
     * @param callback Callback receiving the removed flow identifier.
     */
    void then(std::size_t flow_id, hook_type&& callback) {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_flows.find(flow_id) == _flows.end()) {
            throw std::invalid_argument{udho::utils::format("Cannot register callback for a non-existent flow {}", flow_id)};
        }
        _hooks[flow_id].emplace_back(std::move(callback));
    }
    /// @}

    void stop() {
        std::size_t flows_count = _flows.size();
        for(auto& flow: _flows) {

        }
    }

private:
    /**
     * @brief Asynchronously terminates a flow stream before removing the flow.
     * @param flow Flow retained by this runtime until termination completes.
     */
    void dispose(flow_type& flow) {
        const std::size_t flow_id = flow.id();

        try {
            udho::utils::misc::detail::stream_termination<stream_type>::async_apply(flow.stream(), [this, flow_id](boost::system::error_code error) {
                disposed(flow_id, error);
            });
        } catch(const std::exception& ex) {
            namespace p = udho::logging::params;
            UDHO_LOG_ERROR("udho::manifold", "Failed to initiate stream termination", p::flow_id(flow_id), p::exception(ex.what()));
            boost::system::error_code error = udho::utils::misc::detail::stream_termination<stream_type>::apply(flow.stream());
            disposed(flow_id, error);
        }
    }

    void notify(std::size_t flow_id) {
        hooks_list_type callbacks;

        { // move the callbacks
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _hooks.find(flow_id);
            if(it == _hooks.end()) return;
            callbacks.swap(it->second);
            _hooks.erase(it);
        }

        namespace p = udho::logging::params;
        try{
            for(hook_type& callback: callbacks) {
                callback(flow_id);
            }
        } catch(const std::exception& ex) {
            UDHO_LOG_ERROR("udho::manifold", "Exception thrown from post terminate callback", p::flow_id(flow_id), p::exception(ex.what()));
        } catch(...) {
            UDHO_LOG_ERROR("udho::manifold", "Unknown exception thrown from post terminate callback", p::flow_id(flow_id));
        }
    }

    void disposed(std::size_t flow_id, boost::system::error_code error) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _flows.find(flow_id);
            if(it == _flows.end()) {
                _hooks.erase(flow_id);
                return;
            }

            namespace p = udho::logging::params;
            if(error) {
                UDHO_LOG_DEBUG("udho::manifold", "Stream terminated with error " + error.message(), p::flow_id(flow_id));
            } else {
                UDHO_LOG_TRACE("udho::manifold", "Stream terminated", p::flow_id(flow_id));
            }

            _flows.erase(it);
        }

        notify(flow_id);
    }

private:
    composition_type   _composition;
    configs_type       _baseline;
private:
    collection_type    _flows;
    hooks_assoc_type   _hooks;
    mutable std::mutex _mutex;
    // TODO Add a bounded TLS-shutdown policy and make stop() drain pending disposals before runtime destruction.
};

/**
 * @}
 */

}
}

#endif // UDHO_MANIFOLD_RUNTIME_H
