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

#ifndef UDHO_DEFS_H
#define UDHO_DEFS_H

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/version.hpp>

//  UDHO_VERSION % 100 is the patch level
//  UDHO_VERSION / 100 % 1000 is the minor version
//  UDHO_VERSION / 100000 is the major version

#define UDHO_VERSION 100000
#define UDHO_VERSION_STRING "Udho (উধো) 1.0.0 "  BOOST_BEAST_VERSION_STRING " Boost " BOOST_LIB_VERSION

namespace udho{
namespace defs{
    
typedef boost::beast::http::request<boost::beast::http::string_body>  request_type;
typedef boost::beast::http::response<boost::beast::http::string_body> response_type;
typedef boost::uuids::uuid session_key_type;
    
}
}

/**
 * \defgroup configuration configuration
 * Configuration
 * 
 * udho is configured via heterogenous configurations modules. A configuration class exposes its 
 * configurable parameters, and getter and setters for each of these parameters. and example of 
 * an configuration module x is shown below.
 * \code
 * template <typename T = void>
 * struct x_{
 *      const static struct param_t{                    // The parameter declaration
 *          typedef x_<T> component;                    // There has to be a typedef component to reach the parent config
 *      } param;                                        // the parameter will be identifies with X::param (see the end of this example);
 * 
 *      int param_v;                                    // The parameter value
 * 
 *      void set(param_t, int v) { param_v = v; }       // The setter
 *      int get(param_t) const { return param_v; }      // The setter
 * };
 * 
 * template <typename T> const typename x_<T>::param_t x_<T>::param;
 * 
 * typedef x_<> x;                                      // typedef x as x<>
 * \endcode
 * 
 * multiple such config modules are passed to the udho::configuration template. 
 * Each of the passed config modules are wrapped inside udho::config which provides operator[] overload 
 * for getter and setters which calls the actual get and set functions shown in the example above. Hence
 * the configuration<x, y> can be used with parameters of both x and y module.
 */

/**
 * \defgroup routing routing
 * 
 * URL Routing
 */

/**
 * \defgroup overload overload
 * \ingroup routing
 * 
 * URL overload
 */

/**
 * \defgroup context context
 * 
 * Request Context
 */

/**
 * \defgroup server server
 * 
 * HTTP Server
 */

/**
 * \defgroup cache cache
 * 
 * Cache
 */

/**
 * \defgroup session session
 * 
 * Session
 */

/**
 * \defgroup view view
 * 
 * View Engine
 */

#endif // DEFS_H
