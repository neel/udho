#ifndef UDHO_SESSION_RECORD_DATA_H
#define UDHO_SESSION_RECORD_DATA_H

#include <map>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <udho/session/fwd.h>
#include <udho/utils/traits.h>
#include <udho/session/defs.h>

namespace udho{
namespace session{

struct record_data{
    using container_type = std::map<std::string, std::string>;
    using const_iterator = typename container_type::const_iterator;
    using size_type      = typename container_type::size_type;

    template <typename StorageT, udho::session::modes>
    friend struct udho::session::catalogue;

    inline record_data(): _dirty(false), _created(std::chrono::system_clock::now()), _revision(0) {}
    inline explicit record_data(const udho::session::id& sessid): _dirty(false), _sessid(std::move(sessid)), _created(std::chrono::system_clock::now()), _revision(0) {}

    inline const udho::session::id& sessid() const { return _sessid; }

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
        if(!initial){
            _dirty = true;
            updated(std::chrono::system_clock::now());
        }
    }

    bool remove(const std::string& key) {
        auto it = _container.find(key);
        if(it != _container.end()){
            _container.erase(it);
            _dirty = true;
            return true;
        }
        return false;
    }

    inline const_iterator begin() const { return _container.begin(); }
    inline const_iterator end() const { return _container.end(); }
    inline size_type size() const { return _container.size(); }

    const time_point& created() const { return _created; }
    record_data& created(const time_point& time) {
        _created = time;
        return *this;
    }

    const time_point& updated() const { return _updated; }
    record_data& updated(const time_point& time) {
        _updated = time;
        return *this;
    }

    std::uint64_t revision() const { return _revision; }
    record_data& revision(const std::uint64_t& rev) {
        _revision = rev;
        return *this;
    }

private:
    bool           _dirty;
    udho::session::id    _sessid;
    container_type _container;
    time_point     _created;
    time_point     _updated;
    std::uint64_t  _revision;
};

}
}

#endif // UDHO_SESSION_RECORD_DATA_H
