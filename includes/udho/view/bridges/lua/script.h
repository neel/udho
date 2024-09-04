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

#ifndef UDHO_VIEW_BRIDGES_LUA_SCRIPT_H
#define UDHO_VIEW_BRIDGES_LUA_SCRIPT_H

#include <string>
#include <vector>
#include <functional>
#include <udho/view/tmpl/sections.h>
#include <udho/view/bridges/script.h>
#include <udho/url/detail/format.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

/**
 * @class script
 * @brief Extends the generic script functionality to implement Lua-specific script operations.
 *
 * This class generates and manages Lua scripts derived from various sections of a template (view). It processes different types of template sections such as meta, text, echo, and eval etc...
 */
struct script: udho::view::data::bridges::script{
    /**
     * @brief Constructs a Lua script with a specified name.
     * @param name The identifier name of the script.
     */
    inline explicit script(const std::string& name): udho::view::data::bridges::script(name) {
        // TODO This d should not be constant it should be determined from the variable names inside the meta section.
        // TODO It should contain additional parameter ctx and the signatre should be function(d, ctx, stream) instead.
        // TODO The name ctx should not be fixed as well, it should be retrieved from the neta also.
        *this << "return function(d, stream)" << std::endl;
        ++*this;
    }

    /**
     * @brief Processes a given template section into Lua script format.
     * @param section The template section to process.
     */
    inline void operator()(const udho::view::tmpl::section& section){
        add_section(section);
    }

    /**
     * @brief Finalizes the script, ensuring proper closure in Lua syntax.
     */
    void finish(){
        --*this;
        *this << "end";
    }
    private:
        /**
         * @brief Adds a generic section to the Lua script.
         * @param section The section to add.
         */
        inline void add_section(const udho::view::tmpl::section& section){
            switch(section.type()){
                case udho::view::tmpl::section::meta:
                    add_meta_section(section);
                    break;
                case udho::view::tmpl::section::text:
                case udho::view::tmpl::section::verbatim:
                case udho::view::tmpl::section::echo:
                    add_echo_section(section);
                    break;
                case udho::view::tmpl::section::eval:
                    add_eval_section(section);
                    break;
                default:
                    break;
            }
        }
        /**
         * @brief Adds an evaluation (eval) section from the template into the Lua script.
         * @details This function directly integrates Lua code found within eval blocks of the template into the script. It encapsulates code meant to be executed during the template rendering process, allowing dynamic content generation based on the evaluation results.
         * @param section The eval section to add, containing Lua code.
         */
        inline void add_eval_section(const udho::view::tmpl::section& section) {
            udho::view::data::bridges::script::accept(section);
        }

        /**
         * @brief Adds an echo section to the Lua script.
         * @details Echo sections are processed to output text or expressions directly into the rendered template. This method formats these sections into Lua print statements or equivalent, ensuring they are executed and their outputs are captured during the template's rendering.
         * @param section The echo section containing text or expressions to be output.
         */
        inline void add_echo_section(const udho::view::tmpl::section& section) {
            if (section.size() > 0) {
                *this << std::endl;
                *this << "do -- " + udho::url::format("{}", udho::view::tmpl::section::name(section.type())) << std::endl;
                ++*this;
                if (section.type() == udho::view::tmpl::section::echo) {
                    *this << udho::url::format("local udho_view_str_ = string.format([=====[%s]=====], {})", section.content()) << std::endl;
                } else {
                    *this << udho::url::format("local udho_view_str_ = [=====[{}]=====]", section.content()) << std::endl;
                }
                *this << "stream:write(udho_view_str_)" << std::endl;
                --*this;
                *this << "end" << std::endl;
            }
        }

        /**
         * @brief Adds a meta section to the Lua script.
         * @details Meta sections typically contain configuration or directives that influence how the template is processed or how the scripting functions. These sections might modify the script's behavior, set up necessary preconditions, or provide metadata that affects the execution context. The implementation should parse and integrate these directives into the Lua script accordingly.
         * @param section The meta section to integrate.
         */
        inline void add_meta_section(const udho::view::tmpl::section& section) {
            // TODO implement
        }

};

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_SCRIPT_H
