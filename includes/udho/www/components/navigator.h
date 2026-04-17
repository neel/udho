#ifndef UDHO_WWW_COMPONENTS_NAVIGATOR_H
#define UDHO_WWW_COMPONENTS_NAVIGATOR_H

#include <udho/www/features.h>
#include <udho/manifold/config.h>
#include <udho/manifold/portal.h>
#include <udho/utils/encoding.h>
#include <iostream>
#include <udho/exceptions/exceptions.h>

namespace udho{
namespace www{

namespace components{

struct pretty_url_policy{
    inline udho::www::feature::identifier::result operator()(const udho::net::types::headers::request& request){
        udho::www::feature::identifier::result result;
        auto target = request.target();
        extract(result, target);
        return result;
    }

    template <typename StrT>
    void extract(udho::www::feature::identifier::result& result, const StrT& target) {
        using size_type = typename StrT::size_type;
        auto sep    = target.find('?');
        auto path   = target.substr(0, sep); // check extension .html or .json or .xml or nothing etc...
        result.path(path);
        extract_path(result, path);
        if(sep >= target.size()) return;

        auto query  = target.substr(sep+1);
        size_type p = sep;
        size_type last = 0;
        while(last < query.size()) {
            p = query.find('&', last);

            auto param = query.substr(last, (p-last));
            auto kvsep = param.find('=');
            auto key   = param.substr(0, kvsep);
            std::string kstr{key};
            kstr = udho::utils::decode::url(kstr);
            if(kstr.size() > 0) {
                if(kvsep >= param.size()) {
                    result.add(std::move(kstr), std::string{});
                } else {
                    auto value = param.substr(kvsep+1);
                    if(value.size() > 0) {
                        std::string vstr{value};
                        vstr = udho::utils::decode::url(vstr);
                        result.add(std::move(kstr), std::move(vstr));
                    } else {
                        // but why was there an = if it intended to be empty?
                        // is it a valid url in the first place?
                        // TODO should I throw an exception or silently accept it gracefully?
                        result.add(std::move(kstr), std::string{});
                    }
                }
            } else {
                auto value = param.substr(kvsep+1);
                if(value.size() > 0) {
                    std::string vstr{value};
                    vstr = udho::utils::decode::url(vstr);
                    result.add(std::string{}, std::move(vstr));
                } else {
                    result.add(std::string{}, std::string{});
                }
            }
            last = p < query.size() ? p+1 : p;
        }
    }

    template <typename StrT>
    void extract_path(udho::www::feature::identifier::result& result, const StrT& path) {
        auto dot_pos = path.rfind('.');
        if(dot_pos >= path.size()) {
            result.resource(std::move(path));
        } else {
            auto resource = path.substr(0, dot_pos);
            result.resource(std::move(resource));
            if(dot_pos < path.size()) {
                auto extension = path.substr(dot_pos+1);
                result.extension(std::move(extension));
            }
        }
    }
};

template <typename Policy>
struct navigator{
    using features = udho::manifold::features<udho::www::feature::identifier>;
    using params   = udho::manifold::params<>;

    static constexpr const udho::utils::string_view name = "navigator";

    template <typename... Args>
    navigator(Args&&... args): _policy(std::forward<Args>(args)...){}

    udho::www::feature::identifier::result operator()(const udho::net::types::headers::request& request) {
        return _policy(request);
    }

private:
    Policy _policy;
};

namespace navigators{
using pretty = udho::www::components::navigator<udho::www::components::pretty_url_policy>;
}

} // components
} // www

namespace manifold{

template <typename Policy>
struct facet<udho::www::components::navigator<Policy>, udho::www::feature::identifier>{
    using component_type  = udho::www::components::navigator<Policy>;
    using config_type     = udho::manifold::config<component_type>;

    facet(component_type& component, const config_type& config, std::size_t id): _component(component), _config(config) {}

    template <typename... Components, typename NextT, typename Stream>
    void eval(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream&) const {
        using result = udho::www::feature::identifier::result;
        static_assert(std::is_same_v<udho::www::feature::header_reader::result, udho::net::types::headers::request>);

        const udho::net::types::headers::request& request = journal.template first_of<udho::www::feature::header_reader>();
        try{
            result res = _component(request);
            next.pass(std::move(res));
        } catch(...){
            next.fail(udho::exceptions::captured::propagate());
        }
    }


    template <typename... Components, typename NextT, typename Stream>
    void operator()(const udho::manifold::journal<Components...>& journal, NextT&& next, Stream& stream) const {
        std::cout << "-> facet<components::navigator<Policy>, udho::www::feature::identifier>::operator()(...)" << std::endl;
        eval(journal, std::forward<NextT>(next), stream);
    }
private:
    component_type& _component;
    const config_type& _config;
};

template <typename Policy, typename JournalT>
struct accessor<udho::www::components::navigator<Policy>, JournalT>: basic_accessor<udho::www::components::navigator<Policy>, JournalT>{
    using basic_accessor_type   = basic_accessor<udho::www::components::navigator<Policy>, JournalT>;
    using component_type        = udho::www::components::navigator<Policy>;
    using config_type           = udho::manifold::config<component_type>;
    using journal_type          = JournalT;

    using basic_accessor_type::basic_accessor_type;

    const std::string& resource() const {
        return basic_accessor_type::journal().template at<udho::www::feature::identifier>()->resource();
    }
};

} // manifold

} // udho

#endif // UDHO_WWW_COMPONENTS_NAVIGATOR_H
