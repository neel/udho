#ifndef UDHO_URL_TABLES_H
#define UDHO_URL_TABLES_H

#include <udho/url/action.h>
#include <udho/url/mount.h>
#include <udho/hazo/seq/seq.h>
#include <udho/hazo/string/basic.h>

namespace udho{
namespace url{

/**
 * @addtogroup DoxyG_url
 * @{
 */

/**
 * @brief Ordered heterogeneous collection of router actions.
 *
 * An action table stores one or more basic_action objects by value while
 * preserving their compile-time types and insertion order. Actions may be
 * accessed by their compile-time key or by their zero-based position.
 *
 * Action tables can be constructed and concatenated with the router
 * concatenation operator.
 *
 * @tparam Actions Types of the actions stored in the table.
 *
 * @note The table is intended to contain basic_action instances.
 *
 * @see udho::url::operator|
 */
template <typename... Actions>
class action_table: private udho::hazo::basic_seq<udho::hazo::by_data, Actions...>{
    using sequence_type = udho::hazo::basic_seq<udho::hazo::by_data, Actions...>;

    template <typename... XActions>
    friend class action_table;

    template <typename T>
    using extend = typename sequence_type::template extend<T>;
    const sequence_type& sequence() const { return *this; }

public:
    using sequence_type::visit;
    using sequence_type::visit_at;
    using sequence_type::depth;

    /**
     * @brief Constructs a table from a pack of actions.
     *
     * The supplied actions are moved into the underlying heterogeneous
     * sequence in argument order.
     *
     * @param actions Actions to store in the table.
     */
    action_table(Actions&&... actions): sequence_type(std::forward<Actions>(actions)...) {}

    /**
     * @brief Constructs a table from an existing heterogeneous sequence.
     *
     * @param seq Sequence whose elements are moved into the table.
     */
    action_table(udho::hazo::basic_seq<udho::hazo::by_data, Actions...>&& seq): sequence_type(std::forward<udho::hazo::basic_seq<udho::hazo::by_data, Actions...>>(seq)) {}

    /**
     * @brief Appends an action to the table.
     *
     * Creates a new action table whose first elements are the actions in this
     * table and whose final element is @p right.
     *
     * This operation is also exposed through operator| when an action appears on
     * the right-hand side of an action table.
     *
     * @tparam RFunctionT Callable type used by the appended action.
     * @tparam RStrT Compile-time key string type of the appended action.
     * @tparam RMatchT URL matching rule type of the appended action.
     *
     * @param right Action to append.
     *
     * @return A new action_table containing the existing actions followed by
     *         @p right.
     *
     * @see udho::url::operator|
     */
    template <typename RFunctionT, typename RStrT, typename RMatchT>
    action_table<Actions..., basic_action<RFunctionT, RStrT, RMatchT>> append(basic_action<RFunctionT, RStrT, RMatchT>&& right) const {
        using rhs_type = basic_action<RFunctionT, RStrT, RMatchT>;
        using result_type = action_table<Actions..., rhs_type>;
        return result_type{extend<rhs_type>(sequence(), std::move(right))};
    }

    /**
     * @brief Concatenates another action table after this table.
     *
     * The ordering of both tables is preserved. All actions in this table
     * precede all actions in @p other.
     *
     * This operation is also exposed through operator| when both operands are
     * action tables.
     *
     * @tparam XActions Types of the actions stored in the other table.
     *
     * @param other Action table to append.
     *
     * @return A new action_table containing the actions from both tables.
     *
     * @see udho::url::operator|
     */
    template <typename... XActions>
    action_table<Actions..., XActions...> concat(const action_table<XActions...>& other) const {
        return action_table<Actions..., XActions...>{sequence_type::concat(other.sequence())};
    }

    /**
     * @brief Retrieves an action by its compile-time key.
     *
     * @tparam CharT Character type used by the key.
     * @tparam X Characters forming the compile-time key.
     *
     * @param key Compile-time action key.
     *
     * @return Mutable reference to the action associated with @p key.
     */
    template <typename CharT, CharT... X>
    auto& operator[](const udho::hazo::string::str<CharT, X...>& key) { return sequence_type::operator[](key); }

    /**
     * @brief Retrieves an action by its compile-time key.
     *
     * @tparam CharT Character type used by the key.
     * @tparam X Characters forming the compile-time key.
     *
     * @param key Compile-time action key.
     *
     * @return Constant reference to the action associated with @p key.
     */
    template <typename CharT, CharT... X>
    const auto& operator[](const udho::hazo::string::str<CharT, X...>& key) const { return sequence_type::operator[](key); }

    /**
     * @brief Retrieves an action by its zero-based position.
     *
     * @tparam N Zero-based index of the requested action.
     *
     * @return Constant reference to the action at position @p N.
     */
    template <std::size_t N>
    const auto& at() const { return sequence_type::template value<N>(); }

    /**
     * @brief Returns the number of actions stored in the table.
     *
     * @return Number of action elements in the table.
     */
    constexpr std::size_t length() const { return 1+ sequence_type::depth; }
};

/**
 * @brief Compile-time trait identifying action_table types.
 *
 * The primary template evaluates to `std::false_type`.
 *
 * @tparam T Type to inspect.
 */
template <typename>
struct is_action_table: std::false_type{};

/**
 * @brief Compile-time trait specialization for action_table.
 *
 * @tparam Actions Types stored in the action table.
 *
 * This specialization evaluates to `std::true_type`.
 */
template <typename... Actions>
struct is_action_table<action_table<Actions...>>: std::true_type{};

/**
 * @brief Ordered heterogeneous collection of router mount points.
 *
 * A mount-points table stores one or more mount_point objects by value while
 * preserving their compile-time types and insertion order. Mount points may be
 * accessed by their compile-time key or by their zero-based position.
 *
 * The interface mirrors action_table and exposes the visitation operations of
 * its underlying heterogeneous sequence.
 *
 * Mount-points tables can be constructed, extended, prepended, and
 * concatenated with the router concatenation operator.
 *
 * @tparam Mountpoints Types of the mount points stored in the table.
 *
 * @note The table is intended to contain mount_point instances.
 *
 * @see udho::url::operator|
 */
template <typename... Mountpoints>
class mountpoints_table: private udho::hazo::basic_seq<udho::hazo::by_data, Mountpoints...>{
    using sequence_type = udho::hazo::basic_seq<udho::hazo::by_data, Mountpoints...>;

    template <typename... XMountpoints>
    friend class mountpoints_table;

    template <typename T>
    using extend = typename sequence_type::template extend<T>;
    const sequence_type& sequence() const { return *this; }

public:
    using sequence_type::visit;
    using sequence_type::visit_at;
    using sequence_type::depth;

    /**
     * @brief Constructs a table from a pack of mount points.
     *
     * The supplied mount points are moved into the underlying heterogeneous
     * sequence in argument order.
     *
     * @param mountpoints Mount points to store in the table.
     */
    mountpoints_table(Mountpoints&&... mountpoints): sequence_type(std::forward<Mountpoints>(mountpoints)...) {}

    /**
     * @brief Constructs a table from an existing heterogeneous sequence.
     *
     * @param seq Sequence whose elements are moved into the table.
     */
    mountpoints_table(udho::hazo::basic_seq<udho::hazo::by_data, Mountpoints...>&& seq): sequence_type(std::forward<udho::hazo::basic_seq<udho::hazo::by_data, Mountpoints...>>(seq)) {}

    /**
     * @brief Appends a mount point to the table.
     *
     * Creates a new mount-points table whose first elements are the mount points
     * in this table and whose final element is @p right.
     *
     * This operation is also exposed through operator| when a mount point appears
     * on the right-hand side of a mount-points table.
     *
     * @tparam RStrT Compile-time name string type of the appended mount point.
     * @tparam RActionsT Action collection type of the appended mount point.
     *
     * @param right Mount point to append.
     *
     * @return A new mountpoints_table containing the existing mount points
     *         followed by @p right.
     *
     * @see udho::url::operator|
     */
    template <typename RStrT, typename RActionsT>
    mountpoints_table<Mountpoints..., mount_point<RStrT, RActionsT>> append(mount_point<RStrT, RActionsT>&& right) const {
        using rhs_type = mount_point<RStrT, RActionsT>;
        using result_type = mountpoints_table<Mountpoints..., rhs_type>;
        return result_type{extend<rhs_type>(sequence(), std::move(right))};
    }

    /**
     * @brief Concatenates a constant mount-points table after this table.
     *
     * The ordering of both tables is preserved. All mount points in this table
     * precede all mount points in @p other.
     *
     * This operation is also exposed through operator| when both operands are
     * mount-points tables.
     *
     * @tparam XMountpoints Types of the mount points stored in the other table.
     *
     * @param other Mount-points table to append.
     *
     * @return A new mountpoints_table containing the mount points from both
     *         tables.
     *
     * @see udho::url::operator|
     */
    template <typename... XMountpoints>
    mountpoints_table<Mountpoints..., XMountpoints...> concat(const mountpoints_table<XMountpoints...>& other) const {
        return mountpoints_table<Mountpoints..., XMountpoints...>{sequence_type::concat(other.sequence())};
    }

    /**
     * @brief Concatenates an rvalue mount-points table after this table.
     *
     * The ordering of both tables is preserved. All mount points in this table
     * precede all mount points in @p other.
     *
     * This overload is used by operator| when a mount point is prepended to an
     * existing mount-points table.
     *
     * @tparam XMountpoints Types of the mount points stored in the other table.
     *
     * @param other Mount-points table to append.
     *
     * @return A new mountpoints_table containing the mount points from both
     *         tables.
     *
     * @see udho::url::operator|
     */
    template <typename... XMountpoints>
    mountpoints_table<Mountpoints..., XMountpoints...> concat(mountpoints_table<XMountpoints...>&& other) const {
        return mountpoints_table<Mountpoints..., XMountpoints...>{sequence_type::concat(other.sequence())};
    }

    /**
     * @brief Retrieves a mount point by its compile-time key.
     *
     * @tparam CharT Character type used by the key.
     * @tparam X Characters forming the compile-time key.
     *
     * @param key Compile-time mount-point key.
     *
     * @return Mutable reference to the mount point associated with @p key.
     */
    template <typename CharT, CharT... X>
    auto& operator[](const udho::hazo::string::str<CharT, X...>& key) { return sequence_type::operator[](key); }

    /**
     * @brief Retrieves a mount point by its compile-time key.
     *
     * @tparam CharT Character type used by the key.
     * @tparam X Characters forming the compile-time key.
     *
     * @param key Compile-time mount-point key.
     *
     * @return Constant reference to the mount point associated with @p key.
     */
    template <typename CharT, CharT... X>
    const auto& operator[](const udho::hazo::string::str<CharT, X...>& key) const { return sequence_type::operator[](key); }

    /**
     * @brief Retrieves a mount point by its zero-based position.
     *
     * @tparam N Zero-based index of the requested mount point.
     *
     * @return Constant reference to the mount point at position @p N.
     */
    template <std::size_t N>
    const auto& at() const { return sequence_type::template value<N>(); }

    /**
     * @brief Returns the number of mount points stored in the table.
     *
     * @return Number of mount-point elements in the table.
     */
    constexpr std::size_t length() const { return 1+ sequence_type::depth; }
};


/**
 * @brief Compile-time trait identifying mountpoints_table types.
 *
 * The primary template evaluates to `std::false_type`.
 *
 * @tparam T Type to inspect.
 */
template <typename>
struct is_mountpoints_table: std::false_type{};

/**
 * @brief Compile-time trait specialization for mountpoints_table.
 *
 * @tparam Mountpoints Types stored in the mount-points table.
 *
 * This specialization evaluates to `std::true_type`.
 */
template <typename... Mountpoints>
struct is_mountpoints_table<mountpoints_table<Mountpoints...>>: std::true_type{};

namespace detail{

/**
 * @brief Internal compile-time validator for action-table types.
 *
 * The primary template evaluates to `std::false_type`.
 *
 * @tparam T Type to inspect.
 *
 * @internal
 */
template <typename T>
struct is_action_table : std::false_type {};

/**
 * @brief Internal validator specialization for action_table.
 *
 * Evaluates to `std::true_type` only when every element in the table satisfies
 * detail::is_basic_action.
 *
 * @tparam Actions Types stored in the action table.
 *
 * @internal
 */
template <typename... Actions>
struct is_action_table<action_table<Actions...>> : std::bool_constant<std::conjunction_v<detail::is_basic_action<Actions>...>> {};

}


/// @}

}
}

#endif // UDHO_URL_TABLES_H
