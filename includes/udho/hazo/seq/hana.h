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

#ifndef UDHO_HAZO_SEQ_HANA_H
#define UDHO_HAZO_SEQ_HANA_H

#include <boost/hana.hpp>
#include <udho/hazo/seq/tag.h>
#include <udho/hazo/detail/extraction_helper.h>

namespace boost {
namespace hana {
    template <typename Policy, int N>
    struct at_impl<udho::util::hazo::udho_hazo_seq_tag<Policy, N>> {
        template <typename SeqT, typename I>
        static constexpr decltype(auto) apply(SeqT&& xs, I const&) {
            return udho::util::hazo::extraction_helper<Policy, SeqT, I::value>::apply(std::forward<SeqT>(xs));
        }
    };
    
    template <typename Policy, int N>
    struct drop_front_impl<udho::util::hazo::udho_hazo_seq_tag<Policy, N>> {
        template <typename Xs, typename I>
        static constexpr decltype(auto) apply(Xs&& xs, I const&) {
            return xs.template tail_at<I::value>();
        }
    };

    template <typename Policy, int N>
    struct is_empty_impl<udho::util::hazo::udho_hazo_seq_tag<Policy, N>> {
        template <typename Xs>
        static constexpr auto apply(Xs const& xs) {
            return xs.depth == 1;
        }
    };
    
    template <typename Policy, int N>
    struct unpack_impl<udho::util::hazo::udho_hazo_seq_tag<Policy, N>> {
        template <typename Xs, typename F>
        static constexpr decltype(auto) apply(Xs&& xs, F&& f) {
            return std::forward<Xs>(xs).unpack(std::forward<F>(f));
        }
    };
    
    template <typename Policy, int N>
    struct make_impl<udho::util::hazo::udho_hazo_seq_tag<Policy, N>> {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...args) {
            return udho::util::hazo::basic_seq<Policy, Args...>(std::forward<Args>(args)...);
        }
    };
    
    template <typename Policy, int N>
    struct length_impl<udho::util::hazo::udho_hazo_seq_tag<Policy, N>> {
        template <typename Xs>
        static constexpr auto apply(Xs const&) {
            return Xs::depth +1;
        }
    };
        
    template <typename Policy, int N>
    struct Sequence<udho::util::hazo::udho_hazo_seq_tag<Policy, N>> : std::true_type { };
}
}

#endif // UDHO_HAZO_SEQ_HANA_H
