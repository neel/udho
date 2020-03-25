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

#ifndef UDHO_FORM_H
#define UDHO_FORM_H

#include <map>
#include <string>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <boost/lexical_cast.hpp>
#include <boost/beast/http/message.hpp>

namespace udho{
    
enum class form_type{
    unparsed,
    urlencoded,
    multipart
};
    
template<class ForwardIt1, class ForwardIt2>
constexpr ForwardIt1 _search(ForwardIt1 first, ForwardIt1 last, ForwardIt2 s_first, ForwardIt2 s_last){
    for (; ; ++first) {
        ForwardIt1 it = first;
        for (ForwardIt2 s_it = s_first; ; ++it, ++s_it) {
            if (s_it == s_last) {
                return first;
            }
            if (it == last) {
                return last;
            }
            if (!(*it == *s_it)) {
                break;
            }
        }
    }
}
    
template <typename Iterator>
struct bounded_str{
    typedef bounded_str<Iterator> self_type;
    typedef std::pair<Iterator, Iterator> pair_type;
    
    pair_type _pair;
    
    bounded_str(Iterator begin, Iterator end): _pair(begin, end){}
    bounded_str(const self_type& other): _pair(other._pair){}
    Iterator begin() const{return _pair.first;}
    Iterator end() const{return _pair.second;}
    bool invalid() const{return begin() == end();}
    bool valid() const{return !invalid();}
    template <typename Itarator>
    void copy(Itarator it) const{
        std::copy(begin(), end(), it);
    }
    template <typename ContainerT>
    ContainerT copied() const{
        ContainerT out;
        copy(std::back_inserter(out));
        return out;
    }
    std::size_t size() const{
        return std::distance(begin(), end());
    }
};
  
template <typename Iterator>
struct urlencoded_form{
    typedef Iterator iterator_type;
    typedef typename std::iterator_traits<iterator_type>::value_type value_type;
    typedef std::basic_string<value_type> string_type;
    typedef bounded_str<iterator_type> bounded_string_type;
    typedef std::map<string_type, bounded_string_type> header_map_type;
    typedef bounded_string_type bounded_string;
    
    header_map_type _fields;
    
    void parse(iterator_type begin, iterator_type end){
        iterator_type last  = begin;
        while(true){
            iterator_type it     = std::find(last, end, '&'); 
            iterator_type assign = std::find(last, it, '=');
            bounded_string_type key(last, assign);
            bounded_string_type value(assign+1, it);
            
            std::string key_str = key.template copied<std::string>();
            
            // std::cout << "Key ^"   << key.  template copied<std::string>() << "$" << std::endl;
            // std::cout << "value ^" << value.template copied<std::string>() << "$" << std::endl;
            
            _fields.insert(std::make_pair(key_str, value));
            if(it == end){
                break;
            }else{
                last = it+1;
            }
        }
    }
    std::size_t count() const{
        return _fields.size();
    }
    bool has(const std::string name) const{
        return _fields.find(name) != _fields.end();
    }
    template <typename T>
    const T field(const std::string& name) const{
        return boost::lexical_cast<T>(_fields.at(name).template copied<std::string>());
    }
};

template <typename Iterator>
struct multipart_form{
    typedef Iterator iterator_type;
    typedef typename std::iterator_traits<iterator_type>::value_type value_type;
    typedef std::basic_string<value_type> string_type;
    typedef bounded_str<iterator_type> bounded_string_type;
    typedef std::map<string_type, bounded_string_type> header_map_type;
    typedef bounded_string_type bounded_string;
    
    struct form_part{
        header_map_type _headers;
        bounded_string_type _body;
        
        form_part(iterator_type begin, iterator_type end): _body(begin, end){}
        
        void set_header(const header_map_type& headers){
            _headers = headers;
        }
        const bounded_string_type& header(const std::string& key) const{
            return _headers.at(key);
        }
        bounded_string_type header(const std::string& key, const std::string& sub) const{
            bounded_string_type value = header(key);
            std::string subfield = sub+"=";
            iterator_type sub_begin = _search(value.begin(), value.end(), subfield.begin(), subfield.end());
            iterator_type sub_end = std::find(sub_begin, value.end(), ';');
            if(sub_begin != sub_end){
                auto it = std::find(sub_begin, sub_end, '"');
                if(it != sub_end){
                    sub_begin = it+1;
                    sub_end   = std::find(sub_begin, sub_end, '"');
                }
            }
            return bounded_string_type(sub_begin, sub_end);
        }
        bounded_string_type name() const{
            return header("Content-Disposition", "name");
        }
        bounded_string_type filename() const{
            return header("Content-Disposition", "filename");
        }
        const bounded_string_type& body() const{
            return _body;
        }
        template <typename StrT>
        StrT copied() const{
            return body().template copied<StrT>();
        }
        std::string str() const{
            return copied<std::string>();
        }
        template <typename T>
        T value() const{
            return boost::lexical_cast<T>(str());
        }
    };
    
    std::string _boundary;
    std::map<string_type, form_part> _parts;

    void parse(const std::string& boundary, iterator_type begin, iterator_type end){
        _boundary = boundary;
        iterator_type last  = begin;
        iterator_type index = _search(last, end,_boundary.begin(), _boundary.end());
        while(true){
            last = index;
            index = _search(last+_boundary.size(), end, _boundary.begin(), _boundary.end());
            if(index == end){
                break;
            }
            
            // std::cout << "NEXT PART ^" << bounded_string(last+_boundary.size()+2, index).template copied<string_type>() << "$" << std::endl;

            parse_part(last+_boundary.size()+2, index);
        }
    }
    void parse_part(iterator_type begin, iterator_type end){
        string_type crlf("\r\n");
        iterator_type last  = begin;
        iterator_type index = _search(last, end, crlf.begin(), crlf.end());
        
        header_map_type headers;
        
        while(true){
            // std::cout << "Header ^" << bounded_string(last, index).template copied<string_type>() << "$" << std::endl;
            iterator_type colon = std::find(last, index, ':');
            if(colon != index){
                std::string key = bounded_string(last, colon).template copied<std::string>();
                // std::cout << "Key ^" << key << "$" << std::endl;
                iterator_type vbegin = std::find_if_not(colon+1, index, [](char ch){return std::isspace(ch);});
                // std::cout << "Value ^" << bounded_string(vbegin, index).template copied<string_type>() << "$" << std::endl;
                headers.insert(std::make_pair(key, bounded_string(vbegin, index)));
            }
            last = index+2;
            index = _search(last, end, crlf.begin(), crlf.end());
            if(index == last){
                break;
            }
        }
        // std::cout << "BODY ^" << bounded_string(index+2, end-2).template copied<string_type>() << "$" << std::endl;
        
        form_part f(index+2, end-2);
        f.set_header(headers);
        _parts.insert(std::make_pair(f.name().template copied<std::string>(), f));
    }
    std::size_t count() const{
        return _parts.size();
    }
    bool has(const std::string name) const{
        return _parts.find(name) != _parts.end();
    }
    const form_part& part(const std::string& name) const{
        return _parts.at(name);
    }
    template <typename T>
    const T field(const std::string& name) const{
        return part(name).template value<T>();
    }
};
    
template <typename RequestT>
struct form_{
    typedef RequestT request_type;
    typedef typename request_type::body_type::value_type body_type;
    typedef std::map<std::string, std::string> fields_map_type;
    
    const request_type& _request;
    form_type _type;
    urlencoded_form<std::string::const_iterator> _urlencoded;
    multipart_form<std::string::const_iterator>  _multipart;
    
    form_(const request_type& request): _request(request), _type(form_type::unparsed){
        if(_request[boost::beast::http::field::content_type].find("application/x-www-form-urlencoded") != std::string::npos){
            parse_urlencoded();
        }else if(_request[boost::beast::http::field::content_type].find("multipart/form-data") != std::string::npos){
            parse_multipart();
        }
    }
    void parse_urlencoded(){
        _urlencoded.parse(_request.body().begin(), _request.body().end());
        _type = form_type::urlencoded;
    }
    void parse_multipart(){
        std::string boundary;
        {
            std::string content_type(_request[boost::beast::http::field::content_type]);
            std::string boundary_key("boundary=");
            std::string::size_type index = content_type.find(boundary_key);
            if(index != std::string::npos){
                boundary = "--"+content_type.substr(index+boundary_key.size());
                _type = form_type::multipart;
            }else{
                std::cout << "multipart boundary not found in Content-Type: " << content_type << std::endl;
                return;
            }
        }
        // std::cout << _request << std::endl << "#################### " << std::endl << "BOUNDARY =(" << boundary << ")" << std::endl;
        _multipart.parse(boundary, _request.body().begin(), _request.body().end());
    }
    bool is_urlencoded() const{
        return _type == form_type::urlencoded;
    }
    bool is_multipart() const{
        return _type == form_type::multipart;
    }
    const urlencoded_form<std::string::const_iterator>& urlencoded() const{
        return _urlencoded;
    }
    const multipart_form<std::string::const_iterator>& multipart() const{
        return _multipart;
    }
    bool parsed() const{
        return _type != form_type::unparsed;
    }
    bool has(const std::string& name) const{
        if(is_urlencoded()){
            return _urlencoded.has(name);
        }else if(is_multipart()){
            return _multipart.has(name);
        }
        return false;
    }
    template <typename V>
    V field(const std::string& name) const{
        if(is_urlencoded()){
            return _urlencoded.field<V>(name);
        }else if(is_multipart()){
            return _multipart.field<V>(name);
        }
        return V();
    }
    fields_map_type::size_type count() const{
        if(is_urlencoded()){
            return _urlencoded.count();
        }else if(is_multipart()){
            return _multipart.count();
        }
        return 0;
    }
};
}

#endif // UDHO_FORM_H
