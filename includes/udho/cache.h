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
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem/path.hpp>
#include <ctti/type_id.hpp>
#include <ctti/name.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <udho/configuration.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

namespace udho{
namespace cache{
  
template <typename T = void>
struct content{
    typedef T value_type;
    typedef content<T> self_type;
    
    boost::posix_time::ptime _created;
    boost::posix_time::ptime _updated;
    value_type _value;
        
    content(): _created(boost::posix_time::second_clock::local_time()), _updated(boost::posix_time::second_clock::local_time()) {}
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
        ar & BOOST_SERIALIZATION_NVP(_created);
        ar & BOOST_SERIALIZATION_NVP(_updated);
        ar & BOOST_SERIALIZATION_NVP(_value);
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
        ar & BOOST_SERIALIZATION_NVP(_created);
        ar & BOOST_SERIALIZATION_NVP(_updated);
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
    typedef udho::config<udho::configs::session> session_config_type;
    typedef session_config_type config_type;
    
    memory(const session_config_type& config): _config(config){};
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
        const session_config_type& _config;
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
    typedef udho::config<udho::configs::session> session_config_type;
    typedef session_config_type config_type;
    
    disk(const session_config_type& config): _config(config){
        ctti::name_t name = ctti::nameof<value_type>();
        _name = name.name().str();
    }
    disk(const self_type&) = delete;
    disk(self_type&&) = default;
    
    std::size_t size() const{
        boost::mutex::scoped_lock lock(_mutex);
        std::string ext = extension();
        std::count_if(boost::filesystem::directory_iterator(storage()), boost::filesystem::directory_iterator(), [ext](const boost::filesystem::path& p){
            return boost::algorithm::ends_with(p.filename().string(), ext);
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
        save(file, content);
    }
    content_type retrieve(const key_type& key) const{
        boost::mutex::scoped_lock lock(_mutex);
        content_type content;
        std::ifstream file(path(key).c_str());
        load(file, content);
        return content;
    }
    bool update(const key_type& key, const content_type& content){
        if(exists(key)){
            content_type current = retrieve(key);
            current.update(content);
            boost::mutex::scoped_lock lock(_mutex);
            std::ofstream file(path(key).c_str());
            save(file, current);
            return true;
        }
        return false;
    }
    bool remove(const key_type& key){
        boost::mutex::scoped_lock lock(_mutex);
        return boost::filesystem::remove(path(key));
    }    
    
    protected:
        const session_config_type& _config;
        std::string _name;
        mutable boost::mutex _mutex;
        
        boost::filesystem::path storage() const{
            return _config[udho::configs::session::path];
        }
        udho::configs::session::format format() const{
            return _config[udho::configs::session::serialization];
        }
        std::string extension() const{
            return _config[udho::configs::session::extension];
        }
        std::string filename(const key_type& key) const{
            std::string key_str = boost::lexical_cast<std::string>(key);
            return (boost::format("%1%-%2%.%3%") % key_str % _name % extension()).str();
        }
        boost::filesystem::path path(const key_type& key) const{
            return storage() / filename(key);
        }
        
        void save(udho::configs::session::format format, std::ofstream& file, const content_type& content){
            switch(format){
                case udho::configs::session::format::text:{
                    boost::archive::text_oarchive archive(file);
                    archive << content;
                }
                break;
                case udho::configs::session::format::binary:{
                    boost::archive::binary_oarchive archive(file);
                    archive << content;
                }
                break;
                case udho::configs::session::format::xml:{
                    boost::archive::xml_oarchive archive(file);
                    archive << BOOST_SERIALIZATION_NVP(content);
                }
                break;
            }
        }
        void load(udho::configs::session::format format, std::ifstream& file, content_type& content) const{
            switch(format){
                case udho::configs::session::format::text:{
                    boost::archive::text_iarchive archive(file);
                    archive >> content;
                }
                break;
                case udho::configs::session::format::binary:{
                    boost::archive::binary_iarchive archive(file);
                    archive >> content;
                }
                break;
                case udho::configs::session::format::xml:{
                    boost::archive::xml_iarchive archive(file);
                    archive >> BOOST_SERIALIZATION_NVP(content);
                }
                break;
            }
        }
        void save(std::ofstream& file, const content_type& content){
            udho::configs::session::format storage_format = format();
            save(storage_format, file, content);
        }
        void load(std::ifstream& file, content_type& content) const{
            udho::configs::session::format storage_format = format();
            load(storage_format, file, content);
        }
};

struct redis{};
   
}

template <typename KeyT, typename ValueT = void>
struct abstract_engine{
    typedef KeyT key_type;
    typedef ValueT value_type;
    typedef content<ValueT> content_type;
    typedef abstract_engine<KeyT, ValueT> self_type;
    
    virtual std::size_t size() const = 0;
    virtual bool exists(const key_type& key) const = 0;
    virtual void create(const key_type& key, const content_type& content) = 0;
    virtual content_type retrieve(const key_type& key) const = 0;
    virtual bool update(const key_type& key, const content_type& content) = 0;
    virtual bool remove(const key_type& key) = 0;
    
    boost::posix_time::ptime created(const key_type& key) const{
        return retrieve(key).created();
    }
    boost::posix_time::ptime updated(const key_type& key) const{
        return retrieve(key).updated();
    }
    boost::posix_time::time_duration age(const key_type& key) const{
        return retrieve(key).age();
    }
    boost::posix_time::time_duration idle(const key_type& key) const{
        return retrieve(key).idle();
    }
    
    template<typename U=value_type>
    typename std::enable_if<!std::is_same<U,void>::value>::type insert(const key_type& key, const U& value){
        create(key, content_type(value));
    }
    template<typename U=value_type>
    typename std::enable_if<std::is_same<U,void>::value>::type insert(const key_type& key){
        create(key, content_type());
    }
    
    template<typename U=value_type>
    typename std::enable_if<!std::is_same<U,void>::value, bool>::type update(const key_type& key, const U& value){
        return update(key, content_type(value));
    }
    bool update(const key_type& key){
        return update(key, content_type());
    }
};

template <typename StorageT>
struct engine: private StorageT, public abstract_engine<typename StorageT::key_type, typename StorageT::value_type>{
    typedef StorageT storage_type;
    typedef abstract_engine<typename StorageT::key_type, typename StorageT::value_type> abstract_engine_type;
    typedef typename StorageT::key_type key_type;
    typedef typename StorageT::value_type value_type;
    typedef typename StorageT::content_type content_type;
    typedef engine<StorageT> self_type;
    typedef typename StorageT::config_type config_type;
    
    explicit engine(const config_type& config): StorageT(config) {}
    
    std::size_t size() const final{
        return storage_type::size();
    };
    bool exists(const key_type& key) const final{
        return storage_type::exists(key);
    }
    void create(const key_type& key, const content_type& content) final{
        storage_type::create(key, content);
    }
    content_type retrieve(const key_type& key) const final{
        return storage_type::retrieve(key);
    }
    bool update(const key_type& key, const content_type& content) final{
        return storage_type::update(key, content);
    }
    bool remove(const key_type& key) final{
        return storage_type::remove(key);
    }
    
    using abstract_engine_type::update;
};

template <typename KeyT, typename ValueT = void>
struct driver{
    typedef KeyT key_type;
    typedef ValueT value_type;
    typedef content<ValueT> content_type;
    typedef abstract_engine<KeyT, ValueT> abstract_engine_type;
    
    abstract_engine_type& _engine;
    
    explicit driver(abstract_engine_type& e): _engine(e){}
    std::size_t size() const { return _engine.size(); }
    bool exists(const key_type& key) const{ return _engine.exists(key); }
    template<typename U=value_type>
    typename std::enable_if<!std::is_same<U,void>::value>::type insert(const key_type& key, const U& value){ return _engine.insert(key, value); }
    template<typename U=value_type>
    typename std::enable_if<std::is_same<U,void>::value>::type insert(const key_type& key){ return _engine.insert(key); }
    content_type retrieve(const key_type& key) const{ return _engine.retrieve(key); }
    bool update(const key_type& key, const content_type& content){ return _engine.update(key, content); }
    bool update(const key_type& key) { return _engine.update(key); }
    template<typename U=value_type>
    typename std::enable_if<std::is_same<U, ValueT>::value, bool>::type update(const key_type& key, const U& value){ return update(key, content_type(value)); }
    bool remove(const key_type& key) { return _engine.remove(key); }
    
    boost::posix_time::ptime created(const key_type& key) const{ return _engine.created(key); }
    boost::posix_time::ptime updated(const key_type& key) const{ return _engine.updated(key); }
    boost::posix_time::time_duration age(const key_type& key) const{ return _engine.age(key); }
    boost::posix_time::time_duration idle(const key_type& key) const{ return _engine.idle(key); }
};
    
template <typename KeyT>
struct master: public driver<KeyT>{
    typedef KeyT key_type;
    typedef driver<KeyT> storage_type;
    typedef abstract_engine<KeyT> abstract_engine_type;
    typedef master<KeyT> self_type;
    
    master(abstract_engine_type& e): driver<KeyT>(e){}
    
    master(const self_type&) = delete;
    master(self_type&&) = default;
    
    bool issued(const key_type& key) const{
        return storage_type::exists(key);
    }
    void issue(const key_type& key){
        storage_type::insert(key);
    }
};

template <typename KeyT, typename T>
struct registry: public driver<KeyT, T>{
    typedef registry<KeyT, T> self_type;
    typedef KeyT key_type;
    typedef T value_type;
    typedef abstract_engine<KeyT, T> abstract_engine_type;
       
    registry(abstract_engine_type& e): driver<KeyT, T>(e){}
    
    registry(const self_type&) = delete;
    registry(self_type&&) = default;
    
    const value_type& at(const key_type& key) const{
        return driver<KeyT, T>::retrieve(key).value();
    }
        
    private:
        std::string _name;
    
};

template <typename KeyT, typename... T>
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
template <template <typename, typename> class StorageT, typename KeyT, typename... T>
struct store: private engine<StorageT<KeyT, void>>, private engine<StorageT<KeyT, T>>..., protected master<KeyT>, public registry<KeyT, T>...{
    friend struct shadow<KeyT, T...>;
    
    typedef KeyT key_type;
    typedef master<KeyT> master_type;
    typedef shadow<KeyT, T...> shadow_type;
    typedef store<StorageT, KeyT, T...> self_type;
    typedef typename engine<StorageT<KeyT, void>>::config_type config_type;
      
    store(const config_type& config): engine<StorageT<KeyT, void>>(config), engine<StorageT<KeyT, T>>(config)..., master<KeyT>(static_cast<engine<StorageT<KeyT, void>>&>(*this)), registry<KeyT, T>(static_cast<engine<StorageT<KeyT, T>>&>(*this))... {};
    store(const self_type&) = delete;
    
    template <typename V>
    bool exists(const key_type& key) const{
        return master_type::issued(key) && registry<KeyT, V>::exists(key);
    }
    template <typename V>
    const V& get(const key_type& key, const V& def=V()) const{
        if(master_type::issued(key)){
            return registry<KeyT, V>::at(key);
        }else{
            return def;
        }
    }
    template <typename V>
    V& at(const key_type& key){
        return registry<KeyT, V>::at(key);
    }
    template <typename V>
    void insert(const key_type& key, const V& value){
        registry<KeyT, V>::insert(key, value);
        if(!master_type::issued(key)){
            master_type::issue(key);
        }
        master_type::update();
    }
//     std::size_t size() const{
//         return master_type::size();
//     }
    template <typename V>
    std::size_t size(const key_type& key) const{
        return registry<KeyT, V>::size(key);
    }
//     bool remove(){
//         return master_type::remove();
//     }
    template <typename V>
    bool remove(const key_type& key){
        if(registry<KeyT, V>::remove(key)){
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
   
    template <template <typename, typename> class StorageT, typename... X>
    flake(store<StorageT, KeyT, X...>& store): _registry(store){}
    
    bool exists(const key_type& key) const{
        return _registry.exists(key);
    }
    const value_type& at(const key_type& key) const{
        return _registry.at(key);
    }
    void insert(const key_type& key, const value_type& value){
        _registry.insert(key, value);
    }
    void update(const key_type& key, const value_type& value){
        _registry.update(key, value);
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
//     typedef store<key_type, T...> store_type;
    typedef shadow<key_type, T...> self_type;
    typedef self_type shadow_type;
    typedef master<key_type> master_type;
    
    master_type& _master;
    
    template <template <typename, typename> class StorageT, typename... X>
    shadow(store<StorageT, key_type, X...>& store): flake<key_type, T>(store)..., _master(store){}
    
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
    void set(const key_type& key, const V& value) {
        if(exists<V>(key)){
            flake<key_type, V>::update(key, value);
            _master.update(key);
        }else{
            insert<V>(key, value);
        }
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
