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
#include <udho/view/sections.h>
#include <udho/view/bridges/script.h>
#include <udho/url/detail/format.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

namespace detail{
namespace lua{

struct script: udho::view::data::bridges::script{
    inline explicit script(const std::string& name): udho::view::data::bridges::script(name) {
        *this << "return function(d, stream)" << std::endl;
        ++*this;
    }
    inline void operator()(const udho::view::sections::section& section){
        add_section(section);
    }
    void finish(){
        --*this;
        *this << "end";
    }
    private:
        inline void add_section(const udho::view::sections::section& section){
            switch(section.type()){
                case udho::view::sections::section::text:
                case udho::view::sections::section::verbatim:
                case udho::view::sections::section::echo:
                    add_echo_section(section);
                    break;
                case udho::view::sections::section::eval:
                    add_eval_section(section);
                    break;
                default:
                    break;
            }
        }
        inline void add_eval_section(const udho::view::sections::section& section) {
            udho::view::data::bridges::script::accept(section);
        }

        inline void add_echo_section(const udho::view::sections::section& section) {
            if (section.size() > 0) {
                *this << std::endl;
                *this << "do -- " + udho::url::format("{}", udho::view::sections::section::name(section.type())) << std::endl;
                ++*this;
                if (section.type() == udho::view::sections::section::echo) {
                    *this << udho::url::format("local udho_view_str_ = string.format([=====[%s]=====], {})", section.content()) << std::endl;
                } else {
                    *this << udho::url::format("local udho_view_str_ = [=====[{}]=====]", section.content()) << std::endl;
                }
                *this << "stream:write(udho_view_str_)" << std::endl;
                --*this;
                *this << "end" << std::endl;
            }
        }

};

}
}

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_LUA_SCRIPT_H
