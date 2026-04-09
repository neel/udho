#ifndef UDHO_MANIFOLD_RUNTIME_H
#define UDHO_MANIFOLD_RUNTIME_H

#include <memory>
#include <vector>
#include <udho/manifold/fwd.h>
#include <udho/manifold/pipeline.h>
#include <udho/manifold/portal.h>
#include <udho/manifold/context.h>
#include <udho/logging/macros.h>

namespace udho{
namespace manifold{

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
    using flow_type         = basic_flow<label_type, stream_type>;
    using flow_ptr_type     = std::shared_ptr<flow_type>;
    using collection_type   = std::vector<flow_ptr_type>;
    using portal_type       = typename detail::get_portal_type<composition_type>::type;
    using context_type      = typename detail::get_context_for_portal<StreamT, portal_type>::type;

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
     * @param composition Component composition for this runtime
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
    flow_ptr_type spawn(stream_type&& stream) {
        std::scoped_lock lock(_mutex);
        flow_ptr_type flow_ptr = flow_type::create(*this, std::forward<stream_type>(stream));
        _flows.push_back(flow_ptr);

        return flow_ptr;
    }

    /**
     * @brief Gets the current number of active flows
     *
     * @return Number of flows currently being tracked
     */
    std::size_t count() const {
        std::scoped_lock<std::mutex> lock(_mutex);
        return _flows.size();
    }

    /**
     * @brief Removes a completed flow from tracking
     *
     * Called by flows when they terminate to clean up resources.
     *
     * @param flow The flow to remove
     */
    bool remove(const flow_ptr_type& flow) {
        std::scoped_lock<std::mutex> lock(_mutex);
        auto it = std::find_if(_flows.begin(), _flows.end(), [id = flow->id()](const auto& f){
            return f->id() == id;
        });

        namespace p = udho::logging::params;
        if(it != _flows.end()) {
            // std::cerr << "remove() found flow: ptr=" << flow.get() << " id=" << flow->id() << " tracked=" << _flows.size() << "\n";
            UDHO_LOG_TRACE("udho::manifold", "Runtime removing flow", p::flow_id(flow->id()));
            _flows.erase(it);
            return true;
        } else {
            // std::cerr << "remove() missing flow: ptr=" << flow.get() << " id=" << flow->id() << " tracked=" << _flows.size() << "\n";
            UDHO_LOG_ERROR("udho::manifold", "Runtime failed to remove flow", p::flow_id(flow->id()));
            return false;
        }
    }
    /// @}

    void stop() {
        std::size_t flows_count = _flows.size();
        for(auto& flow: _flows) {

        }
    }

private:
    composition_type _composition;
    configs_type     _baseline;
private:
    collection_type    _flows;
    mutable std::mutex _mutex;
};

}
}

#endif // UDHO_MANIFOLD_RUNTIME_H
