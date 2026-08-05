#ifndef UDHO_SESSION_RECORD_H
#define UDHO_SESSION_RECORD_H

#include <map>
#include <string>
#include <mutex>
#include <functional>
#include <udho/session/fwd.h>
#include <udho/session/record_data.h>
#include <udho/session/defs.h>
#include <boost/uuid/uuid.hpp>

namespace udho{
namespace session{

/** @addtogroup DoxyG_session
 *  @{
 */

/**
 * @struct record
 * @brief Thread-safe session record
 * @details Wraps record_data with mutex protection
 * @note all operations are thread safe
 */
struct record: private record_data{
    using record_type    = record;
    using container_type = typename record_data::container_type;
    using const_iterator = typename container_type::const_iterator;
    using size_type      = typename container_type::size_type;
    using notifier_type  = std::function<void (const record_type&)>;

    template <typename StorageT, udho::session::modes>
    friend struct udho::session::catalogue;

    friend struct udho::session::note;

    /**
     * @brief Construct record with an ssid, initializes meta information such as creation and last updated time to now and revision to 0.
     * @param sessid Session ID
     * @param notifier Change notification callback
     * @warning Not to be instantiated by the usercode.
     * @details The notifier is used to notify the storage about any modification, it is upto the storage to decide whether to synchronize
     *          it then or wait for more modifications.
     */
    inline record(const udho::session::id& sessid, notifier_type&& notifier): record_data(sessid), _notifier(std::move(notifier)), _removed(false) {}

    using record_data::sessid;

    protected:

        /**
         * @brief Check if record has modifications that have not been synchronized to the storage yet.
         * @warning Not Thread safe, for internal purpose only, call @ref dirty instead, which is thread safe
         * @return bool
         */
        inline bool _dirty() const { return record_data::dirty(); }

        /**
         * @brief Check if record has modifications that have not been synchronized to the storage yet.
         * @return bool
         */
        inline bool dirty() const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::dirty();
        }

        /**
         * @brief Get revision number
         * @return std::uint64_t
         */
        inline std::uint64_t revision() const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::revision();
        }

        /**
         * @brief Number of fields in this session, excluding meta information such as revision and creation, updation time
         * @return size_type
         */
        inline size_type size() const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::size();
        }

        /**
         * @brief Check if key exists
         * @param key Data key
         * @return true if key exists
         */
        inline bool exists(const std::string& key) const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::exists(key);
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
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::template get<T>(key);
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
         * @throw std::runtime_error if session is marked as removed
         */
        template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
        void set(const std::string& key, T&& value) {
            std::lock_guard<std::mutex> lock(_mutex_data);
            if(_removed) {
                throw std::runtime_error{"trying to perform set key operations in a deleted session"};
            }
            record_data::template set<T>(key, std::move(value));
            if(record_data::dirty() || _removed) {
                _notifier(*this);
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
         * @throw std::runtime_error if session is marked as removed
         */
        bool remove(const std::string& key) {
            std::lock_guard<std::mutex> lock(_mutex_data);
            if(_removed) {
                throw std::runtime_error{"trying to perform remove key operations in a deleted session"};
            }
            bool res = record_data::remove(key);
            if(record_data::dirty() || _removed) {
                _notifier(*this);
            }
            return res;
        }

        /**
         * @brief remove mark the session record as removed
         * @details All set operations on removed session record will throw exception.
         */
        void remove() {
            std::lock_guard<std::mutex> lock(_mutex_data);
            _removed = true;
        }

        /**
         * @brief check whether the session record is flagged as removed or not
         * @return
         */
        bool removed() const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return _removed;
        }

        /**
         * @brief loop through all elements and apply function f
         * @details expect f to accept two string arguments first for the key and second for the value
         * @param f
         */
        template <typename F>
        void visit(F&& f){
            std::lock_guard<std::mutex> lock(_mutex_data);
            for(const_iterator it = begin(); it != end(); ++it){
                f(it->first, it->second);
            }
        }

    private:
        mutable std::mutex _mutex_data;
        notifier_type      _notifier;
        bool               _removed;
};

/** @} */
}
}

#endif // UDHO_SESSION_RECORD_H
