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

#ifndef UDHO_URL_ACTION_H
#define UDHO_URL_ACTION_H

#include <udho/url/fwd.h>
#include <udho/url/detail/function.h>
#include <udho/hazo/string/basic.h>
#include <udho/url/pattern.h>
#include <boost/hana/concat.hpp>

namespace udho{
namespace url{

namespace detail{

    // template <typename T, typename HeadT, std::size_t... I>
    // T fill_in(HeadT&& head, std::index_sequence<I...>){
    //     constexpr std::size_t head_size = std::tuple_size<HeadT>::value;
    //     auto rest = std::make_tuple(std::tuple_element_t<I+head_size, T>()...);
    //     return std::tuple_cat(std::move(head), std::move(rest));
    // }
    //
    // template <typename T, typename... Args>
    // T fill(Args&&... args){
    //     return fill_in<T>(std::make_tuple(std::move(args)...), std::make_index_sequence<std::tuple_size<T>::value - sizeof...(args)>());
    // }
    //
    // template <typename T>
    // T fill(){return T();}

    template <typename T, std::size_t After, std::size_t... I>
    auto rest(std::index_sequence<I...>){
        return std::make_tuple(std::tuple_element_t<I+After, T>()...);
    }

    template <typename T, std::size_t After>
    auto rest(){
        return rest<T, After>(std::make_index_sequence<std::tuple_size<T>::value - After>());
    }

}

/**
 * @brief The basic_slot class is a template for creating function slots that can be invoked
 *        using string arguments.
 *
 * This template class allows functions to be called with string arguments that are automatically
 * converted to the function's required argument types. The class supports both direct invocation
 * with a tuple of arguments and invocation with iterators that point to the beginning and end
 * of a range of string arguments.
 *
 * @note The `basic_slot` class is not intended to be instantiated directly; instead, instances
 * are typically created using the `slot` function.
 *
 * @tparam F The function type this slot will hold.
 * @tparam CharT The character type of the string key.
 * @tparam C Variadic template arguments representing character literals that form the string key.
 * @see slot
 */
template <typename F, typename CharT, CharT... C>
struct basic_slot<F, udho::hazo::string::str<CharT, C...>>{
    using function_type          = F;                                               ///< The type of the function stored in the slot.
    using key_type               = udho::hazo::string::str<CharT, C...>;            ///< The type used for the key, represented by a compile-time string.
    using return_type            = typename function_type::return_type;             ///< The return type of the function.
    using arguments_type         = typename function_type::arguments_type;          ///< The type representing the function's argument list.
    using decayed_arguments_type = typename function_type::decayed_arguments_type;  ///< The decayed types of the function's arguments.

    /**
     * @brief Helper template to determine if a type T is valid for the function's arguments.
     * @tparam T The type to check against the function's argument requirements.
     */
    template <typename T>
    using valid_args             = typename function_type::template valid_args<T>;

    enum {
        args = function_type::args      ///< Number of arguments the function takes.
    };

    /**
     * @brief Retrieves the type of the N-th argument of the function.
     * @tparam N The index of the argument to retrieve.
     */
    template <int N>
    using arg = typename function_type::template arg<N>;

    /**
     * @brief Retrieves the decayed type of the N-th argument of the function.
     * @tparam N The index of the argument to retrieve.
     */
    template <int N>
    using decayed_arg = typename function_type::template decayed_arg<N>;

    /**
     * @brief Constructs a basic_slot with a function object.
     * @param f Function object to be moved into the slot.
     */
    basic_slot(function_type&& f): _fnc(std::move(f)) {}

    /**
     * @brief Calls the stored function with the given arguments if they are valid.
     *
     * This overload ensures that the types of the passed arguments match the types expected by the function.
     *
     * @tparam T The type of the arguments being passed.
     * @param args Arguments to forward to the function.
     * @return The result of the function call.
     */
    template <typename T, typename std::enable_if<std::is_same<valid_args<T>, T>::value>::type* = nullptr>
    return_type operator()(T&& args) const { return _fnc(std::move(args)); }

    /**
     * @brief Prepares the arguments from a range of strings to be passed to the function.
     * @tparam IteratorT The type of the iterators.
     * @param begin Iterator pointing to the beginning of the range.
     * @param end Iterator pointing to the end of the range.
     * @return A tuple of decayed arguments ready to be passed to the function.
     */
    template <typename IteratorT>
    decayed_arguments_type prepare(IteratorT begin, IteratorT end){ return _fnc.prepare(begin, end); }

    /**
     * @brief Invokes the stored function using a range of string arguments.
     * @tparam IteratorT The type of the iterators.
     * @param begin Iterator pointing to the beginning of the range.
     * @param end Iterator pointing to the end of the range.
     * @return The result of the function call.
     */
    template <typename IteratorT>
    return_type operator()(IteratorT begin, IteratorT end) { return _fnc(begin, end); }

    /**
     * @brief Retrieves the compile-time key of the slot.
     * @return An instance of the key_type, representing the compile-time string.
     */
    static constexpr key_type key() { return key_type{}; }

    /**
     * @brief Retrieves the symbol name of the function, demangled.
     * @return A string representing the symbol name of the function.
     */
    std::string symbol() const { return _fnc.symbol_name(); }

    private:
        function_type _fnc;
};

/**
 * @brief A template struct that extends basic_slot with URL pattern matching capabilities.
 *
 * This structure represents an action that is associated with a specific URL pattern. It combines a function,
 * typically representing a web endpoint or handler, with a matching pattern. This allows the function to be invoked
 * only when the incoming URL matches the specified pattern.
 *
 * @tparam F The function type associated with the action.
 * @tparam CharT Character type for the compile-time string.
 * @tparam C Characters constituting the compile-time string.
 * @tparam MatchT Type of the matching mechanism used to check URL patterns.
 *
 * @example
 * auto action = udho::url::slot("example"_h, &example_function) << udho::url::regx(udho::url::verb::get, "/example/(\\d+)", "/example/{}");
 * // Here, `action` is a basic_action configured to match URLs to `example_function` based on the regex pattern provided.
 */
template <typename F, typename CharT, CharT... C, typename MatchT>
struct basic_action<F, udho::hazo::string::str<CharT, C...>, MatchT>: basic_slot<F, udho::hazo::string::str<CharT, C...>>{
    using slot_type              = basic_slot<F, udho::hazo::string::str<CharT, C...>>;
    using function_type          = F;
    using key_type               = udho::hazo::string::str<CharT, C...>;
    using return_type            = typename function_type::return_type;
    using arguments_type         = typename function_type::arguments_type;
    using decayed_arguments_type = typename function_type::decayed_arguments_type;
    using match_type             = MatchT;
    using pattern_type           = typename match_type::pattern_type;

    /**
     * Constructs a basic_action with a function and a URL pattern match.
     * @param f The function to be associated with the URL pattern.
     * @param match The pattern matching mechanism.
     */
    basic_action(function_type&& f, const match_type& match): slot_type(std::move(f)), _match(match) {}
    /**
     * Constructs a basic_action by moving from a basic_slot and a URL pattern match.
     * @param slot The basic_slot to move.
     * @param match The pattern matching mechanism.
     */
    basic_action(slot_type&& slot, match_type&& match): slot_type(std::move(slot)), _match(match) {}
    using slot_type::operator();

    /**
     * @brief checks whether this action matches with the pattern provided
     * @tparam Ch The character type of the URL string.
     * @param subject The URL to match.
     * @return True if the URL matches the pattern, otherwise false.
     */
    template <typename Ch>
    bool find(const std::basic_string<Ch>& subject) const{
        bool found = _match.find(subject);
        return found;
    }
    /**
     * @brief invokes the function with the captured arguments if this action matches with the pattern provided.
     * @tparam Ch The character type of the URL string.
     * @tparam Args Types of arguments to forward to the function if matched.
     * @param subject The URL to match and potentially invoke the action upon.
     * @param args Arguments to forward to the function.
     * @return True if the URL matches the pattern and the function is invoked, otherwise false.
     */
    template <typename Ch, typename... Args>
    bool invoke(const std::basic_string<Ch>& subject, Args&&... args) const{
        // decayed_arguments_type tuple = detail::fill<decayed_arguments_type>(std::forward<Args>(args)...);
        auto rest = detail::rest<decayed_arguments_type, sizeof...(args)>();
        bool found = _match.find(subject, rest);
        if(found){
            decayed_arguments_type tuple = std::tuple_cat(std::make_tuple(std::move(args)...), rest);
            slot_type::operator()(std::move(tuple));
        }
        return found;
    }

    /**
     * Generates a URL using the associated pattern and provided arguments.
     * @tparam X Argument types used to generate the URL.
     * @param x Arguments to format into the URL pattern.
     * @return A URL string generated by filling the pattern with provided arguments.
     */
    template <typename... X>
    std::string operator()(X&&... x) const {
        return fill(std::make_tuple(std::move(x)...));
    }

    /**
     * Fills the URL pattern with the given arguments, provided in a tuple.
     * @tparam X Argument types wrapped in a tuple.
     * @param args Arguments wrapped in a tuple to format into the URL pattern.
     * @return A URL string generated by filling the pattern.
     */
    template <typename... X>
    std::string fill(std::tuple<X...>&& args) const { return _match.replace(std::move(args)); }

    /**
     * Accessor for the underlying pattern matching mechanism.
     * @return A constant reference to the match object.
     */
    const match_type& match() const { return _match; }
    private:
        match_type    _match;
};

/**
 * @brief Creates a basic_action by associating a basic_slot with a URL pattern match using the left shift operator.
 *
 * @tparam F Function type.
 * @tparam CharT Character type for the compile-time string.
 * @tparam C Characters of the compile-time string.
 * @tparam MatchT Type of the matching mechanism.
 * @param slot The slot to associate with a match.
 * @param match The pattern matching mechanism.
 * @return A basic_action configured with the slot and match.
 */
template <typename F, typename CharT, CharT... C, typename MatchT>
basic_action<F, udho::hazo::string::str<CharT, C...>, MatchT> operator<<(basic_slot<F, udho::hazo::string::str<CharT, C...>>&& slot, MatchT&& match){
    return basic_action<F, udho::hazo::string::str<CharT, C...>, MatchT>(std::move(slot), std::move(match));
}

/**
 * @brief Creates a basic_action by associating a basic_slot with a URL pattern match using the right shift operator.
 *
 * @tparam F Function type.
 * @tparam CharT Character type for the compile-time string.
 * @tparam C Characters of the compile-time string.
 * @tparam MatchT Type of the matching mechanism.
 * @param match The pattern matching mechanism.
 * @param slot The slot to associate with a match.
 * @return A basic_action configured with the slot and match.
 */
template <typename F, typename CharT, CharT... C, typename MatchT>
basic_action<F, udho::hazo::string::str<CharT, C...>, MatchT> operator>>(MatchT&& match, basic_slot<F, udho::hazo::string::str<CharT, C...>>&& slot){
    return basic_action<F, udho::hazo::string::str<CharT, C...>, MatchT>(std::move(slot), std::move(match));
}

/**
 * @brief Creates a slot for a free function, binding a URL pattern to a callback.
 *
 * This template function takes a unique hash identifier and a free function, encapsulating
 * the function into a callable suitable for use in URL routing. This is intended for simple
 * functions that do not require access to an object's state.
 *
 * @tparam FunctionT Type of the function to be encapsulated.
 * @tparam CharT Character type for the compile-time hash string.
 * @tparam C Characters forming the compile-time hash string.
 * @param identifier A compile-time hash string used as a unique identifier for the slot.
 * @param function A rvalue reference to the free function to be used as the callback.
 * @return Returns a `basic_slot` instance encapsulating the provided function.
 * @see basic_slot
 * @example
 * void f0() {
 *     std::cout << "Home page" << std::endl;
 * }
 * auto router = udho::url::router(
 *     udho::url::slot("f0"_h, &f0) << udho::url::home(udho::url::verb::get)
 * );
 */
template <typename FunctionT, typename CharT, CharT... C>
basic_slot<
    udho::url::detail::encapsulate_function<FunctionT>,
    udho::hazo::string::str<CharT, C...>
> slot(udho::hazo::string::str<CharT, C...>, FunctionT&& function){
    using slot_type = basic_slot<udho::url::detail::encapsulate_function<FunctionT>, udho::hazo::string::str<CharT, C...>>;
    return slot_type(detail::encapsulate_function<FunctionT>(std::move(function)));
}

/**
 * @brief Creates a slot for a member function, binding a URL pattern to a member function callback.
 *
 * This template function takes a unique hash identifier, a member function, and a pointer to
 * the object on which the member function should be invoked. It encapsulates the member function
 * into a callable that is suitable for use in URL routing. This overload is useful for member functions
 * that need to access or modify the state of their object.
 *
 * @tparam FunctionT Type of the member function to be encapsulated.
 * @tparam CharT Character type for the compile-time hash string.
 * @tparam C Characters forming the compile-time hash string.
 * @param identifier A compile-time hash string used as a unique identifier for the slot.
 * @param function A rvalue reference to the member function to be used as the callback.
 * @param that Pointer to the object on which the member function will be called.
 * @return Returns a `basic_slot` instance encapsulating the provided member function and object pointer.
 * @see basic_slot
 * @example
 * struct X {
 *     void f0() {
 *         std::cout << "X's f0" << std::endl;
 *     }
 * };
 * X x;
 * auto router = udho::url::router(
 *     udho::url::slot("xf0"_h, &X::f0, &x) << udho::url::fixed(udho::url::verb::get, "/x/f0", "/x/f0")
 * );
 */
template <typename FunctionT, typename CharT, CharT... C>
basic_slot<
    udho::url::detail::encapsulate_mem_function<FunctionT>,
    udho::hazo::string::str<CharT, C...>
> slot(udho::hazo::string::str<CharT, C...>, FunctionT&& function, typename detail::function_signature_<FunctionT>::object_type* that){
    using slot_type = basic_slot<udho::url::detail::encapsulate_mem_function<FunctionT>, udho::hazo::string::str<CharT, C...>>;
    return slot_type(detail::encapsulate_mem_function<FunctionT>(std::move(function), that));
}


template <typename FunctionT, typename MatchT, typename CharT, CharT... C>
basic_action<udho::url::detail::encapsulate_function<FunctionT>, udho::hazo::string::str<CharT, C...>, MatchT>
action(FunctionT&& function, udho::hazo::string::str<CharT, C...>, const MatchT& match){
    using action_type = basic_action<udho::url::detail::encapsulate_function<FunctionT>, udho::hazo::string::str<CharT, C...>, MatchT>;
    return action_type(detail::encapsulate_function<FunctionT>(std::move(function)), match);
}

template <typename FunctionT, typename MatchT, typename CharT, CharT... C>
basic_action<udho::url::detail::encapsulate_mem_function<FunctionT>, udho::hazo::string::str<CharT, C...>, MatchT>
action(FunctionT&& function, typename detail::function_signature_<FunctionT>::object_type* that, udho::hazo::string::str<CharT, C...>, const MatchT& match){
    using action_type = basic_action<udho::url::detail::encapsulate_mem_function<FunctionT>, udho::hazo::string::str<CharT, C...>, MatchT>;
    return action_type(detail::encapsulate_mem_function<FunctionT>(std::move(function), that), match);
}


}
}

#endif // UDHO_URL_ACTION_H
