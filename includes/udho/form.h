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
#include <udho/util.h>
#include <boost/lexical_cast.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/function.hpp>
#include <udho/access.h>

namespace udho{
    
enum class form_type{
    unparsed,
    urlencoded,
    multipart
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
        return boost::lexical_cast<T>(udho::util::urldecode(boost::trim_copy(_fields.at(name).template copied<std::string>())));
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
            return boost::trim_copy(copied<std::string>());
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

template <typename T>
struct field_value_extractor{
    typedef T value_type;
    std::string _message;
    
    field_value_extractor(const std::string& message = "Extraction Failed"): _message(message){}
    bool operator()(const std::string& input, value_type& value) const{
        return boost::conversion::try_lexical_convert<value_type>(input, value);
    }
    std::string message() const{
        return _message;
    }
};

template <>
struct field_value_extractor<std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>>{
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> value_type;
    
    std::string _format;
    std::string _message;
    
    field_value_extractor(const std::string& format, const std::string& message = "Extraction Failed"): _format(format), _message(message){}
    bool operator()(const std::string& input, value_type& value) const{
        if(!input.empty()){
            std::tm tm = {};
            std::istringstream ss(input);
            ss >> std::get_time(&tm, _format.c_str());
            if(ss.fail()){
                return false;
            }
            boost::posix_time::ptime created_time(boost::gregorian::date(tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday));
            boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
            boost::posix_time::time_duration diff = created_time - epoch;
            std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> time_chrono(std::chrono::microseconds(diff.total_microseconds()));
            value = time_chrono;
            return true;
        }
        return false;
    }
    std::string message() const{
        return _message;
    }
};

template <typename T, bool Required>
struct field;

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

struct field_common: udho::prepare<field_common>{
    std::string _name;
    bool _is_valid;
    bool _validated;
    std::string _err;
    std::string _value;
    
    field_common(const std::string& name): _name(name){
        clear();
    }
    inline bool validated() const{
        return _validated;
    }
    inline std::string name() const{
        return _name;
    }
    inline bool valid() const{
        return _is_valid;
    }
    inline std::string error() const{
        return _err;
    }
    inline void clear(){
        _validated = false;
        _is_valid = true;
        _err = std::string();
    }
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | fn("validated", &field_common::validated)
                     | fn("name",      &field_common::name)
                     | var("value",    &field_common::_value)
                     | fn("valid",     &field_common::valid)
                     | fn("error",     &field_common::error);
    }
};

template <typename T>
struct field<T, true>: field_common, field_value_extractor<T>{
    typedef field<T, true> self_type;
    typedef field_common common_type;
    typedef boost::function<bool (const std::string&, std::string&)> function_type;
    typedef field_value_extractor<T> extractor_type;
    typedef std::vector<function_type> validators_collection_type;
    
    T _value;
    validators_collection_type _validators;
    std::string _message_required;
    
    template <typename... U>
    field(const std::string& name, const std::string& message, U... args): extractor_type(args...), common_type(name), _message_required(message){}
    T value() const{
        return _value;
    }
    template <typename F>
    self_type& constrain(F ftor){
        _validators.push_back(ftor);
        return *this;
    }
    template <typename FormT>
    void validate(const FormT& form){
        if(!form.has(common_type::name()) || (form.has(common_type::name()) && form.template field<std::string>(common_type::name()).empty())){
            common_type::_is_valid = false;
            common_type::_err = _message_required;
        }else{
            std::string value = form.template field<std::string>(common_type::name());
            check(value);
        }
        common_type::_validated = true;
    }
    void check(const std::string& input){
        field_common::_value = input;
                
        if(input.empty()){
            common_type::_is_valid = false;
            common_type::_err = _message_required;
        }else{
            std::size_t counter = 0;
            for(const function_type& f: _validators){
                std::string message;
                if(!f(input, message)){
                    common_type::_is_valid = false;
                    common_type::_err = message;
                    break;
                }
                ++counter;
            }
            if(counter == _validators.size()){
                bool success = extractor_type::operator()(input, _value);
                common_type::_is_valid = success;
                if(!success){
                    common_type::_err = extractor_type::message();
                }
            }
        }
    }
    /**
     * validate the field when using a field without a form (e.g. JSON or XML document) if a value if provided in the document
     */
    self_type& operator()(const std::string& input){
        check(input);
        common_type::_validated = true;
        return *this;
    }
    /**
     * validate the field when using a field without a form (e.g. JSON or XML document) if no value if provided in the document
     */
    self_type& operator()(){
        common_type::_is_valid = false;
        common_type::_err = _message_required;
        common_type::_validated = true;
        return *this;
    }
};

template <typename T>
struct field<T, false>: field_common, field_value_extractor<T>{
    typedef field<T, false> self_type;
    typedef field_common common_type;
    typedef boost::function<bool (const std::string&, std::string&)> function_type;
    typedef field_value_extractor<T> extractor_type;
    typedef std::vector<function_type> validators_collection_type;
    
    T _value;
    validators_collection_type _validators;
    
    template <typename... U>
    field(const std::string& name, U... args): extractor_type(args...), common_type(name){}
    T value() const{
        return _value;
    }
    template <typename F>
    self_type& constrain(F ftor){
        _validators.push_back(ftor);
        return *this;
    }
    template <typename FormT>
    void validate(const FormT& form){
        if(!form.has(common_type::name())){
            common_type::_is_valid = true;
            common_type::_validated = true;
        }else{
            std::string value = form.template field<std::string>(common_type::name());
            check(value);
        }
        common_type::_validated = true;
    }
    void check(const std::string& input){
        field_common::_value = input;
        std::size_t counter = 0;
        for(const function_type& f: _validators){
            std::string message;
            if(!f(input, message)){
                common_type::_is_valid = false;
                common_type::_err = message;
                break;
            }
            ++counter;
        }
        if(counter == _validators.size()){
            bool success = extractor_type::operator()(input, _value);
            common_type::_is_valid = success;
            if(!success){
                common_type::_err = extractor_type::message();
            }
        }
    }
    /**
     * validate the field when using a field without a form (e.g. JSON or XML document) if a value if provided in the document
     */
    self_type& operator()(const std::string& input){
        check(input);
        common_type::_validated = true;
        return *this;
    }
    /**
     * validate the field when using a field without a form (e.g. JSON or XML document) if no value if provided in the document
     */
    self_type& operator()(){
        common_type::_is_valid = true;
        common_type::_validated = true;
        return *this;
    }
};

namespace form{
    
template <typename FormT = void>
struct validated: udho::prepare<validated<FormT>>{
    typedef udho::prepare<validated<FormT>> base;
    typedef FormT form_type;
    typedef validated<FormT> self_type;
    
    const form_type& _form;
    bool _submitted;
    bool _valid;
    std::vector<std::string> _errors;
    std::map<std::string, field_common> _fields;
    
    validated(const form_type& form): _form(form), _submitted(false), _valid(true){}
    void add(const field_common& fld){
        _fields.insert(std::make_pair(fld.name(), fld));
        _submitted = true;
        _valid = _valid && fld.valid();
        if(!fld.valid()){
            _errors.push_back(fld.error());
        }
    }
    bool valid() const{
        return _valid;
    }
    const std::vector<std::string>& errors() const{
        return _errors;
    }
    const field_common& operator[](const std::string& name) const {
        return _fields.at(name);
    }
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | base::var("submitted", &self_type::_submitted)
                     | base::var("valid",     &self_type::_valid)
                     | base::var("errors",    &self_type::_errors)
                     | base::var("fields",    &self_type::_fields);
    }
};

template <>
struct validated<void>: udho::prepare<validated<void>>{
    typedef udho::prepare<validated<>> base;
    typedef validated<void> self_type;
    
    bool _submitted;
    bool _valid;
    std::vector<std::string> _errors;
    std::map<std::string, field_common> _fields;
    
    validated(): _submitted(false), _valid(true){}
    void add(const field_common& fld){
        _fields.insert(std::make_pair(fld.name(), fld));
        _submitted = true;
        _valid = _valid && fld.valid();
        if(!fld.valid()){
            _errors.push_back(fld.error());
        }
    }
    bool valid() const{
        return _valid;
    }
    const std::vector<std::string>& errors() const{
        return _errors;
    }
    const field_common& operator[](const std::string& name) const {
        return _fields.at(name);
    }
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | base::var("submitted", &self_type::_submitted)
                     | base::var("valid",     &self_type::_valid)
                     | base::var("errors",    &self_type::_errors)
                     | base::var("fields",    &self_type::_fields);
    }
};

inline validated<void> validate(){
    return validated<void>();
}

template <typename RequestT>
validated<udho::form_<RequestT>> validate(udho::form_<RequestT>& form){
    return validated<udho::form_<RequestT>>(form);
}

template <typename IteratorT>
validated<udho::multipart_form<IteratorT>> validate(const udho::multipart_form<IteratorT>& form){
    return validated<udho::multipart_form<IteratorT>>(form);
}

template <typename T, bool Required, typename RequestT>
validated<udho::form_<RequestT>>& operator<<(validated<udho::form_<RequestT>>& validator, field<T, Required>& field_){
    field_.validate(validator._form);
    validator.add(field_);
    return validator;
}

template <typename T, bool Required, typename IteratorT>
validated<udho::multipart_form<IteratorT>>& operator<<(validated<udho::multipart_form<IteratorT>>& validator, field<T, Required>& field_){
    field_.validate(validator._form);
    validator.add(field_);
    return validator;
}

template <typename T, bool Required>
validated<>& operator<<(validated<>& validator, field<T, Required>& field_){
    validator.add(field_);
    return validator;
}

template <typename T>
using required = field<T, true>;
template <typename T>
using optional = field<T, false>;
  
namespace validators{
struct max_length{
    std::size_t _length;
    std::string _custom;
    
    max_length(std::size_t length, std::string custom = ""): _length(length), _custom(custom){}
    inline bool operator()(const std::string& value, std::string& error) const{
        if(value.size() > _length){
            error = _custom.empty() ? (boost::format("constrain size <= %1% failed") % _length).str() : _custom;
            return false;
        }
        return true;
    }
};
struct min_length{
    std::size_t _length;
    std::string _custom;
    
    min_length(std::size_t length, std::string custom = ""): _length(length), _custom(custom){}
    inline bool operator()(const std::string& value, std::string& error) const{
        if(value.size() < _length){
            error = _custom.empty() ? (boost::format("constrain size >= %1% failed") % _length).str() : _custom;
            return false;
        }
        return true;
    }
};
struct exact_length{
    std::size_t _length;
    std::string _custom;
    
    exact_length(std::size_t length, std::string custom = ""): _length(length), _custom(custom){}
    inline bool operator()(const std::string& value, std::string& error) const{
        if(value.size() != _length){
            error = _custom.empty() ? (boost::format("constrain size == %1% failed") % _length).str() : _custom;
            return false;
        }
        return true;
    }
};
struct no_space{
    std::string _custom;
    
    no_space(std::string custom = ""): _custom(custom){}
    inline bool operator()(const std::string& value, std::string& error) const{
        if(value.find(" ") != std::string::npos){
            error = _custom.empty() ? (boost::format("constrain no space failed")).str() : _custom;
            return false;
        }
        return true;
    }
};
struct all_digit{
    std::string _custom;
    
    all_digit(std::string custom = ""): _custom(custom){}
    inline bool operator()(const std::string& value, std::string& error) const{
        for(auto it = value.begin(); it != value.end(); ++it){
            char c = *it;
            if(!std::isdigit(c)){
                error = _custom.empty() ? (boost::format("constrain all digits failed")).str() : _custom;
                return false;
            }
        }
        return true;
    }
};

struct date_time{
    std::string _format;
    std::string _custom;
    
    date_time(std::string format, std::string custom = ""): _format(format), _custom(custom){}
    inline bool operator()(const std::string& value, std::string& error) const{
        std::tm tm = {};
        std::istringstream ss(value);
        ss >> std::get_time(&tm, _format.c_str());
        if(ss.fail()){
            error = _custom.empty() ? (boost::format("constain date time format %1% failed") % _format).str() : _custom;
            return false;
        }
        return true;
    }
};


}

}

}

#endif // UDHO_FORM_H
