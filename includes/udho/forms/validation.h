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

#ifndef UDHO_FORMS_VALIDATION_H
#define UDHO_FORMS_VALIDATION_H

/** @file
 * @brief Bind parsed form data to field validation and result accumulation.
 */

#include <type_traits>
#include <utility>
#include <udho/forms/accumulated.h>
#include <udho/forms/codec.h>

namespace udho{
namespace forms{

/**
 * @brief Validate fields against one parsed form and collect their display states.
 *
 * Each add() or operator>>() fetches an ordinary field or runs a constrained
 * field's validators, then copies its state into accumulated. Constructing this
 * object alone does not validate any fields. The underlying form_data must
 * outlive this object; only the lightweight form_view is stored by value.
 * @tparam FormT The current HTTP form_data or form_view type.
 */
template <typename FormT>
struct validated: accumulated{
    static_assert(detail::is_form_v<FormT>, "validated expects form_data or form_view");
    /// @brief Parsed storage or lookup-view type supplied by the caller.
    using form_type = FormT;
    /// @brief Concrete result type returned by fluent additions.
    using self_type = validated<FormT>;

    /// @brief Start an empty result that borrows the supplied parsed form.
    explicit validated(const form_type& form): _form(detail::view(form)){}

    /// @brief Run a field's constraint chain and add its resulting display state.
    template <typename ValidatorT, typename TailT>
    self_type& add(detail::constrained_field<ValidatorT, TailT>& field){
        field.validate(_form);
        accumulated::add(field);
        return *this;
    }
    /// @brief Fetch an ordinary field and add its presence and parsing result.
    template <typename T, bool Required>
    self_type& add(field<T, Required>& field){
        field.fetch(_form);
        accumulated::add(field);
        return *this;
    }

    /// @brief Expose inherited summary properties to Udho views and serializers.
    friend auto metatype(udho::view::data::type<self_type>){
        using namespace udho::view::data;
        return assoc("validated_form"), metatype(type<accumulated>{});
    }

    private:
        // Own the lightweight view so validate(form_view(data)) is also safe.
        // The underlying form_data must outlive this validation object.
        udho::net::protocols::detail::form_view _form;
};

/// @brief Create an empty validation result; add fields with add() or operator>>().
template <typename FormT, std::enable_if_t<detail::is_form_v<FormT>, int> = 0>
validated<FormT> validate(const FormT& form){
    return validated<FormT>(form);
}

/// @brief Reject temporary parsed data that would leave the stored view dangling.
validated<detail::form_data> validate(const detail::form_data&&) = delete;

/// @brief Validate a constrained field, collect its state, and allow chaining.
template <typename FormT, typename ValidatorT, typename FieldT>
validated<FormT>& operator>>(validated<FormT>& form, detail::constrained_field<ValidatorT, FieldT>& field){
    return form.add(field);
}

/// @brief Fetch an ordinary field, collect its state, and allow chaining.
template <typename FormT, typename T, bool Required>
validated<FormT>& operator>>(validated<FormT>& form, field<T, Required>& field){
    return form.add(field);
}

/// @brief Add a field to a temporary result, as in validate(form) >> field.
template <typename FormT, typename FieldT>
validated<FormT> operator>>(validated<FormT>&& form, FieldT& field){
    form.add(field);
    return std::move(form);
}

}
}

#endif // UDHO_FORMS_VALIDATION_H
