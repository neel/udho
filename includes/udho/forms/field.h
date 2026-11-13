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

#ifndef UDHO_FORMS_FIELD_H
#define UDHO_FORMS_FIELD_H

/**
 * @file
 * @brief Typed required and optional fields for validating Udho form data.
 */

#include <chrono>
#include <string>
#include <type_traits>
#include <utility>
#include <udho/forms/codec.h>
#include <udho/forms/visitors.h>
#include <udho/view/data.h>

namespace udho{
namespace forms{

/// @brief Named form value whose Required parameter selects required or optional fetching.
template <typename T, bool Required = false>
struct field;

namespace detail{

/**
 * @brief Type-independent field state exposed to Udho views and accumulated results.
 *
 * A fetched field stores its name, display text, validity, and error message.
 * Display text preserves submitted strings, including malformed input; an absent
 * optional field instead displays its formatted default value.
 */
struct field_common{
    /// @brief Initialize an unfetched field with its submitted HTML name.
    field_common(const std::string& name): _name(name), _absent(true), _unparsable(true), _fetched(false), _valid(true){}
    /// @brief Return the cached missing-field flag; meaningful after fetching.
    bool absent() const { return _absent; }
    /// @brief Return the parse-failure flag; inspect after fetching a present field.
    bool unparsable() const { return _unparsable; }
    /// @brief Report whether fetch was attempted, including an absent field.
    bool fetched() const { return _fetched; }
    /// @brief Return the HTML field name used for lookup.
    const std::string& name() const { return _name; }
    /// @brief Return submitted text or the formatted default for redisplaying a form.
    std::string value_serialized() const { return _value_str; }
    /// @brief Return current validity; an unfetched field has not checked submitted data.
    bool valid() const{ return _valid; }
    /// @brief Return the missing, malformed, or first failed-constraint message.
    const std::string& message() const { return _message; }
    /// @brief Return message() under the error name exposed to views.
    const std::string& error() const { return _message; }
    /// @brief Report an invalid field.
    inline bool operator!() const { return !valid(); }

    /// @brief Expose common field properties through Udho's view metadata API.
    friend auto metatype(udho::view::data::type<field_common>){
        using namespace udho::view::data;
        return assoc("form_field"),
               cvar("fetched", &field_common::_fetched),
               cvar("name", &field_common::_name),
               cvar("value", &field_common::_value_str),
               cvar("valid", &field_common::_valid),
               cvar("error", &field_common::_message),
               cvar("message", &field_common::_message);
    }

#ifdef WITH_JSON_NLOHMANN
    /// @brief Serialize field snapshots stored in an accumulated result's map.
    friend void to_json(nlohmann::json& json, const field_common& field){
        json = udho::view::data::to_json(field);
    }
#endif

    protected:
        /// @brief Submitted HTML name used to look up this field.
        std::string _name;
        /// @brief Current missing, malformed, or constraint error message.
        std::string _message;
        /// @brief Cached missing-field flag, updated on the first fetch.
        bool _absent;
        /// @brief Parse-failure flag; meaningful after fetching a present value.
        bool _unparsable;
        /// @brief Whether the first lookup was attempted, even if the field was missing.
        bool _fetched;
        /// @brief Current parsing and constraint result, initially valid before fetching.
        bool _valid;
        /// @brief Submitted text or formatted default exposed as value in a view.
        std::string _value_str;
};

/**
 * @brief Add a typed C++ value to the common state used by Udho views.
 *
 * The value starts value-initialized, so a zero or empty value alone does not
 * indicate success. Check valid() after fetching before using the typed value.
 */
template <typename ValueT>
struct field_data: field_common{
    /// @brief Typed C++ value produced by parsing this field.
    typedef ValueT value_type;
    /// @brief This field storage or specialization.
    typedef field_data<ValueT> self_type;

    using field_common::field_common;

    /// @brief Return the parsed value or the default selected for an absent optional field.
    inline const value_type& value() const { return _value; }
    /// @brief Access the typed value through dereferencing.
    inline const value_type& operator*() const { return value(); }
    /// @brief Set the typed value and its display text using optional codec arguments.
    template <typename... ArgsT>
    void value(const value_type& v, const ArgsT&... args) { _value = v; field_common::_value_str = detail::value_codec<value_type>::serialize(v, args...); }
    /// @brief Access the common state copied into an accumulated result.
    field_common& common() { return static_cast<field_common&>(*this); }
    /// @brief Access the common state without modifying it.
    const field_common& common() const { return static_cast<const field_common&>(*this); }
    private:
        /// @brief Value-initialized typed storage; consult validity before using it.
        value_type  _value{};
};

/**
 * @brief Shared field implementation with fluent messages and constraint construction.
 *
 * DerivedT supplies required/optional fetching. Constraints own a copy of the
 * derived field, so keep and validate the wrapper returned by constrain().
 */
template <typename ValueT, typename DerivedT>
struct basic_field: field_data<ValueT>{
    /// @brief Typed C++ value produced by parsing this field.
    typedef ValueT value_type;
    /// @brief Concrete field type returned by fluent configuration.
    typedef DerivedT derived_type;
    /// @brief Base storing the typed value and common field state.
    typedef field_data<ValueT> data_type;

    using data_type::absent;
    using data_type::unparsable;

    /// @brief Initialize default missing and malformed messages from the field name.
    basic_field(const std::string& name): data_type(name), _message_absent(name + " absent"), _message_unparsable(name + " malformed") {}
    /// @brief Set the message used when a required field is absent.
    derived_type& absent(const std::string& message) { _message_absent = message; return self(); }
    /// @brief Set the message used when a supplied value cannot be parsed.
    derived_type& unparsable(const std::string& message) { _message_unparsable = message; return self(); }
    /// @brief Access typed storage and common field state.
    data_type& data() { return static_cast<data_type&>(*this);}
    /// @brief Access typed storage and common field state without modifying them.
    const data_type& data() const { return static_cast<const data_type&>(*this);}

    /// @brief Expose a concrete required/optional field using its common view properties.
    friend auto metatype(udho::view::data::type<derived_type>){
        using namespace udho::view::data;
        return assoc("field"), metatype(type<field_common>{});
    }

    /// @brief Return a wrapper owning copies of this field and the supplied validator.
    template <typename ValidatorT>
    detail::constrained_field<ValidatorT, derived_type> constrain(const ValidatorT& validator){
        return detail::constrained_field<ValidatorT, derived_type>(validator, self());
    }

    /// @brief Instantiate the validator template for value_type and constrain a copy of this field.
    template <template<typename> class ValidatorT, typename... ArgsT>
    detail::constrained_field<ValidatorT<value_type>, derived_type> constrain(ArgsT&&... args){
        return detail::constrained_field<ValidatorT<value_type>, derived_type>(ValidatorT<value_type>(std::forward<ArgsT>(args)...), self());
    }

    protected:
        /// @brief Convert a present field while preserving its submitted text for redisplay.
        template <typename FormT, typename... ArgsT>
        void read(const FormT& form, bool& okay, const ArgsT&... args){
            data_type::value(detail::read_field<value_type>(form, data_type::_name, okay, args...), args...);
            const auto& input = detail::view(form).field(data_type::_name);
            if(input.is_string()) data_type::_value_str = input.string();
        }

        /// @brief Apply a validator to a valid present value; absent optional values skip it.
        template <typename ValidatorT>
        bool validate(const ValidatorT& validator){
            if(data_type::valid() && !data_type::absent()){
                data_type::_valid = data_type::_valid && validator(data_type::value());
                if(!data_type::_valid){
                    data_type::_message = validator.message();
                }
                return data_type::_valid;
            }
            return data_type::valid();
        }

        /// @brief Return the concrete field for fluent configuration.
        derived_type& self() { return static_cast<derived_type&>(*this); }
        /// @brief Return the concrete field without modifying it.
        const derived_type& self() const { return static_cast<const derived_type&>(*this); }

        /// @brief Configured missing-field message and malformed-message fallback.
        std::string _message_absent;
        /// @brief Configured message for a supplied value that cannot be parsed.
        std::string _message_unparsable;
};

}

/**
 * @brief Required text field converted to T from Udho's parsed form data.
 *
 * A missing, empty, non-text, or unparseable value is invalid. The first fetch
 * is cached, so create a new field for each form submission.
 */
template <typename T>
struct field<T, true>: detail::basic_field<T, field<T, true>>{
    /// @brief Typed C++ value produced by parsing this field.
    typedef T value_type;
    /// @brief This field storage or specialization.
    typedef field<T, true> self_type;
    /// @brief Shared implementation of typed field storage and configuration.
    typedef detail::basic_field<T, field<T, true>> base;
    /// @brief Whether a missing submitted name makes this field invalid.
    enum { is_required = true };

    /// @brief Select the submitted HTML field by name.
    field(const std::string& name): base(name){}
    /// @brief Fetch the first named value once and return its current validity.
    template <typename FormT>
    bool fetch(const FormT& form){
        if(base::_fetched) return base::valid();
        base::_absent  = detail::view(form).count(base::_name) == 0;
        if(!base::_absent){
            bool okay = true;
            base::read(form, okay);
            base::_unparsable = !okay;
            if(base::_unparsable){
                base::_message = base::_message_unparsable.empty() ? base::_message_absent : base::_message_unparsable;
            }
        }else{
            base::_message = base::_message_absent;
        }
        base::_valid = !base::_absent && !base::_unparsable;
        base::_fetched = true;
        return base::valid();
    }
    /// @brief Fetch if needed, then apply a constraint while the field remains valid.
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        return base::template validate<ValidatorT>(validator);
    }
};

/**
 * @brief Optional field using a typed default only when its name is absent.
 *
 * An absent field stays valid and skips constraints. A supplied empty, non-text,
 * or unparseable value is invalid. Fetching is cached for this field instance.
 */
template <typename T>
struct field<T, false>: detail::basic_field<T, field<T, false>>{
    /// @brief Typed C++ value produced by parsing this field.
    typedef T value_type;
    /// @brief This field storage or specialization.
    typedef field<T, false> self_type;
    /// @brief Shared implementation of typed field storage and configuration.
    typedef detail::basic_field<T, field<T, false>> base;
    /// @brief Whether a missing submitted name makes this field invalid.
    enum { is_required = false };

    /// @brief Configure the HTML name and the value used when it is absent.
    field(const std::string& name, const value_type& def = value_type()): base(name), _def(def){}
    /// @brief Return the configured default value.
    const value_type& def() const { return _def; }
    /// @brief Fetch once, selecting the default only when no named value was submitted.
    template <typename FormT>
    bool fetch(const FormT& form){
        if(base::_fetched) return base::valid();
        base::_absent  = detail::view(form).count(base::_name) == 0;
        if(!base::_absent){
            bool okay = true;
            base::read(form, okay);
            base::_unparsable = !okay;
            base::_valid = !base::_unparsable;
            if(base::_unparsable){
                base::_message = base::_message_unparsable.empty() ? base::_message_absent : base::_message_unparsable;
            }
        }else{
            base::_valid = true;
            base::value(_def);
        }
        base::_fetched = true;
        return base::valid();
    }
    /// @brief Fetch if needed and validate a supplied value; skip absent defaults.
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        return base::template validate<ValidatorT>(validator);
    }

    protected:
        /// @brief Typed default selected only when this field name is absent.
        value_type _def;
};

/**
 * @brief Required system-clock date-time field parsed using a local-time format.
 *
 * Like other required fields, the first fetch is cached. The format follows
 * std::get_time(), for example "%Y-%m-%dT%H:%M:%S" for an HTML value.
 */
template <typename DurationT>
struct field<std::chrono::time_point<std::chrono::system_clock, DurationT>, true>: detail::basic_field<std::chrono::time_point<std::chrono::system_clock, DurationT>, field<std::chrono::time_point<std::chrono::system_clock, DurationT>, true>>{
    /// @brief Typed C++ value produced by parsing this field.
    typedef std::chrono::time_point<std::chrono::system_clock, DurationT> value_type;
    /// @brief This field storage or specialization.
    typedef field<std::chrono::time_point<std::chrono::system_clock, DurationT>, true> self_type;
    /// @brief Shared implementation of typed field storage and configuration.
    typedef detail::basic_field<std::chrono::time_point<std::chrono::system_clock, DurationT>, field<std::chrono::time_point<std::chrono::system_clock, DurationT>, true>> base;
    /// @brief Whether a missing submitted name makes this field invalid.
    enum { is_required = true };

    /// @brief Local-time format passed to the date-time codec.
    std::string _format;

    /// @brief Configure the HTML field name and expected local date-time format.
    field(const std::string& name, const std::string& format = default_datetime_format): base(name), _format(format){}
    /// @brief Fetch and parse the first named date-time once.
    template <typename FormT>
    bool fetch(const FormT& form){
        if(base::_fetched) return base::valid();
        base::_absent  = detail::view(form).count(base::_name) == 0;
        if(!base::_absent){
            bool okay = true;
            base::read(form, okay, _format);
            base::_unparsable = !okay;
            if(base::_unparsable){
                base::_message = base::_message_unparsable.empty() ? base::_message_absent : base::_message_unparsable;
            }
        }else{
            base::_message = base::_message_absent;
        }
        base::_valid = !base::_absent && !base::_unparsable;
        base::_fetched = true;
        return base::valid();
    }
    /// @brief Fetch if needed, then validate the parsed time point.
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        return base::template validate<ValidatorT>(validator);
    }
};

/**
 * @brief Optional local date-time field with a typed default for an absent name.
 *
 * Missing values use the default and skip constraints; supplied malformed
 * values fail. The custom format also formats the default for form redisplay.
 */
template <typename DurationT>
struct field<std::chrono::time_point<std::chrono::system_clock, DurationT>, false>: detail::basic_field<std::chrono::time_point<std::chrono::system_clock, DurationT>, field<std::chrono::time_point<std::chrono::system_clock, DurationT>, false>>{
    /// @brief Typed C++ value produced by parsing this field.
    typedef std::chrono::time_point<std::chrono::system_clock, DurationT> value_type;
    /// @brief This field storage or specialization.
    typedef field<std::chrono::time_point<std::chrono::system_clock, DurationT>, false> self_type;
    /// @brief Shared implementation of typed field storage and configuration.
    typedef detail::basic_field<std::chrono::time_point<std::chrono::system_clock, DurationT>, field<std::chrono::time_point<std::chrono::system_clock, DurationT>, false>> base;
    /// @brief Whether a missing submitted name makes this field invalid.
    enum { is_required = false };

    /// @brief Local-time format used for submitted values and the default display text.
    std::string _format;

    /// @brief Configure the HTML name, local-time format, and absent-field default.
    field(const std::string& name, const std::string& format = default_datetime_format, const value_type& def = value_type()): base(name), _format(format), _def(def){}
    /// @brief Return the configured default time point.
    const value_type& def() const { return _def; }
    /// @brief Fetch once, parsing a supplied value or selecting the formatted default.
    template <typename FormT>
    bool fetch(const FormT& form){
        if(base::_fetched) return base::valid();
        base::_absent  = detail::view(form).count(base::_name) == 0;
        if(!base::_absent){
            bool okay = true;
            base::read(form, okay, _format);
            base::_unparsable = !okay;
            base::_valid = !base::_unparsable;
            if(base::_unparsable){
                base::_message = base::_message_unparsable.empty() ? base::_message_absent : base::_message_unparsable;
            }
        }else{
            base::_valid = true;
            base::value(_def, _format);
        }
        base::_fetched = true;
        return base::valid();
    }
    /// @brief Fetch if needed and validate a supplied time point; skip absent defaults.
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        return base::template validate<ValidatorT>(validator);
    }

    protected:
        /// @brief Typed default selected only when this field name is absent.
        value_type _def;
};

/// @brief Fetch and validate a constrained field from form_data or form_view; allow chaining.
template <typename FormT, typename ValidatorT, typename FieldT, std::enable_if_t<detail::is_form_v<FormT>, int> = 0>
const FormT& operator>>(const FormT& form, detail::constrained_field<ValidatorT, FieldT>& field){
    field(form);
    return form;
}

/// @brief Fetch an unconstrained field from form_data or form_view; allow chaining.
template <typename FormT, typename T, bool Required, std::enable_if_t<detail::is_form_v<FormT>, int> = 0>
const FormT& operator>>(const FormT& form, field<T, Required>& field){
    field.fetch(form);
    return form;
}

/// @brief Shorthand for a field that must be supplied and parsed successfully.
template <typename T>
using required = field<T, true>;

/// @brief Shorthand for a field that uses its default only when absent.
template <typename T>
using optional = field<T, false>;

}
}

#endif // UDHO_FORMS_FIELD_H
