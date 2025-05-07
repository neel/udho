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

#ifndef UDHO_VIEW_BRIDGES_SCRIPT_H
#define UDHO_VIEW_BRIDGES_SCRIPT_H

#include <string>
#include <udho/view/bridges/stream.h>
#include <udho/view/bridges/header.h>
#include <udho/view/tmpl/sections.h>
#include <udho/view/meta.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <udho/view/resources/fwd.h>

namespace udho{
namespace view{
namespace data{
namespace bridges{

/**
 * @struct basic_script
 * @ingroup view
 * @brief A specialized stream for handling script generation, particularly useful in scenarios where scripts or code need to be dynamically generated from templates.
 *
 * Inherits from `stream<char, '\t'>` to utilize generic text streaming capabilities with a focus on script formatting.
 */
template <typename DerivedT>
struct basic_script: stream<char, '\t'>{
    using derived_type = DerivedT;
    using header_type  = udho::view::data::bridges::view_header;
    /**
     * @brief Constructs a new script object with a specified name.
     * @param name The name of the script, often used as an identifier.
     */
    explicit basic_script(const std::string& name): stream(), _name(name), _meta_processed(false) {}
    /**
     * @brief Returns the name of the script.
     * @return The name of the script.
     */
    std::string name() const { return _name; }

    /**
     * @brief Returns the meta information of the view.
     * @return View header
     */
    const view_header& header() const{ return _header; }
    /**
     * @brief Processes a given template section into script format.
     * @details Process the meta section inside basic_script as it is same for all template engine. For all other sections delegates the call to the derived class
     * @param section The template section to process.
     */
    inline void operator()(const udho::view::tmpl::section& section){
        if(section.type() == udho::view::tmpl::section::meta){
            if(_meta_processed){
                throw std::runtime_error{"Encountered multiple meta blocks"};
            }

            // pass the _description object through the contents of the meta block
            // this may update the default values of the variables such as vars etc..

            std::string instructions = section.content();
            udho::view::data::meta::exec(_header, instructions);
            self().begin(_header);

            _meta_processed = true;
        } else {
            if(!_meta_processed){
                // TODO warn discarding a block encountered before the meta block
                discard(section);
            } else {
                if (section.size() == 0) {
                    // empty section always discard
                    discard(section);
                } else if (section.type() == udho::view::tmpl::section::text && !_header.whitespace && section.is_whitespace() && section.size() > 1) {
                    // whitespace if false and the section has only whitespaces and there are more than one white space
                    // hence discard
                    // Note: if the section has exactly one white space then keep it
                    const std::string& content = section.content();
                    discard(section);
                    udho::view::tmpl::section space{udho::view::tmpl::section::text, std::string{content[0]}};
                    self().process(space);
                } else {
                    self().process(section);
                }
            }
        }
    }

    void finish(){
        self().end();
    }

    protected:
        /**
         * @brief Accepts a section from a template and appends it to the script.
         * @param section The template section to append.
         */
        inline void accept(const udho::view::tmpl::section& section){ stream::append(section.begin(), section.end()); }
        /**
         * @brief Discards a section from a template. Currently, this function does not perform any operation.
         * @param section The template section to discard.
         */
        inline void discard(const udho::view::tmpl::section&){}

    private:
        derived_type& self() { return static_cast<derived_type&>(*this); }
        /**
         * @brief Adds a meta section to the Lua script.
         * @details Meta sections typically contain configuration or directives that influence how the template is processed or how the scripting functions. These sections might modify the script's behavior, set up necessary preconditions, or provide metadata that affects the execution context. The implementation should parse and integrate these directives into the Lua script accordingly.
         * @param section The meta section to integrate.
         */
        inline void add_meta_section(const udho::view::tmpl::section& section) {
            // TODO implement
            throw std::runtime_error{"Need to parse view meta block"};
        }
    private:
        std::string _name;
        view_header _header;
        bool        _meta_processed;
};

}
}
}
}

#endif // UDHO_VIEW_BRIDGES_SCRIPT_H

