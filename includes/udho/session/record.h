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

/**
 * @struct record
 * @brief Thread-safe session record
 * @details Wraps record_data with mutex protection
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

    inline record(const udho::session::id& sessid, notifier_type&& notifier): record_data(sessid), _notifier(std::move(notifier)) {}

    using record_data::sessid;

    protected:
        inline bool _dirty() const { return record_data::dirty(); }

        inline bool dirty() const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::dirty();
        }

        inline std::uint64_t revision() const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::revision();
        }

        inline size_type size() const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::size();
        }

        inline bool exists(const std::string& key) const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::exists(key);
        }

        template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
        T get(const std::string& key) const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::template get<T>(key);
        }

        template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
        void set(const std::string& key, T&& value) {
            std::lock_guard<std::mutex> lock(_mutex_data);
            record_data::template set<T>(key, std::move(value));
            if(record_data::dirty()) {
                _notifier(*this);
            }
        }

        bool remove(const std::string& key) {
            std::lock_guard<std::mutex> lock(_mutex_data);
            bool res = record_data::remove(key);
            if(record_data::dirty()) {
                _notifier(*this);
            }
            return res;
        }

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
};

}
}

#endif // UDHO_SESSION_RECORD_H
