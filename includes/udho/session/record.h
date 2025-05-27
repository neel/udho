#ifndef UDHO_SESSION_RECORD_H
#define UDHO_SESSION_RECORD_H

#include <map>
#include <string>
#include <mutex>
#include <stdexcept>
#include <boost/intrusive_ptr.hpp>
#include <udho/utils/traits.h>
#include <boost/lexical_cast.hpp>
#include <udho/session/fwd.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace udho{
namespace session{

struct record_data{
    using sessid_type    = boost::uuids::uuid;
    using container_type = std::map<std::string, std::string>;
    using const_iterator = typename container_type::const_iterator;
    using size_type      = typename container_type::size_type;

    friend udho::session::storage::disk;
    template <typename StorageT>
    friend struct udho::session::catalogue;

    inline record_data(): _dirty(false) {}
    inline explicit record_data(const sessid_type& sessid): _dirty(false), _sessid(std::move(sessid)) {}

    inline const sessid_type& sessid() const { return _sessid; }

    inline bool dirty() const { return _dirty; }

    inline bool exists(const std::string& key) const {
        auto it = _container.find(key);
        return (it != _container.end());
    }

    template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    T get(const std::string& key) const {
        auto it = _container.find(key);
        if(it != _container.end()){
            const std::string& value_str = it->second;
            return boost::lexical_cast<T>(value_str);
        }
        throw std::out_of_range{"out of range key: " + key};
    }

    template <typename T, std::enable_if_t<udho::utils::traits::is_ostreamable_v<T>, bool> = true>
    void set(const std::string& key, T&& value, bool initial = false) {
        std::string value_str = boost::lexical_cast<std::string>(std::forward<T>(value));

        auto it = _container.find(key);
        if(it != _container.end()){
            it->second = std::move(value_str);
        } else {
            _container.insert(std::make_pair(key, std::move(value_str)));
        }
        if(!initial)
            _dirty = true;
    }

    inline const_iterator begin() const { return _container.begin(); }
    inline const_iterator end() const { return _container.end(); }
    inline size_type size() const { return _container.size(); }

    private:
        bool           _dirty;
        sessid_type    _sessid;
        container_type _container;
};

struct record: private record_data{
    using record_type    = record;
    using sessid_type    = typename record_data::sessid_type;
    using container_type = typename record_data::container_type;
    using const_iterator = typename container_type::const_iterator;
    using size_type      = typename container_type::size_type;

    friend udho::session::storage::disk;
    template <typename StorageT>
    friend struct udho::session::catalogue;
    friend struct udho::session::note;

    inline record(const boost::uuids::uuid& sessid): record_data(sessid) {}

    using record_data::sessid;

    protected:
        inline bool _dirty() const { return record_data::dirty(); }

        inline bool dirty() const {
            std::lock_guard<std::mutex> lock(_mutex_data);
            return record_data::dirty();
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
            return record_data::template set<T>(key, std::move(value));
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
};

}
}

#endif // UDHO_SESSION_RECORD_H
