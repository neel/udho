// SPDX-FileCopyrightText: 2023 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_SUMMARY_H
#define UDHO_URL_SUMMARY_H

#include <udho/url/fwd.h>
#include <udho/url/detail/format.h>
#include <udho/hazo/string/basic.h>
#include <string>
#include <map>

namespace udho{
namespace url{

namespace summary{

struct mount_point{
    struct url_proxy{
        friend struct mount_point;

        template <typename... Args>
        std::string replace(const std::tuple<Args...>& args) const { return udho::url::format(_replace, args); }
        template <typename... Args>
        std::string operator()(Args&&... args) const {
            return replace(std::tuple<Args...>{args...});
        }
        private:
            inline explicit url_proxy(const std::string& replace): _replace(replace) {}
        private:
            std::string _replace;
    };

    template <typename StrT, typename ActionsT>
    friend struct udho::url::mount_point;

    inline explicit mount_point(const char* name): _name(name) {}

    inline const std::string& name() const { return _name; }

    inline url_proxy operator[](const std::string& key) const {
        auto it = _replacements.find(key);
        if (it == _replacements.end()) {
            throw std::out_of_range("Key not found in replacements");
        }
        assert(key == it->first);
        return url_proxy{it->second};
    }
    template <typename Char, Char... C>
    inline url_proxy operator[](udho::hazo::string::str<Char, C...>&& hstr) const {
        return operator[](hstr.str());
    }

    private:
        std::string _name;
        std::map<std::string, std::string> _replacements;
};

struct router{
    template <typename MountPointT>
    void add(const MountPointT& mp){
        _mount_points.emplace(std::string(mp.name().c_str()), mp.summary());
    }
    inline const summary::mount_point& operator[](const std::string& key) const {
        auto it = _mount_points.find(key);
        if (it == _mount_points.end()) {
            throw std::out_of_range("Mount point not found");
        }
        return it->second;
    }
    template <typename Char, Char... C>
    const summary::mount_point& operator[](udho::hazo::string::str<Char, C...>&& hstr) const {
        return operator[](hstr.str());
    }
    private:
        std::map<std::string, summary::mount_point> _mount_points;
};

}

}
}

#endif // UDHO_URL_SUMMARY_H
