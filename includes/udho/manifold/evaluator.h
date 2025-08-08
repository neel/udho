#ifndef UDHO_MANIFOLD_EVALUATOR_H
#define UDHO_MANIFOLD_EVALUATOR_H

#include <boost/asio/ip/address.hpp>
#include <udho/net/common.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/journal.h>

namespace udho {
namespace manifold {


namespace detail {

template <typename... Facets>
struct facet_evaluator{
    using journal_type = typename udho::manifold::detail::journal_for_facets<Facets...>::type;

    facet_evaluator(journal_type& journal, const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _journal(journal), _address(address), _request(request) {}

    template <typename FacetT>
    bool operator()(udho::manifold::detail::facet_wrapper<FacetT, true>& wrapper) { return wrapper.eval(_journal, _address, _request); }

    template <typename FacetT>
    bool operator()(udho::manifold::detail::facet_wrapper<FacetT, false>&) { return false; }

    journal_type& _journal;

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

}

template <typename FeatureX, typename... Features>
struct evaluator<FeatureX, Features...>{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename... Facets>
    std::size_t operator()(udho::manifold::mediator<Facets...>& mediator, typename udho::manifold::detail::journal_for_facets<Facets...>::type& journal) {
        static_assert(mediator.template count<FeatureX>() > 0, "Feature missing in the manifold facade");

        std::size_t count = mediator.template apply<FeatureX>( detail::facet_evaluator<Facets...>{journal, _address, _request} );
        evaluator<Features...> ev{_address, _request};
        count += ev(mediator, journal);
        return count;
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};

template <typename FeatureX>
struct evaluator<FeatureX>{
    evaluator(const boost::asio::ip::address& address, const udho::net::types::headers::request& request): _address(address), _request(request) {}

    template <typename... Facets>
    std::size_t operator()(udho::manifold::mediator<Facets...>& mediator, typename udho::manifold::detail::journal_for_facets<Facets...>::type& journal) {
        static_assert(mediator.template count<FeatureX>() > 0, "Feature missing in the manifold facade");

        return mediator.template apply<FeatureX>( detail::facet_evaluator<Facets...>{journal, _address, _request} );
    }

    const boost::asio::ip::address& _address;
    const udho::net::types::headers::request& _request;
};


}
}

#endif // UDHO_MANIFOLD_EVALUATOR_H
