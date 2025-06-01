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
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <udho/session/defs.h>
#include <udho/session/storage/features.h>

namespace udho{
namespace session{

template <typename StorageT, udho::session::modes Mode>
struct catalogue: private StorageT{
    static_assert(StorageT::has(Mode), "Storage is incompatible with the specified mode");

    using storage_type      = StorageT;
    using catalog_type      = catalogue<StorageT, Mode>;
    using record_type       = record;
    using record_ptr        = std::unique_ptr<record_type>;
    using record_iptr       = boost::intrusive_ptr<record_type>;
    using key_type          = udho::session::id;
    using container_type    = std::map<key_type, record_ptr>;
    using ref_counts        = std::map<key_type, std::atomic<int>>;
    using note_type         = note;

    template <typename... Args>
    inline explicit catalogue(Args&&... args): storage_type(std::forward<Args>(args)...) {}
    catalogue(const catalogue&) = delete;
    catalogue(catalogue&&) = delete;

    inline storage_type& storage() { return *this; }
    inline const storage_type& storage() const { return *this; }

    note_type borrow(const key_type& sessid) {
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
                if(!_storage_create_load(sessid)){      // create record in storage
                    throw std::runtime_error{"Failed to create session object in the storage"};
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

    protected:
        /**
         * @brief load the record from the memory if it exists on memory, otherwise load it from the storage into the memory.
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

        bool loaded(const key_type& sessid) const {
            std::lock_guard<std::mutex> lock(_mutex);
            return _records.count(sessid) > 0;
        }

    private:
        /**
         * @brief load the record from the storage into _record if it exists in the storage
         */
        bool _storage_load(const key_type& sessid) {
            assert(_records.count(sessid) == 0);
            if(!_storage_exists(sessid)){
                return false;
            }
            return _storage_fetch(sessid);
        }

        bool _storage_exists(const key_type& sessid) { return storage_type::exists(sessid); }

        bool _storage_create_load(const key_type& sessid) {
            auto record = std::make_unique<record_type>(sessid);
            bool result = storage_type::create(*record);
            _records.emplace(sessid, std::move(record));
            _references.emplace(sessid, 0);
            return result;
        }

        /**
         * @brief load the record from the storage into _record assuming it exists in the storage
         */
        bool _storage_fetch(const key_type& sessid) {
            auto record = std::make_unique<record_type>(sessid);
            bool result = storage_type::fetch(*record);
            _records.emplace(sessid, std::move(record));
            _references.emplace(sessid, 0);
            return result;
        }

        bool _storage_serialize(const std::unique_ptr<record_type>& record) {
            bool result = storage_type::save(*record, Mode == udho::session::modes::optimistic);
            _records.erase(record->sessid());
            return result;
        }

    private:
        void release(const key_type& sessid) {
            std::lock_guard<std::mutex> lock(_mutex);

            auto it = _records.find(sessid);
            if(it != _records.end()){
                record_ptr& record = it->second;
                auto refit = _references.find(sessid);
                assert(refit != _references.end());

                if (refit->second.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    try{
                        if(record->_dirty()){
                            _storage_serialize(record);
                        }
                        _records.erase(sessid);
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
        ref_counts     _references;
        mutable std::mutex _mutex;
};

}
}

#endif // UDHO_SESSION_CATALOGUE_H
