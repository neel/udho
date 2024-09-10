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

#ifndef UDHO_VIEW_RESOURCES_RESULTS_H
#define UDHO_VIEW_RESOURCES_RESULTS_H

#include <string>
#include <udho/view/resources/fwd.h>

namespace udho{
namespace view{
namespace resources{

/**
 * @struct results
 * @brief Encapsulates the output of a resource execution, including metadata like name, size, and type.
 *
 * This structure is used to store and access the results of executing a resource, such as a rendered view. It is not constructible directly but through friend classes that manage resource execution.
 */
struct results{
    template <typename BridgeT>
    friend struct udho::view::resources::tmpl::proxy;  ///< Allows proxy to construct and modify results.

    results() = delete;  ///< Prevents direct construction of results instances.

    /**
     * @brief Returns the name of the resource associated with these results.
     * @return The resource name as a string.
     */
    inline std::string name() const { return _name; }
    /**
     * @brief Returns the size of the output data.
     * @return Size of the output.
     */
    inline std::size_t size() const { return _size; }
    /**
     * @brief Returns the type of the resource.
     * @return Resource type as a string.
     */
    inline std::string type() const { return _type; }
    /**
     * @brief Provides access to the string output of the resource execution.
     * @return A const reference to the output string.
     */
    inline const std::string& str() const { return _output; }
    /**
     * @brief Returns an iterator to the beginning of the output string.
     * @return A const iterator to the start of the output string.
     */
    inline std::string::const_iterator begin() const { return _output.begin(); }
    /**
     * @brief Returns an iterator to the end of the output string.
     * @return A const iterator to the end of the output string.
     */
    inline std::string::const_iterator end() const { return _output.end(); }

    private:
        inline explicit results(const std::string& name): _name(name) {}
        inline void type(const std::string& t) { _type = t; }
        inline void size(const std::size_t& s) { _size = s; }
        inline std::string& output() { return _output; }
    private:
        std::string _name;
        std::string _output;
        std::string _type;
        std::size_t _size;
};

}
}
}


#endif // UDHO_VIEW_RESOURCES_RESULTS_H
