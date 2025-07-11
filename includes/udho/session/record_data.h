#ifndef UDHO_SESSION_RECORD_DATA_H
#define UDHO_SESSION_RECORD_DATA_H

#include <map>
#include <set>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <udho/session/fwd.h>
#include <udho/utils/traits.h>
#include <udho/session/defs.h>

namespace udho{
namespace session{

/**
 * @struct record_data
 * @brief Core data container for session records
 *
 * @details Stores session data including key-value pairs, metadata, and change tracking
 */
struct record_data{
    using container_type = std::map<std::string, std::string>;          ///< Key-value storage type
    using const_iterator = typename container_type::const_iterator;     ///< Const iterator type
    using size_type      = typename container_type::size_type;          ///< Size type

    template <typename StorageT, udho::session::modes>
    friend struct udho::session::catalogue;

    /**
     * @brief Default constructor, initializes meta information such as creation and last updated time to now and revision to 0.
     */
    inline record_data(): _created(std::chrono::system_clock::now()), _updated(_created), _revision(0) {}

    /**
     * @brief Construct with session ID, initializes meta information such as creation and last updated time to now and revision to 0.
     * @param sessid Session identifier
     */
    inline explicit record_data(const udho::session::id& sessid): _sessid(std::move(sessid)), _created(std::chrono::system_clock::now()), _updated(_created), _revision(0) {}

    /**
     * @brief Get session ID
     * @return sessiion id
     */
    inline const udho::session::id& sessid() const { return _sessid; }

    /**
     * @brief Check if record has modifications that have not been synchronized to the storage yet.
     * @return bool
     */
    inline bool dirty() const { return _updated_fields.size() > 0 || _removed_fields.size() > 0; }

    /**
     * @brief Check if key exists
     * @param key Data key
     * @return true if key exists
     */
    inline bool exists(const std::string& key) const {
        auto it = _container.find(key);
        return (it != _container.end());
    }

    /**
     * @brief Get value for key with type conversion
     * @tparam T Target type (must be ostreamable)
     * @param key Data key
     * @return Converted value
     * @throw std::out_of_range if key doesn't exist
     * @throw boost::bad_lexical_cast if it fails to convert the value (which is serialized to string in the storage) to the target data type
     */
    template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    T get(const std::string& key) const {
        auto it = _container.find(key);
        if(it != _container.end()){
            const std::string& value_str = it->second;
            return boost::lexical_cast<T>(value_str);
        }
        throw std::out_of_range{"out of range key: " + key};
    }

    /**
     * @brief Set value for key
     * @tparam T Value type (must be ostreamable)
     * @param key Data key
     * @param value Data value
     * @param initial Whether this is initial population (skips dirty tracking)
     * @post marks the key as updated
     * @post sets last update time as now for the session data
     * @post removes the key from the deleted keys, if it was previously deleted
     * @post makes the session as dirty untill synchronized with the storage
     */
    template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    void set(const std::string& key, T&& value, bool initial = false) {
        std::string value_str = boost::lexical_cast<std::string>(std::forward<T>(value));

        auto it = _container.find(key);
        if(it != _container.end()){
            it->second = std::move(value_str);
        } else {
            _container.insert(std::make_pair(key, std::move(value_str)));
        }

        if(_removed_fields.count(key)){
            _removed_fields.erase(key);
        }

        if(!initial){
            _updated_fields.insert(key);
            updated(std::chrono::system_clock::now());
        }
    }

    /**
     * @brief Remove a key
     * @param key Data key
     * @return true if key was removed
     * @pre key should exist (noop otherwise)
     * @post marks the key as deleted
     * @post sets last update time as now for the session data
     * @post removes the key from the updated keys, if it was previously updated
     * @post makes the session as dirty untill synchronized with the storage
     */
    bool remove(const std::string& key) {
        auto it = _container.find(key);
        if(it != _container.end()){
            _container.erase(it);
            _removed_fields.insert(key);
            if(_updated_fields.count(key) > 0)
                _updated_fields.erase(key);
            return true;
        }
        return false;
    }

    /// @name Iterators
    /// @{
    inline const_iterator begin() const { return _container.begin(); }   ///< Begin iterator
    inline const_iterator end() const { return _container.end(); }       ///< End iterator
    inline size_type size() const { return _container.size(); }          ///< Number of elements
    /// @}

    /// @name Metadata
    /// @{
    const time_point& created() const { return _created; }                              ///< Get creation time
    record_data& created(const time_point& time) { _created = time; return *this; }     ///< Set creation time

    const time_point& updated() const { return _updated; }                              ///< Get last update time
    record_data& updated(const time_point& time) { _updated = time; return *this; }     ///< Set update time

    std::uint64_t revision() const { return _revision; }                                ///< Get revision number
    record_data& revision(const std::uint64_t& rev) { _revision = rev; return *this; }  ///< Set revision number
    /// @}

    /// @name Change Tracking
    /// @{
    bool is_updated(const std::string& k) const { return _updated_fields.count(k) > 0; } ///< Check if key was updated
    const std::set<std::string>& updated_fields() const { return _updated_fields; }      ///< Get all updated keys
    const std::set<std::string>& removed_fields() const { return _removed_fields; }      ///< Get all removed keys

    /**
     * @brief Synchronize change tracking state
     * @internal must not be called from usercode
     * @param rev New revision number
     * @param time New update time
     */
    void sync(const std::uint64_t& rev, const time_point& time) {
        revision(rev);
        updated(time);
        _removed_fields.clear();
        _updated_fields.clear();
    }
    /// @}

private:
    udho::session::id     _sessid;
    container_type        _container;
    time_point            _created;
    time_point            _updated;
    std::uint64_t         _revision;
    std::set<std::string> _updated_fields;
    std::set<std::string> _removed_fields;
};

}
}

#endif // UDHO_SESSION_RECORD_DATA_H
