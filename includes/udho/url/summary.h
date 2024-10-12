// SPDX-FileCopyrightText: 2023 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_SUMMARY_H
#define UDHO_URL_SUMMARY_H

#include <udho/url/fwd.h>
#include <udho/url/detail/format.h>
#include <udho/hazo/string/basic.h>
#include <udho/view/data/type.h>
#include <udho/view/data/associative.h>
#include <string>
#include <map>

namespace udho{
namespace url{

namespace summary{

/**
 * @class mount_point
 * @brief Represents a summarized view of a mount point in URL routing, containing replacements and mappings for URLs.
 *
 * The summary::mount_point class provides a simplified, accessible and non-templated way to handle URL replacements based on predefined rules
 * associated with different parts of a URL. It is constructed from a @ref udho::url: mount_point::summary function.
 */
struct mount_point{
    using container_type = std::map<std::string, std::string>;
    using const_iterator = typename container_type::const_iterator;
    using size_type      = typename container_type::size_type;

    /**
     * @class url_proxy
     * @brief Provides a proxy for handling URL replacements in a summarized manner.
     */
    struct url_proxy{
        friend struct mount_point;

        /**
         * @brief Constructs a URL by replacing specified parts using a tuple of arguments using fmt formating scheme.
         * @tparam Args Variadic template arguments, each corresponding to a part of the URL that needs replacement.
         * @param args A tuple containing the values to be used for replacements.
         * @return The fully constructed URL after applying all replacements.
         */
        template <typename... Args>
        std::string replace(const std::tuple<Args...>& args) const { return udho::url::format(_replace, args); }

        /**
         * @brief Simplifies the URL replacement process by allowing direct calls with multiple arguments.
         * @tparam Args Variadic template arguments, each corresponding to a part of the URL that needs replacement.
         * @param args Arguments to be used for URL replacement.
         * @return The fully constructed URL after applying all replacements.
         */
        template <typename... Args>
        std::string operator()(Args&&... args) const {
            return replace(std::tuple<Args...>{args...});
        }

        std::string pattern() const { return _replace; }

        private:
            inline explicit url_proxy(const std::string& replace): _replace(replace) {}
        private:
            std::string _replace;
    };

    template <typename StrT, typename ActionsT>
    friend struct udho::url::mount_point;

    /**
     * @brief Constructs a summary mount point with the specified name.
     * @param name The name of the mount point, typically derived from a compile-time string in the detailed mount_point.
     */
    inline explicit mount_point(const char* name): _name(name) {}

    /**
     * @brief Retrieves the name of the mount point.
     * @return The name as a standard string.
     */
    inline const std::string& name() const { return _name; }

    /**
     * @brief Provides access to a url_proxy (for facilitating URL replacements) by a specific key used in slot while constructing the routing table.
     * @param key The slot key identifying the specific URL pattern or replacement rule.
     * @return A url_proxy instance capable of performing the replacements associated with the given key.
     */
    inline url_proxy url(const std::string& key) const {
        auto it = _replacements.find(key);
        if (it == _replacements.end()) {
            throw std::out_of_range("Key not found in replacements");
        }
        assert(key == it->first);
        return url_proxy{it->second};
    }

    /**
     * @brief Provides access to a url_proxy (for facilitating URL replacements) by a specific key used in slot while constructing the routing table.
     * @param key The slot key identifying the specific URL pattern or replacement rule.
     * @return A url_proxy instance capable of performing the replacements associated with the given key.
     */
    inline url_proxy operator[](const std::string& key) const {
        return url(key);
    }

    /**
     * @brief Enables access to a url_proxy using a compile-time string, enhancing ease of use with compile-time constants.
     * @see url_proxy mount_point::operator[](const std::string& key) const
     * @tparam Char Character type of the compile-time string.
     * @tparam C Characters comprising the compile-time string.
     * @param hstr A compile-time string used as the key for accessing specific URL replacement rules.
     * @return A url_proxy instance associated with the given compile-time string.
     */
    template <typename Char, Char... C>
    inline url_proxy operator[](udho::hazo::string::str<Char, C...>&& hstr) const {
        return operator[](hstr.str());
    }

    const_iterator begin() const { return _replacements.cbegin(); }
    const_iterator end()   const { return _replacements.cend();   }
    size_type      size()  const { return _replacements.size();   }


    friend auto metatype(udho::view::data::type<mount_point>){
        using namespace udho::view::data;

        return assoc("mount_point"),
            index(&mount_point::url),
            iter (&mount_point::begin, &mount_point::end),
            func("url",  &mount_point::url),
            fvar("name", &mount_point::name),
            fvar("size", &mount_point::size);
    }

    private:
        std::string     _name;
        container_type  _replacements;
};

/**
 * @class router
 * @brief Stores a map of mount point summary.
 */
struct router{
    using container_type = std::map<std::string, summary::mount_point>;
    using const_iterator = typename container_type::const_iterator;
    using size_type      = typename container_type::size_type;

    router() = default;
    router(const router& other) = default;

    /**
     * @brief Adds a new mount point to the router summary.
     * @tparam MountPointT The type of the mount point being added.
     * @param mp The mount point instance to add to the router.
     */
    template <typename MountPointT>
    void add(const MountPointT& mp){
        _mount_points.emplace(std::string(mp.name().c_str()), mp.summary());
    }

   /**
     * @brief Retrieves a mount point by its name.
     * @param key The name of the mount point to retrieve.
     * @return The mount point associated with the given name.
     */
    inline const summary::mount_point& route(const std::string& key) const {
        auto it = _mount_points.find(key);
        if (it == _mount_points.end()) {
            throw std::out_of_range("Mount point not found");
        }
        return it->second;
    }

   /**
     * @brief Retrieves a mount point by its name.
     * @param key The name of the mount point to retrieve.
     * @return The mount point associated with the given name.
     */
    inline const summary::mount_point& operator[](const std::string& key) const {
        return route(key);
    }

   /**
     * @brief Retrieves a mount point by its name.
     * @param key The name of the mount point to retrieve.
     * @return The mount point associated with the given name.
     */
    template <typename Char, Char... C>
    const summary::mount_point& operator[](udho::hazo::string::str<Char, C...>&& hstr) const {
        return operator[](hstr.str());
    }

    const_iterator begin() const { return _mount_points.cbegin(); }
    const_iterator end()   const { return _mount_points.cend(); }
    size_type      size()  const { return _mount_points.size(); }

    friend auto metatype(udho::view::data::type<router>){
        using namespace udho::view::data;

        return assoc("router"),
            index(&router::route),
            iter (&router::begin, &router::end),
            fvar("size", &router::size),
            func("route",&router::route);
    }

    private:
        container_type _mount_points;
};

}

}
}


#endif // UDHO_URL_SUMMARY_H
