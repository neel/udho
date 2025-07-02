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

struct record_data{
    using container_type = std::map<std::string, std::string>;
    using const_iterator = typename container_type::const_iterator;
    using size_type      = typename container_type::size_type;

    template <typename StorageT, udho::session::modes>
    friend struct udho::session::catalogue;

    inline record_data(): _created(std::chrono::system_clock::now()), _revision(0) {}
    inline explicit record_data(const udho::session::id& sessid): _sessid(std::move(sessid)), _created(std::chrono::system_clock::now()), _revision(0) {}

    inline const udho::session::id& sessid() const { return _sessid; }

    inline bool dirty() const { return _updated_fields.size() > 0 || _removed_fields.size() > 0; }

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

        if(_removed_fields.count(key)){
            _removed_fields.erase(key);
        }

        if(!initial){
            _updated_fields.insert(key);
            updated(std::chrono::system_clock::now());
        }
    }

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

    bool is_updated(const std::string& k) const {
        return _updated_fields.count(k) > 0;
    }
    const std::set<std::string>& updated_fields() const {
        return _updated_fields;
    }
    const std::set<std::string>& removed_fields() const {
        return _removed_fields;
    }

    void sync(const std::uint64_t& rev, const time_point& time) {
        revision(rev);
        updated(time);
        _removed_fields.clear();
        _updated_fields.clear();
    }

private:
    udho::session::id    _sessid;
    container_type _container;
    time_point     _created;
    time_point     _updated;
    std::uint64_t  _revision;
    std::set<std::string> _updated_fields;
    std::set<std::string> _removed_fields;
};

}
}

#endif // UDHO_SESSION_RECORD_DATA_H
