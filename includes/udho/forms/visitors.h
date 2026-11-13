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

#ifndef UDHO_FORMS_VISITORS_H
#define UDHO_FORMS_VISITORS_H

/**
 * @file
 * @brief Builds owned constraint chains and locates their validators.
 */

#include <string>
#include <utility>

namespace udho{
namespace forms{

namespace detail{

    /**
     * @brief Reaches a validator by unwrapping links from the outside of a constraint chain.
     * @tparam ConstrainedFieldT Current constraint-chain type.
     * @tparam Depth Number of outer links to unwrap.
     */
    template <typename ConstrainedFieldT, unsigned Depth>
    struct constraint_visitor_{
        /// @brief Validator type at the requested chain position.
        typedef typename constraint_visitor_<typename ConstrainedFieldT::head_type, Depth-1>::validator_type validator_type;

        /// @brief Return a mutable reference to the selected validator.
        static validator_type& validator(ConstrainedFieldT& cf){
            return constraint_visitor_<typename ConstrainedFieldT::head_type, Depth-1>::validator(cf._head);
        }
    };

    /// @brief Selects the validator at the current chain link.
    template <typename ConstrainedFieldT>
    struct constraint_visitor_<ConstrainedFieldT, 0>{
        /// @brief Validator type stored by this link.
        typedef typename ConstrainedFieldT::validator_type validator_type;

        /// @brief Return the validator without unwrapping another link.
        static validator_type& validator(ConstrainedFieldT& cf){
            return cf._validator;
        }
    };

    /// @brief Select a validator by one-based insertion order, starting with the first constraint.
    template <typename ConstrainedFieldT, unsigned Depth>
    using constraint_visitor = constraint_visitor_<ConstrainedFieldT, ConstrainedFieldT::depth - Depth>;

    /**
     * @brief Owns copies of a field and its first validator.
     * @details Appending a constraint creates a new copied chain; it does not modify this object.
     */
    template <typename ValidatorT, typename FieldT>
    struct constrained_field{
        /// @brief Underlying field type.
        typedef FieldT field_type;
        /// @brief Parsed value type of the field.
        typedef typename field_type::value_type value_type;
        /// @brief Validator stored by this link.
        typedef ValidatorT validator_type;
        /// @brief Type of this constraint chain.
        typedef constrained_field<validator_type, field_type> self_type;

        /// @brief Number of validators in this chain.
        enum {depth = 1};

        /// @brief Owned validator copy.
        validator_type _validator;
        /// @brief Owned field copy containing the shared state within this chain.
        FieldT _field;

        /// @brief Copy the supplied validator and field into the first chain link.
        constrained_field(const validator_type& validator, const FieldT& field): _validator(validator), _field(field){}
        /// @brief Validate the underlying field with the stored validator.
        template <typename FormT>
        bool validate(const FormT& form){
            return _field.validate(form, _validator);
        }
        /// @brief Validate the field against the supplied parsed form.
        template <typename FormT>
        bool operator()(const FormT& form){
            return validate(form);
        }
        /// @brief Return the underlying field without changing its state.
        inline const field_type& field() const { return _field; }
        /// @brief Return the mutable field owned by this chain.
        inline field_type& field() { return _field; }
        /// @brief Return the stored field validity.
        inline bool valid() const { return field().valid(); }
        /// @brief Return a copy of the stored parsed value.
        inline value_type value() const { return field().value(); }
        /// @brief Return the stored validation message.
        inline std::string message() const { return field().message(); }
        /// @brief Return a copy of the field value.
        inline value_type operator*() const { return value(); }
        /// @brief Whether the stored field state is invalid.
        inline bool operator!() const { return !valid(); }
        /// @brief Return the underlying field name.
        inline std::string name() const { return field().name(); }

        /// @brief Append a copied validator to a new copy of this chain.
        template <typename ValidatorU>
        detail::constrained_field<ValidatorU, self_type> constrain(const ValidatorU& validator){
            return detail::constrained_field<ValidatorU, self_type>(validator, *this);
        }

        /// @brief Construct a validator for the field value type and append it to a copied chain.
        template <template<typename> class ValidatorU, typename... ArgsT>
        detail::constrained_field<ValidatorU<value_type>, self_type> constrain(ArgsT&&... args){
            return constrain<ValidatorU<value_type>>(ValidatorU<value_type>(std::forward<ArgsT>(args)...));
        }

        /**
         * @brief Return a mutable validator by one-based insertion order.
         * @tparam Depth Constraint position: 1 selects the first-added validator.
         */
        template <unsigned Depth>
        auto& constrain(){
            static_assert(Depth > 0 && Depth <= depth, "Constraint depth is out of range");
            if constexpr(Depth > 0 && Depth <= depth){
                return udho::forms::detail::constraint_visitor<self_type, Depth>::validator(*this);
            }
        }
    };

    /**
     * @brief Owns a validator and a copy of the preceding constraint chain.
     * @details Validators run in insertion order and stop at the first failure.
     * All links within this chain use the same innermost field and preserve its first error.
     */
    template <typename ValidatorT, typename... T>
    struct constrained_field<ValidatorT, constrained_field<T...>>{
        /// @brief Chain containing the previously added validators.
        typedef constrained_field<T...> head_type;
        /// @brief Innermost field type shared by the links in this chain.
        typedef typename head_type::field_type field_type;
        /// @brief Parsed value type of the field.
        typedef typename field_type::value_type value_type;
        /// @brief Validator stored by this outer link.
        typedef ValidatorT validator_type;
        /// @brief Type of this constraint chain.
        typedef constrained_field<validator_type, head_type> self_type;

        /// @brief Total number of validators in this chain.
        enum {depth = head_type::depth +1};

        /// @brief Owned copy of the newly added validator.
        validator_type _validator;
        /// @brief Owned copy of the preceding chain.
        head_type _head;

        /// @brief Copy the new validator and the preceding chain.
        constrained_field(const validator_type& validator, const head_type& head): _validator(validator), _head(head){}
        /// @brief Run earlier validators first, skipping this validator if an earlier check fails.
        template <typename FormT>
        bool validate(const FormT& form){
            bool valid = _head.validate(form);
            return valid && field().validate(form, _validator);
        }
        /// @brief Validate this chain against the supplied parsed form.
        template <typename FormT>
        bool operator()(const FormT& form){
            return validate(form);
        }

        /// @brief Return the mutable innermost field used by every link in this chain.
        inline field_type& field() { return _head.field(); }
        /// @brief Return the innermost field without changing its state.
        inline const field_type& field() const { return _head.field(); }
        /// @brief Return the stored field validity after the checks performed so far.
        inline bool valid() const { return field().valid(); }
        /// @brief Return a copy of the stored parsed value.
        inline value_type value() const { return field().value(); }
        /// @brief Return the first stored validation error message.
        inline std::string message() const { return field().message(); }
        /// @brief Return a copy of the field value.
        inline value_type operator*() const { return value(); }
        /// @brief Whether the stored field state is invalid.
        inline bool operator!() const { return !valid(); }
        /// @brief Return the underlying field name.
        inline std::string name() const { return field().name(); }

        /// @brief Append a copied validator to a new copy of this chain.
        template <typename ValidatorU>
        detail::constrained_field<ValidatorU, self_type> constrain(const ValidatorU& validator){
            return detail::constrained_field<ValidatorU, self_type>(validator, *this);
        }

        /// @brief Construct a validator for the field value type and append it to a copied chain.
        template <template<typename> class ValidatorU, typename... ArgsT>
        detail::constrained_field<ValidatorU<value_type>, self_type> constrain(ArgsT&&... args){
            ValidatorU<value_type> validator(std::forward<ArgsT>(args)...);
            return constrain<ValidatorU<value_type>>(validator);
        }

        /**
         * @brief Return a mutable validator by one-based insertion order.
         * @tparam Depth Constraint position: 1 selects the first-added validator.
         */
        template <unsigned Depth>
        auto& constrain(){
            static_assert(Depth > 0 && Depth <= depth, "Constraint depth is out of range");
            if constexpr(Depth > 0 && Depth <= depth){
                return udho::forms::detail::constraint_visitor<self_type, Depth>::validator(*this);
            }
        }
    };
}

}
}

#endif // UDHO_FORMS_VISITORS_H
