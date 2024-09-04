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

#ifndef UDHO_VIEW_DATA_TYPE_H
#define UDHO_VIEW_DATA_TYPE_H

#include <type_traits>

namespace udho{
namespace view{
namespace data{

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
 * @code
 * struct education {
 *     std::string course;
 *     std::string university;
 *
 *     education() = default;
 *     education(const std::string& c, const std::string& u) : course(c), university(u) {}
 *
 *     friend auto prototype(udho::view::data::type<education>) {
 *         using namespace udho::view::data;
 *         return assoc("education"),
 *             mvar("course", &education::course),
 *             mvar("university", &education::university);
 *     }
 * };
 *
 * struct person {
 *     std::string first_name;
 *     std::string last_name;
 *     double age;
 *
 *     address() = default;
 *     address(const std::string loc): locality(loc) {}
 *
 *     friend auto prototype(udho::view::data::type<person>) {
 *         using namespace udho::view::data;
 *         return assoc("person"),
 *             mvar("first_name", &person::first_name),
 *             mvar("last_name", &person::last_name),
 *             cvar("age", &person::age);
 *     }
 * };
 *
 * struct student : person {
 *     std::vector<education> courses;
 *     double _debt;
 *
 *     student() = default;
 *     student(const student&) = delete;
 *
 *     inline double debt() const { return _debt; }
 *     inline void set_debt(const double& v) {
 *         _debt = v > 100 ? 100 : v;
 *     }
 *
 *     std::string print() const {
 *         std::stringstream stream;
 *         stream << "Name: " << first_name << " " << last_name
 *                << ", Age: " << age << ", Debt: " << _debt << std::endl;
 *         for (const education& e : courses) {
 *             stream << e.course << " at " << e.university << std::endl;
 *         }
 *         return stream.str();
 *     }
 *     double add(std::uint32_t a, double b, float c, int d){ return a+b+c+d; }
 *
 *     friend auto prototype(udho::view::data::type<student>) {
 *         using namespace udho::view::data;
 *         return assoc("student"),
 *             prototype(type<person>()),
 *             fvar("debt",     &student::debt, &student::set_debt),
 *             mvar("courses",  &student::courses),
 *             func("print",    &student::print),
 *             func("add",      &student::add);
 *     }
 * };
 * @endcode
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

}
}
}

#endif // UDHO_VIEW_DATA_TYPE_H
