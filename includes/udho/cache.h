/*
 * Copyright (c) 2020, Neel Basu <neel.basu.z@gmail.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY Neel Basu <neel.basu.z@gmail.com> ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Neel Basu <neel.basu.z@gmail.com> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UDHO_CACHE_H
#define UDHO_CACHE_H

#include <set>
#include <map>
#include <chrono>
#include <cstdint>
#include <boost/asio/ip/address.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/path.hpp>
#include <ctti/type_id.hpp>
#include <ctti/name.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/algorithm/string/predicate.hpp>

#define UDHO_SESSION_FILE_EXTENSION "udho.cache.sess"

namespace udho{
namespace cache{
  
template <typename T = void>
struct content{
    typedef T value_type;
    typedef content<T> self_type;
    
    boost::posix_time::ptime _created;
    boost::posix_time::ptime _updated;
    value_type _value;
        
    explicit content(const value_type& value): _created(boost::posix_time::second_clock::local_time()), _updated(boost::posix_time::second_clock::local_time()), _value(value){}
    boost::posix_time::ptime created() const { return _created; }
    boost::posix_time::ptime updated() const { return _updated; }
    boost::posix_time::time_duration age() const { return boost::posix_time::second_clock::local_time() - created(); }
    boost::posix_time::time_duration idle() const { return boost::posix_time::second_clock::local_time() - updated(); }
    const value_type& value() const { return _value; }
    void update(const self_type& other){
        update(other.value());
    }
    void update(const value_type& value){
        _value = value;
        _updated = boost::posix_time::second_clock::local_time();
    }
    void update(){
        _updated = boost::posix_time::second_clock::local_time();
    }
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & _created;
        ar & _updated;
        ar & _value;
    }
    private:
        friend class boost::serialization::access;
};

template <>
struct content<void>{
    typedef void value_type;
    typedef content<> self_type;
    
    boost::posix_time::ptime _created;
    boost::posix_time::ptime _updated;
    
    explicit content(): _created(boost::posix_time::second_clock::local_time()), _updated(boost::posix_time::second_clock::local_time()){}
    template <typename U>
    explicit content(U): _created(boost::posix_time::second_clock::local_time()), _updated(boost::posix_time::second_clock::local_time()){}
    boost::posix_time::ptime created() const { return _created; }
    boost::posix_time::ptime updated() const { return _updated; }
    boost::posix_time::time_duration age() const { return boost::posix_time::second_clock::local_time() - created(); }
    boost::posix_time::time_duration idle() const { return boost::posix_time::second_clock::local_time() - updated(); }
    void update(){
        _updated = boost::posix_time::second_clock::local_time();
    }
    void update(const self_type& /*other*/){
        update();
    }
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & _created;
        ar & _updated;
    }
    private:
        friend class boost::serialization::access;
};

namespace storage{
   
template <typename KeyT, typename ValueT = void>
struct memory{
    typedef KeyT key_type;
    typedef ValueT value_type;
    typedef content<ValueT> content_type;
    typedef std::map<key_type, content_type> map_type;
    typedef storage::memory<KeyT, ValueT> self_type;
    
    memory() = default;
    memory(const self_type&) = delete;
    memory(self_type&&) = default;
    
    std::size_t size() const{
        boost::mutex::scoped_lock lock(_mutex);
        return _storage.size();
    }
    bool exists(const key_type& key) const{
        boost::mutex::scoped_lock lock(_mutex);
        return _storage.count(key);
    }
    void create(const key_type& key, const content_type& content){
        boost::mutex::scoped_lock lock(_mutex);
        _storage.insert(std::make_pair(key, content));
    }
    const content_type& retrieve(const key_type& key) const{
        boost::mutex::scoped_lock lock(_mutex);
        return _storage.at(key);
    }
    bool update(const key_type& key, const content_type& content){
        boost::mutex::scoped_lock lock(_mutex);
        typename map_type::iterator it = _storage.find(key);
        if(it != _storage.cend()){
            it->second.update(content);
            return true;
        }
        return false;
    }
    bool remove(const key_type& key){
        boost::mutex::scoped_lock lock(_mutex);
        typename map_type::iterator it = _storage.find(key);
        if(it != _storage.end()){
            _storage.erase(it);
            return true;
        }
        return false;
    }    
    protected:
        map_type _storage;
        mutable boost::mutex _mutex;
};

template <typename KeyT, typename ValueT = void>
struct disk{
    typedef KeyT key_type;
    typedef ValueT value_type;
    typedef content<ValueT> content_type;
    typedef std::map<key_type, content_type> map_type;
    typedef storage::disk<KeyT, ValueT> self_type;
    
    disk(const boost::filesystem::path& path): _storage(path){
        ctti::name_t name = ctti::nameof<value_type>();
        _name = name.name().str();
    }
    disk(const self_type&) = delete;
    disk(self_type&&) = default;
    
    std::size_t size() const{
        boost::mutex::scoped_lock lock(_mutex);
        std::count_if(boost::filesystem::directory_iterator(_storage), boost::filesystem::directory_iterator(), [](const boost::filesystem::path& p){
            return boost::algorithm::ends_with(p.filename(), UDHO_SESSION_FILE_EXTENSION);
        });
        return 0;
    }
    bool exists(const key_type& key) const{
        boost::mutex::scoped_lock lock(_mutex);
        return boost::filesystem::exists(path(key));
    }
    void create(const key_type& key, const content_type& content){
        boost::mutex::scoped_lock lock(_mutex);
        std::ofstream file(path(key).c_str());
        boost::archive::text_oarchive archive(file);
        archive << content;
    }
    content_type retrieve(const key_type& key) const{
        boost::mutex::scoped_lock lock(_mutex);
        content_type content;
        std::ifstream file(path(key).c_str());
        boost::archive::text_iarchive archive(file);
        archive >> content;
        return content;
    }
    bool update(const key_type& key, const content_type& content){
        boost::mutex::scoped_lock lock(_mutex);
        if(exists(key)){
            std::ofstream file(path(key).c_str());
            boost::archive::text_oarchive archive(file);
            archive << content;
            return true;
        }
        return false;
    }
    bool remove(const key_type& key){
        boost::mutex::scoped_lock lock(_mutex);
        return boost::filesystem::remove(path(key));
    }    
    
    protected:
        std::string _name;
        boost::filesystem::path _storage;
        mutable boost::mutex _mutex;
        
        std::string filename(const key_type& key) const{
            std::string key_str = boost::lexical_cast<std::string>(key);
            return (boost::format("%1%-%2%.%3%") % key_str % _name % UDHO_SESSION_FILE_EXTENSION).str();
        }
        boost::filesystem::path path(const std::string& key) const{
            return _storage / filename(key);
        }
};

struct redis{};
   
}

template <typename StorageT>
struct engine: StorageT{
    typedef StorageT storage_type;
    typedef typename storage_type::key_type key_type;
    typedef typename storage_type::value_type value_type;
    typedef typename storage_type::content_type content_type;
    typedef engine<StorageT> self_type;
    
    engine() = default;
    engine(const self_type&) = delete;
    engine(self_type&&) = default;
    
    boost::posix_time::ptime created(const key_type& key) const{
        return storage_type::retrieve(key).created();
    }
    boost::posix_time::ptime updated(const key_type& key) const{
        return storage_type::retrieve(key).updated();
    }
    boost::posix_time::time_duration age(const key_type& key) const{
        return storage_type::retrieve(key).age();
    }
    boost::posix_time::time_duration idle(const key_type& key) const{
        return storage_type::retrieve(key).idle();
    }
    template<typename U=value_type>
    typename std::enable_if<!std::is_same<U,void>::value>::type insert(const key_type& key, const U& value){
        storage_type::create(key, content_type(value));
    }
    template<typename U=value_type>
    typename std::enable_if<std::is_same<U,void>::value>::type insert(const key_type& key){
        storage_type::create(key, content_type());
    }
    template<typename U=value_type>
    const typename std::enable_if<!std::is_same<U,void>::value, value_type>::type& at(const key_type& key) const{
           return storage_type::retrieve(key).value();
    }
    template<typename U=value_type>
    typename std::enable_if<!std::is_same<U,void>::value, bool>::type update(const key_type& key, const U& value){
        return storage_type::update(key, content_type(value));
    }
    bool update(const key_type& key){
        return storage_type::update(key, content_type());
    }
    bool remove(const key_type& key){
        return storage_type::remove(key);
    }
};
    
template <typename KeyT, template<typename, typename> class StorageT = storage::memory>
struct master: public engine<StorageT<KeyT, void>>{
    typedef KeyT key_type;
    typedef engine<StorageT<KeyT, void>> storage_type;
    
    bool issued(const key_type& key) const{
        return storage_type::exists(key);
    }
    void issue(const key_type& key){
        storage_type::insert(key);
    }
};

template <typename KeyT, typename T, template<typename, typename> class StorageT = storage::memory>
struct registry: public engine<StorageT<KeyT, T>>{
    typedef registry<KeyT, T> self_type;
       
    registry() = default;
    registry(const self_type&) = delete;
    registry(self_type&&) = default;
    
    private:
        std::string _name;
    
};

template <typename KeyT, template<typename, typename> class StorageT = storage::memory, typename... T>
struct shadow;

/**
 * A type safe non-copiable storage 
 * \code
 * user u("Neel");
 * appearence a("red");
 * udho::cache::store<std::string, user, appearence> store;
 * store.insert("x", u);
 * store.insert("x", a);
 * store.exists<user>();
 * \endcode
 */
template <typename KeyT, template<typename, typename> class StorageT = storage::memory, typename... T>
struct store: master<KeyT, StorageT>, registry<KeyT, T, StorageT>...{ 
    typedef KeyT key_type;
    typedef master<KeyT, StorageT> master_type;
    typedef store<KeyT, StorageT, T...> self_type;
    typedef shadow<KeyT, StorageT, T...> shadow_type;
      
    store() = default;
    store(const self_type&) = delete;
    store(self_type&&) = default;
    
    template <typename V>
    bool exists(const key_type& key) const{
        return master_type::issued(key) && registry<KeyT, V, StorageT>::exists(key);
    }
    template <typename V>
    const V& get(const key_type& key, const V& def=V()) const{
        if(master_type::issued(key)){
            return registry<KeyT, V, StorageT>::at(key);
        }else{
            return def;
        }
    }
    template <typename V>
    V& at(const key_type& key){
        return registry<KeyT, V, StorageT>::at(key);
    }
    template <typename V>
    void insert(const key_type& key, const V& value){
        registry<KeyT, V, StorageT>::insert(key, value);
        if(!master_type::issued(key)){
            master_type::issue(key);
        }
        master_type::update();
    }
    std::size_t size() const{
        return master_type::size();
    }
    template <typename V>
    std::size_t size(const key_type& key) const{
        return registry<KeyT, V, StorageT>::size(key);
    }
    bool remove(){
        return master_type::remove();
    }
    template <typename V>
    bool remove(const key_type& key){
        if(registry<KeyT, V, StorageT>::remove(key)){
            master_type::update();
            return true;
        }
        return false;
    }
    
    boost::posix_time::ptime created(const key_type& key) const{
        return master_type::created(key);
    }
    boost::posix_time::ptime updated(const key_type& key) const{
        return master_type::updated(key);
    }
    boost::posix_time::time_duration age(const key_type& key) const{
        return master_type::age(key);
    }
    boost::posix_time::time_duration idle(const key_type& key) const{
        return master_type::idle(key);
    }
};

/**
 * copiable flake containes a reference to the actual registry object
 */
template <typename KeyT, typename T>
struct flake{
    typedef KeyT key_type;
    typedef T value_type;
    typedef registry<key_type, T> registry_type;
    typedef flake<KeyT, T> flake_type;
    typedef flake_type self_type;
   
    template <typename... X>
    flake(store<KeyT, X...>& store): _registry(store){}
    
    bool exists(const key_type& key) const{
        return _registry.exists(key);
    }
    const value_type& at(const key_type& key) const{
        return _registry.at(key);
    }
    void insert(const key_type& key, const value_type& value){
        _registry.insert(key, value);
    }
    std::size_t size(const key_type& key) const{
        return _registry.size(key);
    }
    bool remove(const key_type& key){
        return _registry.remove(key);
    }
    boost::posix_time::ptime created(const key_type& key) const{
        return _registry.created(key);
    }
    boost::posix_time::ptime updated(const key_type& key) const{
        return _registry.updated(key);
    }
    boost::posix_time::time_duration age(const key_type& key) const{
        return _registry.age(key);
    }
    boost::posix_time::time_duration idle(const key_type& key) const{
        return _registry.idle(key);
    }
private:
    registry_type& _registry;
};

/**
 * copiable shadow of a store reference.
 * \code
 * user u("Neel");
 * appearence a("red");
 * udho::cache::store<std::string, user, appearence> store;
 * udho::cache::shadow<std::string, user, appearence> shadow_ua(store);
 * shadow_ua.insert("x", u);
 * shadow_ua.insert("x", a);
 * std::cout << std::boolalpha << shadow_ua.exists<user>("x") << std::endl; // true
 * std::cout << std::boolalpha << shadow_ua.exists<appearence>("x") << std::endl; // true
 * std::cout << shadow_ua.get<user>("x").name << std::endl; // Neel
 * std::cout << shadow_ua.get<appearence>("x").color << std::endl; // red
 * udho::cache::shadow<std::string, user> shadow_u(store); 
 * std::cout << std::boolalpha << shadow_u.exists<user>("x") << std::endl; // true
 * std::cout << std::boolalpha << shadow_u.get<user>("x").name << std::endl; // true
 * std::cout << shadow_u.exists<appearence>("x") << std::endl; // won't compile
 * udho::cache::shadow<std::string, user> shadow_u2(shadow_u); // copiable
 * udho::cache::shadow<std::string, user> shadow_u3(shadow_ua); // narrowable
 * udho::cache::shadow<std::string, appearence> shadow_a(store); // unordered
 * std::cout << shadow_a.exists<appearence>("x") << std::endl; // true
 * std::cout << shadow_a.get<appearence>("x").color << std::endl; // red
 * udho::cache::shadow<std::string> shadow_none(shadow_ua);
 * \endcode
 */
template <typename KeyT, typename... T>
struct shadow: flake<KeyT, T>...{
    typedef KeyT key_type;
    typedef store<key_type, T...> store_type;
    typedef shadow<key_type, T...> self_type;
    typedef self_type shadow_type;
    typedef master<key_type> master_type;
    
    master_type& _master;
    
    template <typename... X>
    shadow(store<key_type, X...>& store): flake<key_type, T>(store)..., _master(store){}
    
    template <typename... X>
    shadow(shadow<key_type, X...>& other): flake<key_type, T>(other)..., _master(other._master){}
    
    bool issued(const key_type& key) const{
        return _master.issued(key);
    }
    void issue(const key_type& key){
        _master.issue(key);
    }
    
    template <typename V>
    bool exists(const key_type& key) const{
        return issued(key) && flake<key_type, V>::exists(key);
    }
    template <typename V>
    const V& at(const key_type& key) const{
        return flake<key_type, V>::at(key);
    }
    template <typename V>
    const V& get(const key_type& key, const V& def=V()) const{
        if(issued(key)){
            return at<V>(key);
        }
        return def;
    }
    template <typename V>
    void insert(const key_type& key, const V& value){
        flake<key_type, V>::insert(key, value);
        _master.update(key);
        if(!issued(key)){
            issue(key);
        }
    }
    
    std::size_t size() const{
        return _master.size();
    }
    template <typename V>
    std::size_t size(const key_type& key) const{
        return flake<KeyT, V>::size(key);
    }
    
    bool remove(const key_type& key) {
        return _master.remove(key);
    }
    template <typename V>
    bool remove(const key_type& key) {
        if(flake<KeyT, V>::remove(key)){
            return _master.update(key);
        }
        return false;
    }
    
    boost::posix_time::ptime created(const key_type& key) const{
        return _master.created(key);
    }
    boost::posix_time::ptime updated(const key_type& key) const{
        return _master.updated(key);
    }
    boost::posix_time::time_duration age(const key_type& key) const{
        return _master.age(key);
    }
    boost::posix_time::time_duration idle(const key_type& key) const{
        return _master.idle(key);
    }
    template <typename V>
    boost::posix_time::ptime created(const key_type& key) const{
        return flake<KeyT, V>::created(key);
    }
    template <typename V>
    boost::posix_time::ptime updated(const key_type& key) const{
        return flake<KeyT, V>::updated(key);
    }
    template <typename V>
    boost::posix_time::time_duration age(const key_type& key) const{
        return flake<KeyT, V>::age(key);
    }
    template <typename V>
    boost::posix_time::time_duration idle(const key_type& key) const{
        return flake<KeyT, V>::idle(key);
    }
};

template <typename KeyT>
struct generator;

template <>
struct generator<boost::uuids::uuid>{
    boost::uuids::uuid parse(const std::string& key){
        return boost::lexical_cast<boost::uuids::uuid>(key);
    }
    boost::uuids::uuid generate(){
        return boost::uuids::random_generator()();
    }
};

   
}
}

#endif // UDHO_CACHE_H
