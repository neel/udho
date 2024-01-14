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

#include <udho/url/action.h>
#include <udho/url/mount.h>
#include <udho/hazo/seq/seq.h>
#include <udho/url/tabulate.h>

namespace udho{
namespace url{

template <typename FunctionT, typename MatchT, typename StrT, typename... TailT>
tabulate::Table& operator<<(tabulate::Table& table, const udho::hazo::basic_seq_d<basic_action<FunctionT, StrT, MatchT>, TailT...>& chain){
    table.add_row({"method", "label", "args", "pattern", "replacement", "callback"});
    for(size_t i = 0; i < 6; ++i) {
        table[0][i].format().font_color(tabulate::Color::yellow).font_style({tabulate::FontStyle::bold});
    }
    tabulize tab(table);
    chain.visit(tab);
    for(size_t i = 0; i < table.size(); ++i) {
        table[i][1].format().font_style({tabulate::FontStyle::bold});
    }
    return table;
}

template <typename FunctionT, typename MatchT, typename StrT, typename... TailT>
std::ostream& operator<<(std::ostream& stream, const udho::hazo::basic_seq_d<basic_action<FunctionT, StrT, MatchT>, TailT...>& chain){
    tabulate::Table table;
    table << chain;
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

template <typename StrT, typename ActionsT, typename... TailT>
tabulate::Table& operator<<(tabulate::Table& table, const udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, TailT...>& chain){
    tabulize tab(table);
    chain.visit(tab);
    return table;
}

template <typename StrT, typename ActionsT, typename... TailT>
std::ostream& operator<<(std::ostream& stream, const udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, TailT...>& chain){
    tabulate::Table tab;
    tab << chain;
    stream << tab;
    return stream;
}

template <typename Mountpoints>
std::ostream& operator<<(std::ostream& stream, const udho::url::router<Mountpoints>& router){
    stream << router._mountpoints;
    return stream;
}

}
}

#endif // UDHO_URL_IO_H
