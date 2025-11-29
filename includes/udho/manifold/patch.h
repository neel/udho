#ifndef UDHO_MANIFOLD_CONFIG_PATCH_H
#define UDHO_MANIFOLD_CONFIG_PATCH_H

#include <udho/hazo/map.h>
#include <udho/hazo/string/basic.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/params.h>

namespace udho{
namespace manifold{

template <typename... Params>
struct changeset: udho::hazo::map_d<Params...>{
    typedef udho::hazo::map_d<Params...> map_type;

    using map_type::map_type;

    template <typename ComponentT>
    void mutate(config<ComponentT>& config) {
        map_type::visit(mutate_{config});
    }

private:
    template <typename ComponentT>
    struct mutate_{
        using config_type = config<ComponentT>;

        mutate_(config_type& config): _config(config) {}

        template <typename ParamT, std::enable_if_t<config_type::template contains<ParamT>::value, bool> = true>
        void operator()(const ParamT& param) {
            _config[udho::hazo::element_t<ParamT>{}] = param.value();
        }

        template <typename ParamT, std::enable_if_t<!config_type::template contains<ParamT>::value, bool> = true>
        void operator()(const ParamT& param) { }

        config_type& _config;
    };
};

template <typename... Params>
changeset<Params...> make_changeset(Params&&... params){
    return changeset<Params...>{params...};
}


template <typename ParamsT, typename ChangesetT>
struct patch;

template <typename... Fields, typename... Params>
struct patch<udho::manifold::params<Fields...>, udho::manifold::changeset<Params...>> {
    using params_type    = udho::manifold::params<Fields...>;
    using changeset_type = udho::manifold::changeset<Params...>;

    patch(const params_type& params, const changeset_type& changeset): _params(params), _changeset(changeset) {}

    template <typename ParamT, std::enable_if_t<changeset_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){
        return _changeset[key];
    }

    template <typename ParamT, std::enable_if_t<changeset_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const {
        return _changeset[key];
    }

    template <typename ParamT, std::enable_if_t<!changeset_type::template contains<ParamT>::value, bool> = true>
    auto& operator[](const udho::hazo::element_t<ParamT>& key){
        return _params[key];
    }

    template <typename ParamT, std::enable_if_t<!changeset_type::template contains<ParamT>::value, bool> = true>
    const auto& operator[](const udho::hazo::element_t<ParamT>& key) const {
        return _params[key];
    }

private:
    const params_type& _params;
    const changeset_type& _changeset;
};

template <typename... Fields, typename... Params>
patch<udho::manifold::params<Fields...>, udho::manifold::changeset<Params...>> make_patch(const udho::manifold::params<Fields...>& params, const udho::manifold::changeset<Params...>& changeset){
    return patch<udho::manifold::params<Fields...>, udho::manifold::changeset<Params...>>{params, changeset};
}

template <typename... Fields, typename... Params>
udho::hazo::map_d<detail::field<Fields>...> make_diff(const udho::manifold::params<Fields...>& params, const udho::manifold::changeset<Params...>& changeset){
    using changeset_type = udho::manifold::changeset<Params...>;
    return udho::hazo::map_d<detail::field<Fields>...>{changeset_type::template contains<Fields>::value...};
}

}
}

#endif // UDHO_MANIFOLD_CONFIG_PATCH_H
