/*
 * Copyright (c) 2020, <copyright holder> <email>
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
 * THIS SOFTWARE IS PROVIDED BY <copyright holder> <email> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> <email> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_FOLDING_MAP_HANA_H
#define UDHO_FOLDING_MAP_HANA_H

#include <boost/hana.hpp>
#include <udho/folding/map/tag.h>
#include <udho/folding/map/fwd.h>

namespace boost {
namespace hana {
    template <typename Policy, typename... X>
    struct accessors_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename K>
        struct get_member {
            template <typename MapT>
            constexpr decltype(auto) operator()(MapT&& map) const {
                return map.template at<K::value>()._capsule.key();
            }
        };

        static decltype(auto) apply() {
            udho::util::folding::access_helper<Policy, typename udho::util::folding::map<Policy, X...>::node_type, typename udho::util::folding::build_indices<sizeof...(X)>::indices_type> helper;
            return helper.apply();
        }
    };

    template <typename Policy, typename... X>
    struct drop_front_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename Xs, typename N>
        static constexpr decltype(auto) apply(Xs&& xs, N const&) {
            return xs.template tail_at<N::value>();
        }
    };

    template <typename Policy, typename... X>
    struct is_empty_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename Xs>
        static constexpr auto apply(Xs const& xs) {
            return xs.depth == 1;
        }
    };
    
    template <typename Policy, typename... X>
    struct unpack_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return xs.unpack(std::forward<F>(f));
        }
    };
    
    template <typename Policy, typename... X>
    struct make_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...args) {
            return udho::util::folding::map<Policy, Args...>(std::forward<Args>(args)...);
        }
    };
    
    template <typename Policy, typename... X>
    struct length_impl<udho::util::folding::udho_folding_map_tag<Policy, X...>> {
        template <typename ...Xn>
        static constexpr auto apply(udho::util::folding::map<Xn...> const&) {
            return hana::size_t<sizeof...(Xn)>{};
        }
    };
}
}


#endif // UDHO_FOLDING_MAP_HANA_H
