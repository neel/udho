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

#ifndef UDHO_URL_IO_H
#define UDHO_URL_IO_H

#include <udho/url/fwd.h>
#include <udho/url/io_fwd.h>
#include <udho/url/tabulate.h>

#include <udho/url/action.h>
#include <udho/url/mount.h>
#include <udho/url/tables.h>

namespace udho{
namespace url{

/**
 * @addtogroup DoxyG_url_op
 * @{
 */

template <typename... Args>
tabulate::Table& operator<<(tabulate::Table& table, const action_table<Args...>& actions){
    table.add_row({"method", "label", "args", "pattern", "replacement", "callback"});
    for(size_t i = 0; i < 6; ++i) {
        table[0][i].format().font_color(tabulate::Color::yellow).font_style({tabulate::FontStyle::bold});
    }
    tabulize tab(table);
    actions.visit(tab);
    for(size_t i = 0; i < table.size(); ++i) {
        table[i][1].format().font_style({tabulate::FontStyle::bold});
    }
    return table;
}

template <typename... Args>
std::ostream& operator<<(std::ostream& stream, const action_table<Args...>& actions){
    tabulate::Table table;
    table << actions;
    stream << table;
    return stream;
}

template <typename StrT, typename ActionsT>
tabulate::Table& operator<<(tabulate::Table& table, const mount_point<StrT, ActionsT>& point){
    table.add_row({point.name().c_str(), point.path()});
    tabulate::Table chain_table;
    chain_table << point.actions();
    table.add_row({"", chain_table});
    return table;
}

template <typename StrT, typename ActionsT>
std::ostream& operator<<(std::ostream& stream, const mount_point<StrT, ActionsT>& point){
    tabulate::Table table;
    table << point;
    stream << table;
    return stream;
}

template <typename... Args>
tabulate::Table& operator<<(tabulate::Table& table, const mountpoints_table<Args...>& mountpoints){
    tabulize tab(table);
    mountpoints.visit(tab);
    return table;
}

template <typename... Args>
std::ostream& operator<<(std::ostream& stream, const mountpoints_table<Args...>& mountpoints){
    tabulate::Table tab;
    tab << mountpoints;
    stream << tab;
    return stream;
}

template <typename Mountpoints>
std::ostream& operator<<(std::ostream& stream, const udho::url::detail::routing_table<Mountpoints>& router){
    router.print(stream);
    return stream;
}

template <typename Mountpoints>
std::ostream& operator<<(std::ostream& stream, const basic_router<Mountpoints>& router){
    const detail::routing_table<Mountpoints>& table = router.table();
    stream << table;
    return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const basic_router<void>& router){
    return stream;
}

/// @}

}
}

#endif // UDHO_URL_IO_H
