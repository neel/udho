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

#ifndef UDHO_URL_DETAIL_FUNCTION_H
#define UDHO_URL_DETAIL_FUNCTION_H

#include <utility>
#include <functional>
#include <type_traits>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <udho/url/detail/format.h>

namespace udho{
namespace url{

namespace detail{

    template <typename T, int Index>
    struct cast_optionally{
        template <typename IteratorT>
        static T cast(IteratorT begin, IteratorT end){
            auto distance = std::distance(begin, end);
            if(distance <= Index){
                std::stringstream stream;
                std::copy(begin, end, std::ostream_iterator<std::string>(stream, ","));
                throw std::out_of_range(format("Argument {} cannot be extracted because there are only {} items: [{}]", Index+1, distance, stream.str()));
            }
            IteratorT i = begin;
            std::advance(i, Index);
            std::string argstr = *i;
            try{
                return boost::lexical_cast<T>(argstr);
            }catch(...){
                throw std::invalid_argument(format("Failed to cast argument {} '{}' to expected type", Index+1, argstr));
                return T();
            }
        }
    };

    template <typename TupleT, int Index=std::tuple_size<TupleT>::value-1>
    struct arg_to_tuple: arg_to_tuple<TupleT, Index-1>{
        typedef arg_to_tuple<TupleT, Index-1> base_type;

        template <typename IteratorT>
        static void convert(TupleT& tuple, IteratorT begin, IteratorT end){
            std::get<Index>(tuple) = cast_optionally<typename std::tuple_element<Index, TupleT>::type, Index>::cast(begin, end);
            base_type::convert(tuple, begin, end);
        }
    };

    template <typename TupleT>
    struct arg_to_tuple<TupleT, 0>{
        template <typename IteratorT>
        static void convert(TupleT& tuple, IteratorT begin, IteratorT end){
            std::get<0>(tuple) = cast_optionally<typename std::tuple_element<0, TupleT>::type, 0>::cast(begin, end);
        }
    };

    template <typename TupleT, typename IteratorT>
    void arguments_to_tuple(TupleT& tuple, IteratorT begin, IteratorT end){
        arg_to_tuple<TupleT>::convert(tuple, begin, end);
    }

    /**
     * extract the function signature
     */
    template <typename F>
    struct function_signature_;

    template <typename R, typename... Args>
    struct function_signature_<R (*)(Args...)>{
        typedef R                                 return_type;
        typedef void                              object_type;
        typedef std::tuple<Args...>               arguments_type;
        typedef std::tuple<std::decay_t<Args>...> decayed_arguments_type;
    };

    template <typename R, typename... Args>
    struct function_signature_<R (Args...)>{
        typedef R                                 return_type;
        typedef void                              object_type;
        typedef std::tuple<Args...>               arguments_type;
        typedef std::tuple<std::decay_t<Args>...> decayed_arguments_type;
    };

    template <typename R, typename C, typename... Args>
    struct function_signature_<R (C::*)(Args...)>{
        typedef R                                 return_type;
        typedef std::tuple<Args...>               arguments_type;
        typedef std::tuple<std::decay_t<Args>...> decayed_arguments_type;
        typedef C                                 object_type;
    };

    template <typename R, typename C, typename... Args>
    struct function_signature_<R (C::*)(Args...) const>{
        typedef R                                 return_type;
        typedef std::tuple<Args...>               arguments_type;
        typedef std::tuple<std::decay_t<Args>...> decayed_arguments_type;
        typedef const C                           object_type;
    };

    template <typename F>
    struct encapsulate_mem_function{
        using signature              = function_signature_<F>;
        using return_type            = typename signature::return_type;
        using arguments_type         = typename signature::arguments_type;
        using object_type            = typename signature::object_type;
        using decayed_arguments_type = typename signature::decayed_arguments_type;

        enum {
            args = std::tuple_size<arguments_type>::value
        };
        template <int N>
        using arg = typename std::tuple_element<N, arguments_type>::type;
        template <int N>
        using decayed_arg = typename std::tuple_element<N, decayed_arguments_type>::type;

        encapsulate_mem_function(F f, object_type* that): _f(f), _that(that) {}
        return_type operator()(decayed_arguments_type&& args){
            return std::apply(_f, std::tuple_cat(std::make_tuple(_that), args));
        }
        return_type operator()(arguments_type&& args){
            return std::apply(_f, std::tuple_cat(std::make_tuple(_that), args));
        }
        template <typename IteratorT>
        decayed_arguments_type prepare(IteratorT begin, IteratorT end){
            decayed_arguments_type decayed_args;
            auto nargs = std::distance(begin, end);
            if(nargs != std::tuple_size<decayed_arguments_type>::value){
                throw std::out_of_range("expects " + std::to_string(std::tuple_size<decayed_arguments_type>::value) + " args but called with " + std::to_string(nargs) + " arguments");
            }
            arguments_to_tuple(decayed_args, begin);
            return decayed_args;
        }
        template <typename IteratorT>
        return_type operator()(IteratorT begin, IteratorT end){
            return operator()(prepare(begin, end));
        }
        private:
            F _f;
            object_type* _that;
    };
    template <typename F>
    struct encapsulate_function{
        using signature              = function_signature_<F>;
        using return_type            = typename signature::return_type;
        using arguments_type         = typename signature::arguments_type;
        using decayed_arguments_type = typename signature::decayed_arguments_type;

        enum {
            args = std::tuple_size<arguments_type>::value
        };
        template <int N>
        using arg = typename std::tuple_element<N, arguments_type>::type;
        template <int N>
        using decayed_arg = typename std::tuple_element<N, decayed_arguments_type>::type;

        encapsulate_function(F f): _f(f) {}
        return_type operator()(decayed_arguments_type&& args){
            return std::apply(_f, args);
        }
        return_type operator()(arguments_type&& args){
            return std::apply(_f, args);
        }
        template <typename IteratorT>
        decayed_arguments_type prepare(IteratorT begin, IteratorT end){
            decayed_arguments_type decayed_args;
            auto nargs = std::distance(begin, end);
            if(nargs != std::tuple_size<decayed_arguments_type>::value){
                throw std::out_of_range("expects " + std::to_string(std::tuple_size<decayed_arguments_type>::value) + " args but called with " + std::to_string(nargs) + " arguments");
            }
            arguments_to_tuple(decayed_args, begin, end);
            return decayed_args;
        }
        template <typename IteratorT>
        return_type operator()(IteratorT begin, IteratorT end){
            return operator()(prepare(begin, end));
        }
        private:
            F _f;
    };

    template <typename F>
    function_signature_<F> function_signature(const F&){
        return function_signature_<F>{};
    }

    template <typename F>
    function_signature_<F> function_signature(const F&, typename function_signature_<F>::object_type*){
        return function_signature_<F>{};
    }

}

}
}

#endif // UDHO_URL_DETAIL_FUNCTION_H
