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

#ifndef UDHO_WATCHER_H
#define UDHO_WATCHER_H

#include <list>
#include <map>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/mutex.hpp>

namespace udho{
    
template <typename WatchT>
struct watcher;
    
template <typename DerivedT, typename KeyT>
struct watch{
    typedef KeyT                             key_type;
    typedef watch<DerivedT, KeyT>            self_type;
    typedef boost::posix_time::ptime         time_point_type;
    typedef boost::posix_time::time_duration time_duration_type;
       
    key_type        _key;
    time_point_type _expiry;
    bool            _released;
    
    watch(const key_type& key): _key(key), _expiry(boost::posix_time::not_a_date_time), _released(false){}
    void release(const boost::system::error_code& e = boost::asio::error::operation_aborted){
        DerivedT& self = static_cast<DerivedT&>(*this);
        self(e);
        _released = true;
    }
    time_point_type expiry() const{
        return _expiry;
    }
    bool released() const{
        return _released;
    }
    key_type key() const{
        return _key;
    }
    bool valid() const{
        return !_expiry.is_not_a_date_time();
    }
    void expire_at(boost::posix_time::ptime time){
        _expiry = time;
    }
};
   
template <typename WatchT>
struct watcher{
    typedef WatchT                                  watch_type;
    typedef typename watch_type::key_type           key_type;
    typedef watcher<WatchT>                         self_type;
    typedef std::list<watch_type>                   container_type;
    typedef typename container_type::iterator       iterator_type;
    typedef std::multimap<key_type, iterator_type>  index_type;
    typedef boost::posix_time::ptime                time_point_type;
    typedef boost::posix_time::time_duration        time_duration_type;
    
    boost::asio::io_service&    _io;
    time_duration_type          _duration;
    boost::asio::deadline_timer _timer;
    container_type              _watchers;
    index_type                  _index;
    mutable boost::mutex        _mutex;
    
    watcher(boost::asio::io_service& io, const time_duration_type& duration): _io(io), _timer(io), _duration(duration){}    
    bool insert(watch_type watch){
        if(watch.released() || watch.valid()){
            // watch is valid iff an expiry time is set by the watcher
            // which implies that the watch has already been added to the queue
            // so reject such watches
            return false;
        }
        key_type key = watch.key();
        watch.expire_at(boost::posix_time::second_clock::local_time() + _duration);
        boost::mutex::scoped_lock lock(_mutex);
        if(_watchers.empty()){
            boost::posix_time::time_duration duration = watch.expiry() - boost::posix_time::second_clock::local_time();
            _timer.expires_from_now(duration);
            _timer.async_wait(boost::bind(&self_type::expired, this, boost::asio::placeholders::error));
        }
        _watchers.push_back(watch);
        iterator_type it = _watchers.end();
        _index.insert(std::make_pair(key, --it));
        return true;
    }
    void expired(const boost::system::error_code& err){
        if(err != boost::asio::error::operation_aborted){
            _mutex.lock();
            if(!_watchers.empty()){
                _mutex.unlock();
                _mutex.lock();
                watch_type current = _watchers.front();
                auto now = boost::posix_time::second_clock::local_time();
                if(current.expiry() > now){
                    boost::posix_time::time_duration duration = current.expiry() - now;
                    _timer.expires_from_now(duration);
                    _timer.async_wait(boost::bind(&self_type::expired, this, boost::asio::placeholders::error));
                    return;
                }
                auto iterator = _watchers.begin();
                key_type key = current.key();
                auto range = _index.equal_range(key);
                for(auto i = range.first; i != range.second;){
                    if(i->second == iterator){
                        i = _index.erase(i);
                    }else{
                        ++i;
                    }
                }
                _watchers.pop_front();
                _mutex.unlock();
                current.release(err); // May call another watcher function. So must unlock before calling
                _mutex.lock();
            }
            _mutex.unlock();
            _mutex.lock();
            if(!_watchers.empty()){ // The previous block may pop
                watch_type next = _watchers.front();
                boost::posix_time::time_duration duration = next.expiry() - boost::posix_time::second_clock::local_time();
                _timer.expires_from_now(duration);
                _timer.async_wait(boost::bind(&self_type::expired, this, boost::asio::placeholders::error));
            }
            _mutex.unlock();
        }
    }
    std::size_t notify(const key_type& key){
        _mutex.lock();
        auto range = _index.equal_range(key);
        auto count = std::distance(range.first, range.second);
        std::vector<watch_type> released;
        for(auto i = range.first; i != range.second;){
            iterator_type it = i->second;
            watch_type watch = *it;
            released.push_back(watch);
            _watchers.erase(it);
            i = _index.erase(i);
        }
        _mutex.unlock();
        for(watch_type& watch: released){
            watch.release(boost::asio::error::operation_aborted);
        }
        return count;
    }
    std::size_t notify_all(){
        _mutex.lock();
        std::vector<watch_type> released;
        for(auto i = _index.begin(); i != _index.end();){
            iterator_type it = i->second;
            watch_type watch = *it;
            released.push_back(watch);
            _watchers.erase(it);
            i = _index.erase(i);
        }
        std::size_t count = _index.size();
        _mutex.unlock();
        for(watch_type& watch: released){
            watch.release(boost::asio::error::operation_aborted);
        }
        return count;
    }
    void async_notify(const key_type& key){
        _io.post(boost::bind(&self_type::notify, this, key));
    }
    void async_notify_all(){
        _io.post(boost::bind(&self_type::notify_all, this));
    }
};
    
}

#endif // UDHO_WATCHER_H
