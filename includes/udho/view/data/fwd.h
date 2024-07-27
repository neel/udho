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

#ifndef UDHO_VIEW_DATA_FWD_H
#define UDHO_VIEW_DATA_FWD_H

#include <nlohmann/json.hpp>

namespace udho{
namespace view{
namespace data{

template <typename PolicyT, typename KeyT, typename... X>
struct nvp;

template <class ClassT>
nlohmann::json to_json(const ClassT& data);

template <class ClassT>
void from_json(ClassT& data, const nlohmann::json& json);

template <typename Class>
struct type {};


/**
 * @brief Default prototype function template used when specific type overloads are absent.
 *
 * This function serves as a default implementation of the prototype function for any class type that is not explicitly overloaded.
 * It triggers a static assertion error to indicate the absence of an appropriate overload when attempting to expose a class as
 * the data object of the view in an MVC framework setup. This is part of integrating C++ classes with foreign languages by
 * defining how class members are accessed and manipulated from the foreign environment.
 *
 * @note The purpose of this function is to ensure compile-time errors when no suitable overload is provided for a specific class type,
 * thereby enforcing the requirement that all classes exposed to the view must have an explicit prototype definition.
 *
 * @tparam ClassT The class type for which the prototype function is to be defined.
 * @param data A type tag representing the ClassT, used for specializing the function for different classes.
 * @return returns an instance of metatype
 *
 * @note This function will compile only if there is a specific overload for the type `ClassT`. If not, it will cause a compile-time error.
 *
 * @example
 * struct info {
 *     std::string name;
 *     double value;
 *     std::uint32_t _x;
 *
 *     inline double x() const { return _x; }
 *     inline void setx(const std::uint32_t& v) { _x = v; }
 *
 *     inline info() {
 *         name = "Hello";
 *         value = 42;
 *         _x = 43;
 *     }
 *
 *     void print() {
 *         std::cout << "name: " << name << " value: " << value << std::endl;
 *     }
 *
 *     // Prototype specialization for 'info' type
 *     friend auto prototype(udho::view::data::type<info>) {
 *         using namespace udho::view::data;
 *         return assoc(
 *             mvar("name", &info::name),
 *             cvar("value", &info::value),
 *             fvar("x", &info::x, &info::setx),
 *             func("print", &info::print)
 *         ).as("info");
 *     }
 * };
 */
template <class ClassT>
auto prototype(udho::view::data::type<ClassT>){
    static_assert("prototype method not overloaded");
}

/**
 * @brief checks whether a prototype overload exists for a given class
 */
template <typename ClassT>
struct has_prototype: std::integral_constant<bool, !std::is_void_v<decltype(prototype(std::declval<type<ClassT>>()))>>{};

template <typename X = void, typename... Xs>
struct associative;

}
}
}

#endif // UDHO_VIEW_DATA_FWD_H
