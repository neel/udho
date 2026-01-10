#ifndef UDHO_SESSION_CATALOGUE_H
#define UDHO_SESSION_CATALOGUE_H

#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <stdexcept>
#include <boost/intrusive_ptr.hpp>
#include <udho/utils/traits.h>
#include <boost/lexical_cast.hpp>
#include <udho/session/fwd.h>
#include <udho/session/defs.h>
#include <udho/session/note.h>
#include <udho/session/abstract_catalogue.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <udho/session/storage/features.h>
#include <udho/utils/format.h>

namespace udho{
namespace session{


/**
 * @class catalogue
 * @brief Central session management system with configurable persistence strategies
 *
 * @tparam StorageT Storage backend type providing persistence operations
 * @tparam Mode Synchronization mode (lazy, optimistic or immediate)
 *
 * The catalogue manages the lifecycle of session records, providing:
 * - Creation, loading and saving from storage
 * - Reference counting
 * - Configurable persistence strategies
 * - Thread-safe access
 *
 * @note Storage backend must satisfy the required interface for the selected mode
 */
template <typename StorageT, udho::session::modes Mode>
struct catalogue: abstract_catalogue{
    static_assert(StorageT::has(Mode), "Storage is incompatible with the specified mode");

    using storage_type      = StorageT;                                 ///< Storage backend type
    using catalog_type      = catalogue<StorageT, Mode>;
    using record_type       = record;                                   ///< Session record type
    using record_ptr        = std::unique_ptr<record_type>;             ///< Unique ownership pointer
    using record_iptr       = boost::intrusive_ptr<record_type>;
    using key_type          = udho::session::id;                        ///< Session identifier type
    using container_type    = std::map<key_type, record_ptr>;           ///< Active sessions container
    using ref_counts        = std::map<key_type, std::atomic<int>>;
    using note_type         = note;                                     ///< Session access handle type
    using ptr               = std::unique_ptr<catalog_type>;

    /**
     * @brief Construct catalogue with storage arguments
     * @tparam Args Storage backend constructor argument types
     * @param args Arguments forwarded to storage backend
     *
     * @par Example:
     * @code
     * using catalogue = udho::session::catalogue<udho::session::storage::fs, udho::session::modes::lazy>;
     * udho::utils::filesystem::path root = udho::utils::filesystem::current_path();
     * auto cat = catalogue::create(udho::session::storage::fs{root});
     * @endcode
     */
    static std::unique_ptr<catalog_type> create(storage_type&& storage) {
        return std::make_unique<catalog_type>(std::forward<storage_type>(storage));
    }

    inline explicit catalogue(storage_type&& storage): abstract_catalogue(Mode), _storage(std::move(storage)) {}

    catalogue(const catalogue&) = delete;   ///< Non-copyable
    catalogue(catalogue&&) = delete;        ///< Non-movable

    /// @brief Access storage backend
    inline storage_type& storage() { return _storage; }

    /// @brief Access storage backend (const)
    inline const storage_type& storage() const { return _storage; }

    /**
     * @brief Acquire session access handle
     * @param sessid Session identifier
     * @param expect_existing if true expects the session to exist already, otherwise throws exception
     * @return note_type Session access handle
     *
     * @par Workflow:
     * Borrows a session record and provides a @ref note refering to the borrowed @ref record.
     * Multiple notes may refer to the same borrowed @ref record at the same time in a thread
     * safe way. In lazy or optimistic mode, destruction of the last note triggers serialization
     * of the record through the storage. However, if immediate mode is used then the updated
     * fields are synchronized immediately.
     *
     * @throws std::runtime_error on storage failures
     * @note Thread-safe through internal locking
     */
    virtual note_type borrow(const key_type& sessid, bool expect_existing = false) override {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _records.find(sessid);
        if(it != _records.end()){                       // record already loaded
            record_ptr& record = it->second;
            auto refit = _references.find(sessid);
            assert(refit != _references.end());
            note_type note(record, std::bind(&catalog_type::release, this, std::placeholders::_1));      // create a note from the record and return
            refit->second.fetch_add(1, std::memory_order_relaxed);
            return note;
        } else {                                        // record not loaded
            if(!_storage_exists(sessid)){               // record does not exists in storage
                if(expect_existing) {
                    throw std::runtime_error{udho::utils::format("session {} doesn't exist", udho::session::to_string(sessid))};
                } else {
                    if(!_storage_create_load(sessid)){  // create record in storage
                        throw std::runtime_error{"Failed to create session object in the storage"};
                    }
                }
            } else {                                    // record exists in storage
                if(!_storage_load(sessid)){             // load record from the storage
                    throw std::runtime_error{"Failed to load session object from the storage"};
                }
            }
        }

        // session record is loaded

        auto rit = _records.find(sessid);               // search for it again
        assert(rit != _records.end());                  // expect the record to be loaded because storage_load has been called
        record_ptr& record = rit->second;

        auto refit = _references.find(sessid);
        assert(refit != _references.end());
        note_type note(record, std::bind(&catalog_type::release, this, std::placeholders::_1));      // create a note from the record and return
        refit->second.fetch_add(1, std::memory_order_relaxed);

        return note;         // create a note from the record and return
    }

    /**
     * @brief remove marks a session as to_be_removed
     * @details does not delete the session immediately from the storage
     *          rather waits for the last note to be destroyed. Any set
     *          or remove operation on a note referencing a session which
     *          is marked to be removed will lead to std::runtime_errror
     *          exception being thrown.
     * @param sessid
     */
    virtual void remove(const key_type& sessid) override {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _records.find(sessid);
        if(it != _records.end()){
            record_ptr& record = it->second;
            record->remove();
        }
    }

    /**
     * @brief checks whether a session exists for the given sessid
     * @param sessid
     * @return boolean
     */
    virtual bool exists(const key_type& sessid) override {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _records.find(sessid);
        if(it != _records.end()){
            return true;
        } else {
            return _storage_exists(sessid);
        }
    }

    protected:
        /**
         * @brief load the record from the memory if it exists on memory, otherwise load it from the storage into the memory.
         * @param sessid Session identifier
         * @return true if loaded, false if already present
         */
        bool load(const key_type& sessid) {
            std::lock_guard<std::mutex> lock(_mutex);
            if(_records.count(sessid) == 0){
                bool result = _storage_load(sessid);
                if(result) {
                    assert(_records.count(sessid) == 0);
                    _references.insert(std::make_pair(sessid, 0));
                }
                return result;
            } else {
                return false;
            }
        }

        /**
         * @brief Check if session is loaded in memory
         * @param sessid Session identifier
         * @return true if loaded, false otherwise
         */
        bool loaded(const key_type& sessid) const {
            std::lock_guard<std::mutex> lock(_mutex);
            return _records.count(sessid) > 0;
        }

    private:

        /**
         * @brief Load session from storage into memory
         * @param sessid Session identifier
         * @return true on success, false on failure
         *
         * @pre Session must exist in storage
         * @post Record added to _records container
         */
        bool _storage_load(const key_type& sessid) {
            assert(_records.count(sessid) == 0);
            if(!_storage_exists(sessid)){
                return false;
            }
            return _storage_fetch(sessid);
        }

        /// @brief Check if session exists in persistent storage
        bool _storage_exists(const key_type& sessid) { return _storage.exists(sessid); }

        /**
         * @brief Create new session in storage and load into memory
         * @param sessid Session identifier
         * @return true on success, false on failure
         */
        bool _storage_create_load(const key_type& sessid) {
            auto record = std::make_unique<record_type>(sessid, std::bind(&catalog_type::notify, this, std::placeholders::_1));
            bool result = _storage.create(*record);
            _records.emplace(sessid, std::move(record));
            _references.emplace(sessid, 0);
            return result;
        }

        /**
         * @brief load the record from the storage into _record assuming it exists in the storage
         * @param sessid Session identifier
         * @return true on success, false on failure
         */
        bool _storage_fetch(const key_type& sessid) {
            auto record = std::make_unique<record_type>(sessid, std::bind(&catalog_type::notify, this, std::placeholders::_1));
            bool result = _storage.fetch(*record);
            _records.emplace(sessid, std::move(record));
            _references.emplace(sessid, 0);
            return result;
        }

        /**
         * @brief Serialize record to storage
         * @param record Record to serialize
         * @return true on success, false on failure
         *
         * @note In optimistic mode, only dirty records are saved
         */
        bool _storage_serialize(const std::unique_ptr<record_type>& record) {
            bool result = _storage.save(*record, Mode == udho::session::modes::optimistic);
            _records.erase(record->sessid());
            return result;
        }

        bool _storage_remove(const std::unique_ptr<record_type>& record) {
            _records.erase(record->sessid());
            bool result = _storage.remove(*record);
            return false;
        }

    private:

        /**
         * @brief Handle record modification notifications
         * @param record Modified record
         */
        void notify(const record_type& record) {
            if constexpr (Mode == udho::session::modes::immediate) {
                bool result = _storage.save(record);
            }
        }

        /**
         * @brief Release reference to session record
         * @param sessid Session identifier
         * @details decrements reference count, if reference count is 1 (implying the only note pointing
         *          to the same record) then 1. triggers serialization 2. removal from catalog
         * @note does not serialize if the record is not dirty.
         * @throws std::out_of_range if session not loaded
         */
        void release(const key_type& sessid) {
            std::lock_guard<std::mutex> lock(_mutex);

            auto it = _records.find(sessid);
            if(it != _records.end()){
                record_ptr& record = it->second;
                auto refit = _references.find(sessid);
                assert(refit != _references.end());

                if (refit->second.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    try{
                        if(record->removed()) {
                            _records.erase(sessid);
                            _storage_remove(record);
                        } else {
                            if(record->_dirty()){
                                _storage_serialize(record);
                            }
                            _records.erase(sessid);
                        }
                    } catch(const std::exception& ex) {
                        // TODO log fatal error
                    } catch (...) {
                        // TODO log fatal error
                    }
                }
            }else {
                throw std::out_of_range{"Trying to save a session which has not yet been loaded"};
            }
        }

    private:
        container_type _records;
        storage_type   _storage;
        ref_counts     _references;
        mutable std::mutex _mutex;
};

}
}

#endif // UDHO_SESSION_CATALOGUE_H
