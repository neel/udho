// SPDX-FileCopyrightText: 2024 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_ROUTER_H
#define UDHO_URL_ROUTER_H

#include <udho/hazo/seq/seq.h>

namespace udho{
namespace url{

/**
 * mounts points
 * @tparam MountPointsT udho::hazo::basic_seq_d<mount_point<StrT, ActionsT>, ...>
 */
template <typename MountPointsT>
struct router{
    using mountpoints_type = MountPointsT;

    router(mountpoints_type&& mountpoints): _mountpoints(std::move(mountpoints)) {}

    template <typename XStrT>
    auto& operator[](XStrT&& xstr) { return _mountpoints[std::move(xstr)]; }

    template <typename XStrT>
    const auto& operator[](XStrT&& xstr) const { return _mountpoints[std::move(xstr)]; }

    private:


    private:
        mountpoints_type _mountpoints;
};

}
}

#endif // ROUTER_H
