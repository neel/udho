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

#ifndef UDHO_CONTEXTS_H
#define UDHO_CONTEXTS_H

#include <udho/context.h>
#include <udho/bridge.h>
#include <udho/configuration.h>

namespace udho{
/**
 * conveniance typedefs for stateful and stateless contexts using default bridge and the default HTTP request type.
 * \defgroup shorthands
 * \ingroup context
 */
namespace contexts{
    /**
     * A stateless context for stateless callable using the default bridge and the default HTTP request type.
     * \see udho::context
     * \ingroup shorthands
     */
    using stateless = context<udho::bridge<udho::configuration_type>, udho::defs::request_type, void>;
    /**
     * A stateful context for stateful callable using the default bridge and the default HTTP request type.
     * \tparam T... Session states
     * \see udho::context< AuxT, RequestT, void >
     * \ingroup shorthands
     */
    template <typename... T>
    using stateful = context<udho::bridge<udho::configuration_type>, udho::defs::request_type, udho::cache::shadow<udho::defs::session_key_type, T...>>;
}
}

#endif // UDHO_CONTEXTS_H
