// SPDX-FileCopyrightText: 2024 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_ROUTER_H
#define UDHO_URL_ROUTER_H

#include <udho/hazo/seq/seq.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

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

    template <typename Ch>
    bool find(const std::basic_string<Ch>& pattern) const {
        bool found = false;
        _mountpoints.visit([&pattern, &found](const auto& mointpoint){
            if(found)
                return;
            auto path = mointpoint.path();
            if(!boost::starts_with(pattern, path))
                return;
            auto rest = pattern.substr(path.size());
            found = mointpoint.find(rest);
        });
        return found;
    }

    template <typename Ch>
    bool invoke(const std::basic_string<Ch>& pattern) {
        bool found = false;
        _mountpoints.visit([&pattern, &found](auto& mointpoint){
            if(found)
                return;
            auto path = mointpoint.path();
            if(!boost::starts_with(pattern, path))
                return;
            auto rest = pattern.substr(path.size());
            found = mointpoint.invoke(rest);
        });
        return found;
    }

    private:


    private:
        mountpoints_type _mountpoints;
};

}
}

#endif // ROUTER_H
