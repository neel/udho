#ifndef UDHO_MANIFOLD_BASIC_PIPELINE_H
#define UDHO_MANIFOLD_BASIC_PIPELINE_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/detail.h>
#include <udho/manifold/evaluator.h>

namespace udho{
namespace manifold{

/**
 * @brief Basic pipeline implementation for a single stage
 *
 * Provides the core evaluation infrastructure for a specific pipeline stage,
 * managing the fabric of components and their evaluation order.
 *
 * @tparam Stage The pipeline stage index
 * @tparam Components... The component types available in this composition
 */
template <std::size_t Stage, typename... Components>
struct basic_pipeline{
    using fabric_type       = typename udho::manifold::detail::flatten_all<Stage, Components...>::type;
    using full_journal_type = typename detail::get_journal_for_all_components<Components...>::type;

    /**
     * @brief Constructs a basic pipeline for a composition
     *
     * @tparam XComponents... Component types in the provided composition
     * @param composition The component composition to evaluate
     * @param configs Configuration for all components
     */
    template <typename... XComponents>
    basic_pipeline(udho::manifold::composition<XComponents...>& composition, const udho::manifold::configs<XComponents...>& configs, std::size_t id): _fabric(composition, configs, id) {}

    /**
     * @brief Evaluator for executing features in a specific order
     *
     * Provides type-safe evaluation of features in the specified order,
     * managing the flow control between facets and handling completion callbacks.
     *
     * @tparam Features... The feature types to evaluate in order
     */
    template <typename... Features>
    struct evaluator{
        using helper_type         = detail::evaluator_helper<Stage, Features...>;
        using handler_type        = typename detail::get_handler_type<full_journal_type, fabric_type>::template for_features<Features...>;
        using safe_success_type   = typename handler_type::safe_success_type;
        using async_callback_type = typename handler_type::async_callback_type;

        /**
         * @brief Constructs an evaluator for the pipeline stage
         *
         * @param fabric The fabric containing all facets for this stage
         * @param journal Journal for storing facet results
         * @param callback Completion callback for pipeline stage
         */
        evaluator(fabric_type& fabric, full_journal_type& journal, async_callback_type& callback): _handler(fabric, journal, callback) {}

        /**
         * @brief Evaluates the pipeline stage with the given arguments
         *
         * Initiates evaluation of all features in the specified order,
         * forwarding arguments to each facet.
         *
         * @tparam Args... Argument types to forward to facets
         * @param args Arguments to forward to facets
         */
        template <typename... Args>
        void eval(Args&&... args){
            _handler.template operator()<0>(std::forward<Args>(args)...);
        }

    private:
        handler_type _handler;
    };

    /// @name Fabric Access
    /// @{

    /**
     * @brief Gets the fabric for this pipeline stage
     * @return Reference to the stage's fabric
     */
    fabric_type& fabric() { return _fabric; }

    /**
     * @brief Gets the fabric for this pipeline stage (const)
     * @return Const reference to the stage's fabric
     */
    const fabric_type& fabric() const { return _fabric; }
    /// @}

private:
    fabric_type  _fabric;

};


}
}

#endif // UDHO_MANIFOLD_BASIC_PIPELINE_H
