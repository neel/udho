#ifndef UDHO_SESSION_NOTE_H
#define UDHO_SESSION_NOTE_H

#include <string>
#include <memory>
#include <functional>
#include <udho/utils/traits.h>
#include <udho/session/fwd.h>
#include <udho/session/defs.h>
#include <udho/session/record.h>

namespace udho{
namespace session{

/**
 * @brief The note class
 * A note can only be created by the catalog. It is created using a reference to the unique_ptr to the record.
 * All get and set calls to the note are forwarded to the record. While constructing the note, catalog passes
 * a release callback which is called when a note is destroyed. That release callback checks whether it is the
 * last note corresponding to a record or not. If there is no other note pointing to the actual record then
 * serialize the record_data to the storage, otherwise keep it in the memory.
 *
 * @note unless immediate mode is used, modification operations will not be synced to the storage immediately.
 */
struct note{
    using record_type   = record;
    using key_type      = udho::session::id;
    using record_ptr    = std::unique_ptr<record_type>;
    using relesef_type  = std::function<void (const key_type&)>;

    /**
     * @brief mutable accessor to the value associated with key
     */
    struct proxy{
        proxy(note& self, const std::string& key): _self(self), _key(key) {}

        /**
         * @brief assignment operator overload
         * @param value
         * @return
         */
        template <typename T>
        proxy& operator=(T&& value){
            _self.template set<T>(_key, std::forward<T>(value));
            return *this;
        }

        /**
         * @brief get the value and lexically convert it to the desired type (the value is serialized as string in the storage)
         * @return
         */
        template <typename T>
        T as() const { return _self.template get<T>(_key); }

        /**
         * @brief operator T
         */
        template <typename T>
        operator T() const { return as<T>(); }

        /**
         * @brief unset
         * @return
         */
        bool unset(){ return _self.unset(_key); }

        private:
            note& _self;
            std::string _key;
    };

    /**
     * @brief immutable accessor to the value associated with key
     */
    struct const_proxy{
        const_proxy(const note& self, const std::string& key): _self(self), _key(key) {}

        /**
         * @brief get the value and lexically convert it to the desired type (the value is serialized as string in the storage)
         * @return
         */
        template <typename T>
        T as() const { return _self.template get<T>(_key); }

        /**
         * @brief get the value and lexically convert it to the desired type (the value is serialized as string in the storage)
         * @return
         */
        template <typename T>
        operator T() const { return as<T>(); }

        private:
            const note& _self;
            std::string _key;
    };

    note() = delete;
    note(const note&) = delete;
    note(note&& other) noexcept : _record(other._record), _releasef(std::move(other._releasef)) {}

    /**
     * @brief existence check
     * @param key
     * @return
     */
    bool exists(const std::string& key) const { return _record->exists(key); }

    /**
     * @brief get the value and lexically convert it to the desired type (the value is serialized as string in the storage)
     * @param key
     * @tparam T target type
     * @return
     */
    template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    T get(const std::string& key) const { return _record->template get<T>(key); }

    /**
     * @brief set the value by lexically converting it to the string
     * @param key
     * @tparam T type
     * @return
     */
    template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    void set(const std::string& key, T&& value) { _record->template set<T>(key, std::forward<T>(value)); }

    /**
     * @brief unset a key
     * @param key
     * @return
     */
    bool unset(const std::string& key) { return _record->remove(key); }

    /**
     * @brief revision of the session id
     * @return
     */
    std::uint64_t revision() const { return _record->revision(); }

    /**
     * @brief whether the session contains modifications that have not been synced with the storage yet
     * @return
     */
    bool dirty() const { return _record->dirty(); }

    /**
     * @brief returns an immutable accessor to the value associated with key
     * @pre expects exists(key)
     * @param key
     * @return
     */
    const_proxy operator[](const std::string& key) const { return const_proxy{*this, key}; }

    /**
     * @brief returns an mutable accessor to the value associated with key
     * @pre expects exists(key)
     * @param key
     * @return
     */
    proxy operator[](const std::string& key) { return proxy{*this, key}; }

    /**
     * @brief destructor triggers synchronization if there is no other note referencing the same session record
     */
    inline ~note(){
        if(_releasef)
            _releasef(_record->sessid());
    }

    template <typename StorageT, udho::session::modes>
    friend struct catalogue;

    private:
        inline explicit note(record_ptr& record, relesef_type realesef) noexcept: _record(record), _releasef(realesef) {}
    private:
        record_ptr&  _record;
        relesef_type _releasef;
};

}
}

#endif // UDHO_SESSION_NOTE_H
