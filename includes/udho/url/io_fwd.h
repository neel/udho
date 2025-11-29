#ifndef UDHO_URL_IO_FWD_H
#define UDHO_URL_IO_FWD_H

#include <udho/url/fwd.h>
#include <udho/url/tabulate.h>

namespace udho{
namespace url{

template <typename... Args>
tabulate::Table& operator<<(tabulate::Table& table, const action_table<Args...>& actions);

template <typename... Args>
std::ostream& operator<<(std::ostream& stream, const action_table<Args...>& actions);

template <typename StrT, typename ActionsT>
tabulate::Table& operator<<(tabulate::Table& table, const mount_point<StrT, ActionsT>& point);

template <typename StrT, typename ActionsT>
std::ostream& operator<<(std::ostream& stream, const mount_point<StrT, ActionsT>& point);

template <typename... Args>
tabulate::Table& operator<<(tabulate::Table& table, const mountpoints_table<Args...>& mountpoints);

template <typename... Args>
std::ostream& operator<<(std::ostream& stream, const mountpoints_table<Args...>& mountpoints);

template <typename Mountpoints>
std::ostream& operator<<(std::ostream& stream, const detail::routing_table<Mountpoints>& router);

template <typename Mountpoints = void>
std::ostream& operator<<(std::ostream& stream, const basic_router<Mountpoints>& router);

}
}

#endif // UDHO_URL_IO_FWD_H
