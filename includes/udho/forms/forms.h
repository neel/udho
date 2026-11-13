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

#ifndef UDHO_FORMS_FORMS_H
#define UDHO_FORMS_FORMS_H

/**
 * @file
 * @brief Umbrella header for typed validation of parsed HTTP form fields.
 *
 * Udho's protocol component parses the request body before the action is called.
 * This module consumes the resulting form_data or form_view; it does not read
 * the network or parse the body again.
 *
 * Fields describe the expected names and C++ types. A codec converts each text
 * value, and constraints check the converted value. constrained_field owns a
 * copy of its field and validators, checking constraints in attachment order
 * until the first failure for that field.
 *
 * validate(form) creates a validated result. Adding fields with operator>>()
 * fetches and checks them, then stores their display states in the inherited
 * accumulated summary. Bare form >> field and summary << field perform these
 * two steps separately. Typed values stay in the field objects; the summary
 * holds validity, errors, and text values suitable for redisplaying the form.
 *
 * @code
 * using namespace udho::forms;
 * const auto& form = context.portal().body().form();
 * auto age = required<unsigned>("age")
 *     .constrain<constraints::gte>(18, "Must be 18 or older");
 * auto nickname = optional<std::string>("nickname", "Guest");
 * auto result = validate(form);
 * result >> age >> nickname;
 * if(result.valid()){
 *     const unsigned parsed_age = *age; // Use the typed value in application logic.
 * }
 * @endcode
 *
 * When a result is exposed through a Udho view model, Lua can read result.valid,
 * result.errors, and result.fields.age.value or result.fields.age.message. The
 * field value exposed to views is text, including malformed submitted text.
 *
 * A missing required field is invalid. A missing optional field uses its default
 * and skips constraints; a present empty value is malformed for either kind.
 * Scalar fields read the first occurrence of a name and reject upload paths or
 * buffers. submitted() records a field fetch attempt, not the HTTP method.
 *
 * The parsed form must outlive its validated object. Fields cache their first fetch;
 * create fresh fields and a fresh validation object for each request. Date-time
 * fields use local time and default_datetime_format unless a format is supplied.
 */
#include <udho/forms/codec.h>
#include <udho/forms/visitors.h>
#include <udho/forms/constraints.h>
#include <udho/forms/field.h>
#include <udho/forms/accumulated.h>
#include <udho/forms/validation.h>

#endif // UDHO_FORMS_FORMS_H
