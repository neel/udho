#ifndef UDHO_MANIFOLD_ORDER_H
#define UDHO_MANIFOLD_ORDER_H

namespace udho{
namespace manifold{

/**
 * @brief Defines the evaluation order of features within a stage
 *
 * The order template specifies the sequence in which features should be
 * evaluated within a pipeline stage. Features are evaluated in the order
 * they appear in the template parameter list.
 *
 * @tparam Features... The feature types in evaluation order
 *
 * @ingroup DoxyG_manifold
 */
template <typename... Features>
struct order{};

}
}

#endif // UDHO_MANIFOLD_ORDER_H
