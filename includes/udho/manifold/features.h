#ifndef UDHO_MANIFOLD_FEATURES_H
#define UDHO_MANIFOLD_FEATURES_H

#include <map>
#include <type_traits>
#include <udho/manifold/fwd.h>
#include <udho/net/common.h>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <udho/cookies/jar.h>
#include <udho/session/note.h>
#include <udho/url/route_index.h>
#include <udho/net/protocols/form_data.h>
#include <udho/net/protocols/body_reader_result.h>
#include <boost/asio/buffers_iterator.hpp>
#include <nlohmann/json.hpp>

namespace udho {
namespace manifold {

/**
 * @addtogroup DoxyG_manifold
 * @{
 */

/**
 * @brief manifold features
 *
 * Defines the types of functionality a manifold component can provide
 */
namespace feature{

}

namespace detail{

template <typename... Features>
struct feature_max_stage;

template <typename F, typename... Features>
struct feature_max_stage<F, Features...>{
private:
    static constexpr std::size_t value_rest = feature_max_stage<Features...>::value;
public:
    static constexpr std::size_t value = F::stage >= value_rest ? F::stage : value_rest;
};

template <typename F>
struct feature_max_stage<F>{
private:
public:
    static constexpr std::size_t value = F::stage;
};

template <>
struct feature_max_stage<>{
private:
public:
    static constexpr std::size_t value = 0;
};

}

template <typename... Features>
struct features{
    template <typename FeatureT>
    using has = std::disjunction<std::is_same<FeatureT, Features>...>;

    static constexpr std::size_t max_stage = detail::feature_max_stage<Features...>::value;
};

/**
 * @}
 */

}
}



#endif // UDHO_MANIFOLD_FEATURES_H
