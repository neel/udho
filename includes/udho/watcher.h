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

namespace udho{
    
template <typename DerivedT, typename KeyT>
struct watch{
    typedef KeyT                             key_type;
    typedef boost::posix_time::ptime         time_point_type;
    typedef boost::posix_time::time_duration time_duration_type;
    
    key_type        _key;
    time_point_type _expiry;
    bool            _released;
    
    watch(const key_type& key, const boost::posix_time::time_duration& duration = boost::posix_time::seconds(15)): _key(key), _expiry(boost::posix_time::second_clock::local_time() + duration), _released(false){}
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
};
   
template <typename WatchT>
struct watcher{
    typedef WatchT watch_type;
    typedef typename watch_type::key_type key_type;
    typedef watcher<WatchT> self_type;
    typedef std::list<watch_type> container_type;
    typedef typename container_type::iterator iterator_type;
    typedef std::multimap<key_type, iterator_type> index_type;
    
    boost::asio::io_service&    _io;
    boost::asio::deadline_timer _timer;
    container_type              _watchers;
    index_type                  _index;
    
    watcher(boost::asio::io_service& io): _io(io), _timer(io){}    
    bool insert(const key_type& key, const watch_type& watch){
        if(watch.released()){
            return false;
        }
        if(_watchers.empty()){
            boost::posix_time::time_duration duration = watch.expiry() - boost::posix_time::second_clock::local_time();
            _timer.expires_from_now(duration);
            std::cout << this << " async_wait for" << duration <<" from " << boost::posix_time::second_clock::local_time() << std::endl;
            _timer.async_wait(boost::bind(&self_type::expired, this, boost::asio::placeholders::error));
            std::cout << this << " watch expiring at " << watch.expiry() << std::endl;
        }
        if(watch.expiry() <= _watchers.back().expiry()){
            std::cout << this << " watch infeasible last " << _watchers.back().expiry() << " current " << watch.expiry() << std::endl;
        }
        _watchers.push_back(watch);
        iterator_type it = _watchers.end();
        _index.insert(std::make_pair(key, --it));
        return true;
    }
    void expired(const boost::system::error_code& err){
        std::cout << this << " watch expired " << err << std::endl;
        if(err != boost::asio::error::operation_aborted){
            if(!_watchers.empty()){
                watch_type current = _watchers.front();
                auto now = boost::posix_time::second_clock::local_time();
                if(current.expiry() > now){
                    boost::posix_time::time_duration duration = current.expiry() - now;
                    _timer.expires_from_now(duration);
                    std::cout << this << " async_wait for" << duration <<" from " << now << std::endl;
                    _timer.async_wait(boost::bind(&self_type::expired, this, boost::asio::placeholders::error));
                    std::cout << this << " watch expiring at " << current.expiry() << std::endl;
                    return;
                }
                auto iterator = _watchers.begin();
                key_type key = current.key();
                auto range = _index.equal_range(key);
                for(auto i = range.first; i != range.second;){
                    std::cout << "iterating" << std::endl;
                    if(i->second == iterator){
                        std::cout << "removing" << std::endl;
                        i = _index.erase(i);
                    }else{
                        ++i;
                    }
                }
                _watchers.pop_front();
                current.release(err);
            }
            if(!_watchers.empty()){
                watch_type next = _watchers.front();
                boost::posix_time::time_duration duration = next.expiry() - boost::posix_time::second_clock::local_time();
                _timer.expires_from_now(duration);
                std::cout << this << " async_wait for" << duration <<" from " << boost::posix_time::second_clock::local_time() << std::endl;
                _timer.async_wait(boost::bind(&self_type::expired, this, boost::asio::placeholders::error));
                std::cout << this << " watch expiring at " << next.expiry() << std::endl;
            }
        }
    }
    std::size_t notify(const key_type& key){
        std::cout << "notifying " << key << std::endl;
        auto range = _index.equal_range(key);
        auto count = std::distance(range.first, range.second);
        for(auto i = range.first; i != range.second;){
            iterator_type it = i->second;
            watch_type watch = *it;
            std::cout << this << " watch found " << key << std::endl;
            watch.release(boost::asio::error::operation_aborted);
            _watchers.erase(it);
            i = _index.erase(i);
        }
        return count;
    }
    std::size_t notify_all(){
        for(auto i = _index.begin(); i != _index.end();){
            iterator_type it = i->second;
            watch_type watch = *it;
            watch.release(boost::asio::error::operation_aborted);
            _watchers.erase(it);
            i = _index.erase(i);
        }
        return _index.size();
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
