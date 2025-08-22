#ifndef UDHO_MANIFOLD_PIPELINE_H
#define UDHO_MANIFOLD_PIPELINE_H

#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/composition.h>
#include <udho/manifold/journal.h>
#include <udho/manifold/evaluator.h>

namespace udho {
namespace manifold {

namespace detail{

template <typename FabricT>
struct get_handler_type;

template <std::size_t Stage, typename... Facets>
struct get_handler_type<udho::manifold::fabric<Stage, Facets...>>{
    template <typename... Features>
    using for_features = typename detail::evaluator_helper<Stage, Features...>::template handler<Facets...>;
};

};

template <std::size_t Stage, typename... Components>
struct basic_pipeline{
    using fabric_type      = typename udho::manifold::detail::flatten_all<Stage, Components...>::type;
    using journal_type     = typename udho::manifold::detail::journal_for_fabric<fabric_type>::type;

    template <typename... XComponents>
    basic_pipeline(udho::manifold::composition<XComponents...>& composition): _fabric(composition) {}

    template <typename... Features>
    struct evaluator{
        using helper_type  = detail::evaluator_helper<Stage, Features...>;
        using handler_type = typename detail::get_handler_type<fabric_type>::template for_features<Features...>;
        using promise_type = std::promise<bool>;

        evaluator(fabric_type& fabric, journal_type& journal): _handler(fabric, journal, _promise) {}

        template <typename... Args>
        void eval(Args&&... args){
            _handler.template operator()<0>(std::forward<Args>(args)...);
        }

        promise_type& promise() { return _promise; }

        private:
        handler_type _handler;
        promise_type _promise;
    };

    fabric_type& fabric() { return _fabric; }
    const fabric_type& fabric() const { return _fabric; }

    journal_type& journal() { return _journal; }
    const journal_type& journal() const { return _journal; }

    private:
    fabric_type  _fabric;
    journal_type _journal;

};

template <typename... Features>
struct order{};

template <std::size_t Stage, typename OrderT, typename CompositionT>
class common_pipepine;

template <std::size_t Stage, typename... Features, typename... Components>
class common_pipepine<Stage, order<Features...>, udho::manifold::composition<Components...>>: private basic_pipeline<Stage, Components...> {
    using composition_type    = udho::manifold::composition<Components...>;
    using basic_pipeline_type = basic_pipeline<Stage, Components...>;
    using evaluator_type      = typename basic_pipeline_type::template evaluator<Features...>;
    using promise_type        = typename evaluator_type::promise_type;

    evaluator_type _evaluator;

public:
    common_pipepine(composition_type& composition): basic_pipeline_type(composition), _evaluator(basic_pipeline_type::fabric(), basic_pipeline_type::journal()) {}

    promise_type& promise() { return _evaluator.promise(); }

    using basic_pipeline_type::fabric;
    using basic_pipeline_type::journal;

    template <typename... Args>
    void eval(Args&&... args){
        _evaluator.eval(std::forward<Args>(args)...);
    }

    template <typename... Args>
    std::future<bool> operator()(Args&&... args){
        eval(std::forward<Args>(args)...);
        return promise().get_future();
    }
};

// template <typename... Components>
// class pipeline<0, Components...>: basic_pipeline<0, Components...> {
//     using basic_pipeline_type = basic_pipeline<0, Components...>;

//     template <typename ConnectionT, typename... Features>
//     std::size_t operator()(ConnectionT&& conn, udho::manifold::evaluator<Features...>&& evaluator) {
//         return evaluator(basic_pipeline_type::fabric(), basic_pipeline_type::journal());
//     }
// };

}
}

#endif // UDHO_MANIFOLD_PIPELINE_H
