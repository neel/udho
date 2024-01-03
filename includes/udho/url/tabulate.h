#ifndef UDHO_URL_TABULATE_H
#define UDHO_URL_TABULATE_H

#include <tabulate/table.hpp>
#include <udho/url/fwd.h>
#include <boost/lexical_cast.hpp>

namespace udho{
namespace url{

// template <typename ActionT>
// struct tabulize;


struct tabulize{
    tabulate::Table& _table;

    tabulize(tabulate::Table& table): _table(table) {}

    template<typename FunctionT, typename StrT, typename MatchT>
    void operator()(const basic_action<FunctionT, StrT, MatchT>& action){
        _table.add_row({
            boost::lexical_cast<std::string>(action.match().method()),
            StrT().c_str(),
            std::to_string(action.args),
            action.match().pattern(), action.match().replacement(),
            action.symbol()
        });
    }
};

// template<typename FunctionT, typename StrT, typename MatchT>
// struct tabulize<udho::hazo::capsule<basic_action<FunctionT, StrT, MatchT>>>
//     : private tabulize<basic_action<FunctionT, StrT, MatchT>>
// {
//     tabulize(tabulate::Table& table)
//         : tabulize<basic_action<FunctionT, StrT, MatchT>>(table) {}
//
//     void operator()(const udho::hazo::capsule<basic_action<FunctionT, StrT, MatchT>>& capsule){
//         tabulize<basic_action<FunctionT, StrT, MatchT>>::operator()(capsule.data());
//     }
// };

// template<typename FunctionT, typename StrT, typename MatchT, typename... TailT>
// struct tabulize<udho::hazo::basic_seq_d<basic_action<FunctionT, StrT, MatchT>, TailT...>>
//     : private tabulize<basic_action<FunctionT, StrT, MatchT>>,
//       private tabulize<TailT...>
// {
//     tabulize(tabulate::Table& table)
//         : tabulize<basic_action<FunctionT, StrT, MatchT>>(table),
//           tabulize<TailT...>(table) {}
//
//     void operator()(const udho::hazo::basic_seq_d<basic_action<FunctionT, StrT, MatchT>, TailT...>& chain){
//         tabulize<basic_action<FunctionT, StrT, MatchT>>::operator()(chain.front());
//         tabulize<TailT...>::operator()(chain.tail());
//     }
// };

}
}


#endif // UDHO_URL_TABULATE_H
