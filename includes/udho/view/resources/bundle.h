/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_VIEW_RESOURCES_BUNDLE_H
#define UDHO_VIEW_RESOURCES_BUNDLE_H

#include <string>
#include <stdexcept>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <udho/view/resources/resource.h>
#include <udho/view/sections.h>

namespace udho{
namespace view{
namespace resources{

namespace view{

template <typename BridgeT>
struct proxy;

struct results{
    template <typename BridgeT>
    friend struct proxy;

    results() = delete;

    inline std::string name() const { return _name; }
    inline std::size_t size() const { return _size; }
    inline std::string type() const { return _type; }
    inline const std::string& str() const { return _output; }
    inline std::string::const_iterator begin() const { return _output.begin(); }
    inline std::string::const_iterator end() const { return _output.end(); }

    private:
        inline explicit results(const std::string& name): _name(name) {}
        inline void type(const std::string& t) { _type = t; }
        inline void size(const std::size_t& s) { _size = s; }
        inline std::string& output() { return _output; }
    private:
        std::string _name;
        std::string _output;
        std::string _type;
        std::size_t _size;
};

template <typename BridgeT>
struct proxy{
    using bridge_type = BridgeT;

    proxy(const std::string& name, bridge_type& bridge): _name(name), _bridge(bridge) {}

    inline std::string name() const { return _name; }

    template <typename T>
    view::results eval(T&& data){
        results res(_name);
        std::size_t size = _bridge.exec(_name, std::forward<T>(data), res.output());
        res.size(size);
        return res;
    }

    template <typename T>
    view::results operator()(T&& data){
        return eval(std::forward<T>(data));
    }

    private:
        std::string  _name;
        bridge_type& _bridge;
};

}

template <typename BridgeT>
struct bundle;

namespace detail {
    template<typename BundleT, typename ProxyT, typename TypeIndex, typename CompositeIndex, resource::resource_type type>
    struct bundle_proxy;

    template<typename BridgeT, typename ProxyT, typename TypeIndex, typename CompositeIndex, resource::resource_type type>
    struct bundle_proxy<bundle<BridgeT>, ProxyT, TypeIndex, CompositeIndex, type> {
        using bundle_type        = bundle<BridgeT>;
        using proxy_type         = ProxyT;
        using type_iterator      = typename TypeIndex::const_iterator;
        using composite_iterator = typename CompositeIndex::const_iterator;
        using size_type          = typename TypeIndex::size_type;

        bundle_proxy(bundle_type& bundle) : _bundle(bundle) { }
        bundle_proxy(const bundle_proxy& other) = default;

        type_iterator begin() const  {
            auto range = _bundle.by_type().equal_range(type);
            return range.first;
        }
        type_iterator end() const    {
            auto range = _bundle.by_type().equal_range(type);
            return range.second;
        }
        size_type count() const      {
            auto range = _bundle.by_type().equal_range(type);
            if (range.first == range.second) return 0;
            return std::distance(range.first, range.second);
        }

        composite_iterator find(const std::string& name) const {
            return _bundle.by_composite().find(boost::make_tuple(type, name));
        }

        typename proxy_type::result_type operator[](const std::string& name) const {
            composite_iterator it = find(name);
            if (it != _bundle.by_composite().end()) {
                proxy_type proxy{*it, _bundle};
                return proxy();
            } else {
                throw std::out_of_range("Resource with name '" + name + "' not found");
            }
        }
        template <typename T>
        view::results operator()(const std::string& name, T&& arg){
            typename proxy_type::result_type v = operator[](name);
            return v(std::forward<T>(arg));
        }

    private:
        bundle_type& _bundle;
    };

    template<typename BundleT>
    struct view_proxy{
        using bridge_type = typename BundleT::bridge_type;
        using result_type = view::proxy<bridge_type>;

        view_proxy(const resource& res, BundleT& bundle): _res(res), _bridge(bundle.bridge()) {}
        result_type operator()() { return result_type{_res.name(), _bridge}; }

        private:
            const resource& _res;
            bridge_type&    _bridge;
    };
}

template <typename BridgeT>
struct bundle{
    using bridge_type = BridgeT;
    using self_type   = bundle<bridge_type>;
    using view_proxy  = view::proxy<bridge_type>;

    struct tags{
        struct type{};
        struct name{};
        struct composite{};
    };

    using resource_set = boost::multi_index_container<
        resource,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<typename tags::composite>,
                boost::multi_index::composite_key<
                    resource,
                    boost::multi_index::const_mem_fun<resource, resource::resource_type, &resource::type>,
                    boost::multi_index::const_mem_fun<resource, std::string, &resource::name>
                >
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::name>,
                boost::multi_index::const_mem_fun<resource, std::string, &resource::name>
            >,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<typename tags::type>,
                boost::multi_index::const_mem_fun<resource, resource::resource_type, &resource::type>
            >
        >
    >;

    using type_index               = typename resource_set::template index<typename tags::type>::type;
    using name_index               = typename resource_set::template index<typename tags::name>::type;
    using composite_index          = typename resource_set::template index<typename tags::composite>::type;

    using type_iterator            = typename resource_set::template index<typename tags::type>::type::iterator;
    using type_const_iterator      = typename resource_set::template index<typename tags::type>::type::const_iterator;
    using name_iterator            = typename resource_set::template index<typename tags::name>::type::iterator;
    using name_const_iterator      = typename resource_set::template index<typename tags::name>::type::const_iterator;
    using composite_iterator       = typename resource_set::template index<typename tags::composite>::type::iterator;
    using composite_const_iterator = typename resource_set::template index<typename tags::composite>::type::const_iterator;
    using size_type                = typename resource_set::size_type;

    template <resource::resource_type type>
    using bundle_proxy = detail::bundle_proxy<
        self_type,
        detail::view_proxy<self_type>,
        type_index,
        composite_index,
        type
    >;

    using bundle_proxy_view = bundle_proxy<resource::resource_type::view>;

    friend bundle_proxy_view;

    explicit bundle(bridge_type& bridge, const std::string& prefix): _bridge(bridge), _prefix(prefix), views(*this) {}
    bundle(const bundle&) = delete;

    void add(const resource& res) {
        _resources.insert(res);
        if(res.is_view()){
            prepare_view(res);
        }
    }

    template <typename Tag>
    typename resource_set::template index<Tag>::type& by() { return _resources.template get<Tag>(); }
    typename resource_set::template index<typename tags::type>::type& by_type() { return by<typename tags::type>(); }
    typename resource_set::template index<typename tags::name>::type& by_name() { return by<typename tags::name>(); }
    typename resource_set::template index<typename tags::composite>::type& by_composite() { return by<typename tags::composite>(); }

    std::string prefix() const { return _prefix; }

    friend bundle& operator<<(bundle& b, const resource& res){
        b.add(res);
        return b;
    }

    view_proxy view(const std::string& name){
        auto it = by_composite().find(boost::make_tuple(resource::resource_type::view, name));
        if (it != by_composite().end()) {
            return view_proxy{name, _bridge};
        } else {
            throw std::out_of_range("Resource with name '" + name + "' not found");
        }
    }

    bridge_type& bridge() { return _bridge; }

    private:
        bool prepare_view(const resource& res){
            typename bridge_type::script_type script = _bridge.create(res.name());
            udho::view::sections::parser parser;
            parser.parse(res.path().string(), script);
            script.finish();
            return _bridge.compile(script);
        }
    private:
        bridge_type& _bridge;
        std::string  _prefix;
        resource_set _resources;
    public:
        bundle_proxy_view views;
        // detail::bundle_asset_proxy<bridge_type> assets;
};

}
}
}


#endif // UDHO_VIEW_RESOURCES_BUNDLE_H
