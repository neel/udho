#ifndef UDHO_MANIFOLD_TRANSITION_H
#define UDHO_MANIFOLD_TRANSITION_H

#include <cstdlib>
#include <udho/manifold/fwd.h>
#include <udho/manifold/runtime.h>

namespace udho{
namespace manifold{

/**
 * @brief Configuration patching between pipeline stages
 *
 * The default configuration patching during stage transition. The apply() method
 * is called after a stage completes successfully, before the next stage begins, to
 * implement custom configuration.
 *
 * @warning Usercode should not specialize this template. Instead specialize patch
 * and call patch_config::apply in order to apply the default patching for the stage.
 *
 * @tparam LabelT The pipeline label type
 * @tparam Stage The stage index from which the transition occurs
 */
template <typename LabelT, typename StreamT, std::size_t Stage>
struct default_transition{
    using label_type        = LabelT;
    using stream_type       = StreamT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = basic_runtime<label_type, StreamT>;
    using flow_type         = udho::manifold::basic_flow<LabelT, StreamT>;
    using pipeline_type     = typename runtime_type::template pipeline_at<Stage>;
    using configs_type      = typename runtime_type::configs_type;

    /**
     * @brief Applies configuration patches between stages
     *
     * Called after stage completion to modify configuration before
     * the next stage begins. The default implementation does nothing.
     *
     * @param p The completed pipeline stage
     * @param config The configuration to modify for the next stage
     */
    template <typename... Args>
    static void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, configs_type& config, Args&&... args) {
        p.next(flow, std::forward<Args>(args)...);
    }
};

/**
 * @brief Configuration patching between pipeline stages
 *
 * Usercode should specialize this template to implement custom configuration
 * modifications when transitioning between pipeline stages. The apply() method
 * is called after a stage completes successfully, before the next stage begins.
 *
 * @tparam LabelT The pipeline label type
 * @tparam Stage The stage index from which the transition occurs
 */
template <typename LabelT, typename StreamT, std::size_t Stage>
struct transition{
    using label_type        = LabelT;
    using stream_type       = StreamT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = basic_runtime<label_type, stream_type>;
    using flow_type         = udho::manifold::basic_flow<LabelT, StreamT>;
    using pipeline_type     = typename runtime_type::template pipeline_at<Stage>;
    using configs_type      = typename runtime_type::configs_type;

    /**
     * @brief Applies configuration patches between stages
     *
     * Called after stage completion to modify configuration before
     * the next stage begins. The default implementation does nothing.
     *
     * @param p The completed pipeline stage
     * @param config The configuration to modify for the next stage
     */
    template <typename... Args>
    static void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, configs_type& config, Args&&... args) {
        udho::manifold::default_transition<LabelT, StreamT, Stage>::apply(flow, p, config, std::forward<Args>(args)...);
    }
};

namespace detail {

/**
 * @brief Compile-time patcher for all pipeline stages
 *
 * Recursively applies patch_config specializations for each stage
 * transition in the pipeline.
 *
 * @tparam LabelT The pipeline label type
 * @tparam Count Total number of pipeline stages
 * @tparam Stage Current stage index
 */
template <typename LabelT, typename StreamT, std::size_t Count, std::size_t Stage>
struct transitioner: public detail::transitioner<LabelT, StreamT, Count, Stage+1>/*, private udho::manifold::patch<LabelT, Stage>*/{
    using label_type        = LabelT;
    using stream_type       = StreamT;
    using sketch_type       = sketch<label_type>;
    using runtime_type      = basic_runtime<label_type, stream_type>;
    using flow_type         = udho::manifold::basic_flow<label_type, stream_type>;
    using pipeline_type     = typename runtime_type::template pipeline_at<Stage>;
    using configs_type      = typename runtime_type::configs_type;

    /**
     * @brief apply patch_config on the configs with the current pipeline
     * @param p
     * @param configs
     */
    template <typename... Args>
    void apply(std::shared_ptr<flow_type> flow, pipeline_type& p, configs_type& configs, Args&&... args){
        udho::manifold::transition<LabelT, StreamT, Stage>::apply(flow, p, configs, std::forward<Args>(args)...);
    }
};

template <typename LabelT, typename StreamT, std::size_t Count>
struct transitioner<LabelT, StreamT, Count, Count>{};

}

}
}

#endif // UDHO_MANIFOLD_TRANSITION_H
