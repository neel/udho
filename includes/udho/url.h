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

#ifndef UDHO_URL_H
#define UDHO_URL_H

#include <string>
#include <udho/configuration.h>
#include <udho/form.h>
#include <functional>
#include <memory>
#include <boost/algorithm/string/join.hpp>

namespace udho{
namespace detail{
    
template <typename T = void>
struct url_data_{
    const static struct protocol_t{
        typedef url_data_<T> component;
    } protocol;
    const static struct host_t{
        typedef url_data_<T> component;
    } host;
    const static struct port_t{
        typedef url_data_<T> component;
    } port;
    const static struct target_t{
        typedef url_data_<T> component;
    } target;
    const static struct path_t{
        typedef url_data_<T> component;
    } path;
    const static struct query_t{
        typedef url_data_<T> component;
    } query;
    
    std::string _protocol;
    std::string _host;
    std::size_t _port;
    std::string _path;
    std::string _target;
    std::string _query;
    
    url_data_(): _port(0){}
    
    void set(protocol_t, const std::string& v){_protocol = v;}
    std::string get(protocol_t) const{return _protocol;}
    
    void set(host_t, const std::string& v){_host = v;}
    std::string get(host_t) const{return _host;}
    
    void set(port_t, std::size_t v){_port = v;}
    std::size_t get(port_t) const{return _port;}
    
    void set(target_t, const std::string& v){_target = v;}
    std::string get(target_t) const{return _target;}
    
    void set(path_t, const std::string& v){_path = v;}
    std::string get(path_t) const{return _path;}
    
    void set(query_t, const std::string& v){_query = v;}
    const std::string& get(query_t) const{return _query;}
};

template <typename T> const typename url_data_<T>::protocol_t   url_data_<T>::protocol;
template <typename T> const typename url_data_<T>::host_t       url_data_<T>::host;
template <typename T> const typename url_data_<T>::port_t       url_data_<T>::port;
template <typename T> const typename url_data_<T>::target_t     url_data_<T>::target;
template <typename T> const typename url_data_<T>::path_t       url_data_<T>::path;
template <typename T> const typename url_data_<T>::query_t      url_data_<T>::query;

typedef url_data_<> url_data;
      
}

struct url: udho::configuration<detail::url_data>, udho::urlencoded_form<std::string::const_iterator>{
    class param{
      std::string _key;
      std::string _value;
      public:
        inline param(const std::string& k, const std::string& v): _key(k), _value(v){}
        inline std::string key() const{return _key;}
        inline std::string value() const{return _value;}
        inline std::string to_string() const{return _key+"="+_value;}
    };
    static url build(const std::string& base, const std::initializer_list<param>& params){
        std::vector<std::string> stringified_params;
        std::transform(params.begin(), params.end(), std::back_inserter(stringified_params), std::bind(&param::to_string, std::placeholders::_1));
        std::string joined = boost::algorithm::join(stringified_params, "&");
        std::string::const_iterator it = std::find(base.begin(), base.end(), '?');
        if(it != base.end()){
            return url(base+"&"+joined);
        }else{
            return url(base+"?"+joined);
        }
    }
    static inline url parse(const std::string& str){
        return url(str);
    }
    
    url() = delete;
    url(const url& other): udho::configuration<detail::url_data>(other), udho::urlencoded_form<std::string::const_iterator>(other){}
    
    std::string stringify() const{
        return to_string();
    }
    private:
        explicit url(const std::string& url_str){
            from_string(url_str);
        }
        inline void from_string(const std::string& url){
            std::string proto, hst, prt, pth, tgt, qry;
            std::string protocol_terminal = "://";
            std::string::const_iterator protocol_end = std::search(url.begin(), url.end(), protocol_terminal.begin(), protocol_terminal.end());
            if(protocol_end != url.end()){
                std::copy(url.begin(), protocol_end, std::back_inserter(proto));
                std::advance(protocol_end, 3);
            }else{
                protocol_end = url.begin();
            }
            std::string terminals = ":/";
            std::string::const_iterator it = std::find_first_of(protocol_end, url.end(), terminals.begin(), terminals.end());
            std::copy(protocol_end, it, std::back_inserter(hst));
            if(it != url.end()){
                if(*it == ':'){
                    std::string::const_iterator slash_it = std::find(++it, url.end(), '/');
                    std::copy(it, slash_it, std::back_inserter(prt));
                    it = slash_it;
                }
                std::copy(it, url.end(), std::back_inserter(tgt));
                std::string::const_iterator query_it = std::find(it, url.end(), '?');
                std::copy(it, query_it, std::back_inserter(pth));
                std::copy(++query_it, url.end(), std::back_inserter(qry));
            }
            
            (*this)[protocol] = proto;
            (*this)[host]     = hst;
            (*this)[port]     = !prt.empty() ? std::stoul(prt) : (proto == "https" ? 443 : 80);
            (*this)[path]     = !pth.empty() ? pth : std::string("/");
            (*this)[target]   = !tgt.empty() ? tgt : std::string("/");
            (*this)[query]    = qry;
            
            const std::string& qstr_ref = (*this)[query];
            
            udho::urlencoded_form<std::string::const_iterator>::parse(qstr_ref.begin(), qstr_ref.end());
        }
        inline std::string to_string() const{
            return (boost::format("%1%://%2%:%3%%4%") % (*this)[protocol] % (*this)[host] % (*this)[port] % (*this)[target]).str();
        }
};

}

#endif // UDHO_URL_H
