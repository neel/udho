// SPDX-FileCopyrightText: 2023 Neel Basu <email>
// SPDX-License-Identifier: BSD-3-Clause

#ifndef UDHO_URL_MOUNT_H
#define UDHO_URL_MOUNT_H

#include <string>
#include <udho/url/fwd.h>
#include <udho/hazo/string/basic.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <udho/url/action.h>
#include <udho/url/summary.h>

namespace udho{
namespace url{

/**
 * @addtogroup DoxyG_url
 * @{
 */

template <typename StrT, typename ActionsT>
struct mount_point;

/**
 * @brief Manages a collection of URL actions, organizing them as a mount point in a web application's URL space.
 *
 * The `mount_point` class acts as a container and manager for actions associated with specific URL patterns.
 * Each action is tagged with a compile-time hash and is associated with a specific pattern matching mechanism,
 * such as regex or fixed string patterns. This class facilitates the grouping of these actions under a common
 * base URL path, allowing for structured and organized URL handling.
 *
 * @note Use udho::url::mount function to instantiate instead.
 *
 * @tparam StrT Compile-time string type used for identifying the mount_point.
 * @tparam ActionsT A sequence type (typically a variant or tuple) that holds different action types.
 *
 * @code{.cpp}
 * auto chain = udho::url::slot("f0"_h,  &f0) << udho::url::home(udho::url::verb::get) |
 *              udho::url::slot("f1"_h,  &f1) << udho::url::regx(udho::url::verb::get, "/f1/(\\w+)/(\\d+)", "/f1/{}/{}");
 * auto mount_point = udho::url::mount{"chain"_h, "/pchain", std::move(chain)};
 * std::cout << mount_point.find("/f1/john/42") << std::endl; // Outputs: true
 * @endcode
 */
template <typename StrT, typename... Actions>
struct mount_point<StrT, udho::url::action_table<Actions...>>{
    using actions_type = udho::url::action_table<Actions...>;
    using name_type    = StrT;
    using key_type     = StrT;

    static constexpr const std::size_t depth = actions_type::depth;

    /**
     * Constructs a mount_point with a name, base path, and actions.
     * @param name Compile-time string name of the mount point.
     * @param path Base URL path to which this mount point is mapped.
     * @param actions Actions associated with this mount point.
     */
    mount_point(name_type&& name, const std::string& path, actions_type&& actions): _name(std::move(name)), _path(path), _actions(std::move(actions)), _summary(_name.c_str(), _path) {
        check();
        summarize();
    }

    /**
     * Retrieves the compile-time key of the mount_point.
     * @return Compile-time string key.
     */
    static constexpr key_type key() { return key_type{}; }

    /**
     * @brief number of actions in the action table
     * @return std::size_t
     */
    static constexpr std::size_t length() { return 1+ depth; }

    /**
     * Accessor for the mount point's name.
     * @return Reference to the compile-time string name.
     */
    const name_type& name() const { return _name; }
    /**
     * Accessor for the base URL path of the mount point.
     * @return The base URL path as a standard string.
     */
    const std::string& path() const { return _path; }
    /**
     * Accessor for the actions stored in this mount point.
     * @return Constant reference to the container of actions.
     */
    const actions_type& actions() const { return _actions; }
    /**
     * Accessor for mutable actions stored in this mount point.
     * @return Reference to the container of actions.
     */
    actions_type& actions() { return _actions; }

    /**
     * @brief Accesses a mutable reference to an action associated with a given compile-time string identifier.
     *
     * This operator provides direct access to actions stored within the mount point, allowing modifications
     * to the action if needed. It is templated to accept any compile-time string type, enabling flexible use
     * with different string implementations.
     *
     * @tparam XStrT The type of the compile-time string, which uniquely identifies an action.
     * @param xstr A forward reference to the compile-time string representing the action's key.
     * @return A reference to the action associated with the given key.
     *
     * @code{.cpp}
     * auto& action = mount_point["f1"_h]; // Accesses the action associated with the "f1" key
     * @endcode
     */
    template <typename XStrT>
    auto& operator[](XStrT&& xstr) { return _actions[std::move(xstr)]; }

    /**
     * @brief Accesses a constant reference to an action associated with a given compile-time string identifier.
     *
     * Similar to the non-const version, this operator allows accessing actions stored within the mount point
     * in a read-only fashion. It ensures that actions cannot be modified through the returned reference.
     *
     * @tparam XStrT The type of the compile-time string, which uniquely identifies an action.
     * @param xstr A forward reference to the compile-time string representing the action's key.
     * @return A constant reference to the action associated with the given key.
     *
     * @code{.cpp}
     * const auto& action = mount_point["f1"_h]; // Accesses the action associated with the "f1" key for read-only operations
     * @endcode
     */
    template <typename XStrT>
    const auto& operator[](XStrT&& xstr) const { return _actions[std::move(xstr)]; }

    /**
     * @brief Finds if a given URL matches any of the actions in the mount point.
     * @tparam Ch Character type of the URL string.
     * @param subject URL to be matched.
     * @return index of the matched element, -1 if not fouond
     */
    int index_of(boost::beast::http::verb method, const std::string& subject) const {
        int index = -1;
        _actions.visit_at([method, &subject, &index](const auto& action, std::size_t depth){
            if(index >= 0) return;
            if(action.find(method, subject)){
                index = depth;
            }
        });
        return index;
    }

    /**
     * @brief Invokes the appropriate action at a given index.
     * @param subject URL to be processed.
     * @param args Arguments to pass to the action handler.
     * @return True if an action was successfully invoked, otherwise false.
     */
    template <typename... Args>
    bool invoke_at(int index, const std::string& subject, Args&&... args) const {
        assert(index > -1);
        bool found = false;
        _actions.visit_at([index, &subject, &found, &args...](auto& action, std::size_t depth){
            if(found) return;
            found = (depth == index);
            if(found) {
                action.invoke(subject, std::forward<Args>(args)...);
            }
        });
        return found;
    }

    /**
     * @brief reconfigures the config superset by applying the options (subset) associated with action at the given index
     * @param index
     * @param config superset
     * @return bool
     */
    template <typename ConfigSupersetT>
    bool reconfigure_for(int index, ConfigSupersetT& config) const {
        assert(index > -1);
        bool found = false;
        bool result = false;
        _actions.visit_at([index, &found, &result, &config](auto& action, std::size_t depth){
            if(found) return;
            found = (depth == index);
            if(found) {
                result = action.options().apply(config);
            }
        });
        return found && result;
    }

    /**
     * Finds if a given URL matches any of the actions in the mount point.
     * @tparam Ch Character type of the URL string.
     * @param subject URL to be matched.
     * @return True if any action matches the URL, otherwise false.
     */
    template <typename Ch>
    bool find(boost::beast::http::verb method, const std::basic_string<Ch>& subject) const {
        bool found = false;
        _actions.visit([method, &subject, &found](const auto& action){
            if(found) return;
            found = action.find(method, subject);
        });
        return found;
    }

    /**
     * Invokes the appropriate action based on the given URL and arguments.
     * @tparam Ch Character type of the URL string.
     * @tparam Args Types of arguments passed to the action.
     * @param subject URL to be processed.
     * @param args Arguments to pass to the action handler.
     * @return True if an action was successfully invoked, otherwise false.
     */
    template <typename Ch, typename... Args>
    bool invoke(boost::beast::http::verb method, const std::basic_string<Ch>& subject, Args&&... args) const {
        bool found = false;
        _actions.visit([method, &subject, &found, &args...](auto& action){
            if(found) return;
            found = action.invoke(method, subject, std::forward<Args>(args)...);
        });
        return found;
    }

    /**
     * Formats and returns a complete URL for a specific action using provided arguments.
     * @tparam XStrT Type of the compile-time string key for the action.
     * @tparam X Types of the arguments for URL generation.
     * @param xstr Compile-time string key for the action.
     * @param x Arguments to be formatted into the URL.
     * @return A complete URL string.
     */
    template <typename XStrT, typename... X>
    std::string operator()(XStrT&& xstr, X&&... x) const {
        return format("{}{}", _path, operator[](std::move(xstr))(x...));
    }

    /**
     * Fills the URL pattern for a specific action using a tuple of arguments.
     * @tparam XStrT Type of the compile-time string key for the action.
     * @tparam X Types of the arguments wrapped in a tuple for URL generation.
     * @param xstr Compile-time string key for the action.
     * @param x Tuple containing arguments to be formatted into the URL.
     * @return A complete URL string.
     */
    template <typename XStrT, typename... X>
    std::string fill(XStrT&& xstr, std::tuple<X...>&& x) const {
        return format("{}{}", _path, operator[](std::move(xstr)).fill(std::move(x)));
    }

    const summary::mount_point& summary() const { return _summary; }
    private:
        void summarize(){
            auto& replacements = _summary._replacements;
            auto& actions      = _summary._actions;
            _actions.visit([&replacements, &actions](auto& action){
                replacements.emplace(action.key().c_str(), action.match().replacement());
                actions.emplace(action.key().c_str(), action);
            });
        }
        void check(){
            if(_path.front() != '/'){
                throw std::invalid_argument(format("the mountpoint path ({}) must start with /", _path));
            }
            if(_path.size() > 1 && _path.back() == '/'){
                throw std::invalid_argument(format("the mountpoint path ({}) must not end with a /", _path));
            }
        }
    private:
        name_type    _name;
        std::string  _path;
        actions_type _actions;
        summary::mount_point _summary;
};

/**
 * @brief Creates a mount_point with a specified name, path, and actions.

 * @tparam CharT Character type for the compile-time string.
 * @tparam C Characters of the compile-time string.
 * @param name Compile-time string representing the name of the mount point.
 * @param path Base URL path for the mount point.
 * @tparam Actions... Actions stored in the actions table.
 *
 * @param actions Table of actions associated with the mount point.
 * @return A fully configured mount_point object.
 */
template <typename CharT, CharT... C, typename... Actions>
mount_point<udho::hazo::string::str<CharT, C...>, udho::url::action_table<Actions...>> mount(udho::hazo::string::str<CharT, C...>&& name, const std::string& path, udho::url::action_table<Actions...>&& actions){
    return mount_point<udho::hazo::string::str<CharT, C...>, udho::url::action_table<Actions...>>{std::move(name), path, std::move(actions)};
}

/**
 * @brief Creates a mount_point with a specified name, path, and actions.
 *
 * Overload for a single action
 *
 * @tparam ActionsT Type of the action.
 * @tparam CharT Character type for the compile-time string.
 * @tparam C Characters of the compile-time string.
 * @param name Compile-time string representing the name of the mount point.
 * @param path Base URL path for the mount point.
 *
 * @param action action bound to the mount point.
 * @return A fully configured mount_point object.
 */
template <typename CharT, CharT... C, typename F, CharT... X, typename MatchT>
mount_point<udho::hazo::string::str<CharT, C...>, udho::url::action_table<basic_action<F, udho::hazo::string::str<CharT, X...>, MatchT>>> mount(udho::hazo::string::str<CharT, C...>&& name, const std::string& path, basic_action<F, udho::hazo::string::str<CharT, X...>, MatchT>&& action){
    return mount_point<udho::hazo::string::str<CharT, C...>, udho::url::action_table<basic_action<F, udho::hazo::string::str<CharT, X...>, MatchT>>>{std::move(name), path, udho::url::action_table(std::move(action))};
}


/**
 * @brief Creates a root mount_point, representing the base of the URL space.
 * @tparam ActionsT Type of the actions container.
 * @param actions Container of actions associated with the root.
 * @return A root mount_point object.
 */
template <typename ActionsT>
mount_point<udho::hazo::string::str<char, 'r', 'o', 'o', 't'>, ActionsT> root(ActionsT&& actions){
    using namespace udho::hazo::string::literals;
    return mount_point<udho::hazo::string::str<char, 'r', 'o', 'o', 't'>, ActionsT>{"root"_h, "/", std::move(actions)};
}

namespace detail {

template <typename T>
struct is_mount_point : std::false_type {};

template <typename StrT, typename ActionsT>
struct is_mount_point<mount_point<StrT, ActionsT>> : std::true_type {
    using type = mount_point<StrT, ActionsT>;
};


}

/// @}

}
}

#endif // UDHO_URL_MOUNT_H
