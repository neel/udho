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

#ifndef UDHO_FORMS_ACCUMULATED_H
#define UDHO_FORMS_ACCUMULATED_H

/** @file
 * @brief Collect field results and errors for application logic and Udho views.
 */

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <udho/forms/field.h>
#include <udho/view/data.h>

namespace udho{
namespace forms{

/**
 * @brief Summary of field states already fetched or validated by the caller.
 *
 * Copies display data rather than typed field values. Adding a field does not
 * fetch or validate it. The first snapshot for each name is retained; use a fresh
 * summary for each form. Metadata exposes submitted, valid, errors, and fields.
 */
struct accumulated{
    /// @brief Summary type returned by fluent additions.
    typedef accumulated self_type;

    /// @brief Create an empty, valid summary with no fetched fields.
    inline accumulated(): _valid(true), _submitted(false){}

    /// @brief Copy the underlying field state from a constrained field.
    template <typename ValidatorT, typename TailT>
    self_type& add(const detail::constrained_field<ValidatorT, TailT>& cf){
        return add(cf.field().common());
    }
    /// @brief Add a field snapshot, update overall validity, and collect its error.
    inline self_type& add(const detail::field_common& common){
        _submitted = _submitted || common.fetched();
        bool valid = common.valid();
        _valid = _valid && valid;
        if(!valid){
            _errors.push_back(common.message());
        }
        _fields.insert(std::make_pair(common.name(), common));
        return *this;
    }

    /// @brief Return whether all added states are valid and no form error was added.
    inline bool valid() const { return _valid; }
    /// @brief Return whether any added field was fetched, even if it was absent.
    inline bool submitted() const { return _submitted; }

    /// @brief Append an error for the whole form and mark the summary invalid.
    inline void add_error(const std::string& err){
        _valid = false;
        _errors.push_back(err);
    }

    /// @brief Return field and form errors in the order they were added.
    const std::vector<std::string>& errors() const{
        return _errors;
    }

    /**
     * @brief Return the stored display state for a field name.
     * @throws std::out_of_range If no field with this name was added.
     */
    const detail::field_common& operator[](const std::string& name) const{
        return _fields.at(name);
    }

    /// @brief Expose the summary and its field map to Udho data bridges.
    friend auto metatype(udho::view::data::type<accumulated>){
        using namespace udho::view::data;
        return assoc("form_validation"),
               cvar("submitted", &self_type::_submitted),
               cvar("valid", &self_type::_valid),
               cvar("errors", &self_type::_errors),
               // The Lua map table adapter uses the mvar policy.
               mvar("fields", &self_type::_fields);
    }

    private:
        bool _valid;
        bool _submitted;
        std::vector<std::string> _errors;
        std::map<std::string, detail::field_common> _fields;
};

/// @brief Add a constrained field's current state without running validation.
template <typename ValidatorT, typename FieldT>
accumulated& operator<<(accumulated& acc, const detail::constrained_field<ValidatorT, FieldT>& field){
    return acc.add(field);
}

/// @brief Add an ordinary field's current state without fetching it.
template <typename T, bool Required>
accumulated& operator<<(accumulated& acc, const field<T, Required>& field){
    return acc.add(field);
}

}
}

#endif // UDHO_FORMS_ACCUMULATED_H
