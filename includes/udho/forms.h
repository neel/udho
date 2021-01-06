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

#ifndef UDHO_FORMS_H
#define UDHO_FORMS_H

#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <udho/util.h>
#include <udho/access.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/format.hpp>

namespace udho{
namespace forms{
    
static constexpr const char* default_datetime_format = "%Y-%m-%d %H:%M:%S";
    
template <typename V, typename U>
struct deserializer;

template <typename V>
struct deserializer<V, std::string>{
    static bool check(const std::string& input){
        V value;
        return boost::conversion::try_lexical_convert<V>(input, value);
    }
    static V deserialize(const std::string& input){
        V value;
        boost::conversion::try_lexical_convert<V>(input, value);
        return value;
    }
};

template <typename U>
struct deserializer<std::string, U>{
    static bool check(const U& input){
        std::string value;
        return boost::conversion::try_lexical_convert<std::string>(input, value);
    }
    static std::string deserialize(const U& input){
        std::string value;
        boost::conversion::try_lexical_convert<std::string>(input, value);
        return value;
    }
};

template <typename U>
struct deserializer<U, U>{
    static bool check(const U&){
        return true;
    }
    static std::string deserialize(const U& input){
        return input;
    }
};

template <>
struct deserializer<std::string, std::string>{
    static bool check(const std::string&){
        return true;
    }
    static std::string deserialize(const std::string& input){
        return input;
    }
};

template <typename DurationT>
struct deserializer<std::chrono::time_point<std::chrono::system_clock, DurationT>, std::string>{
    typedef std::chrono::time_point<std::chrono::system_clock, DurationT> time_type;
    
    static bool check(const std::string& input, const std::string& format = default_datetime_format){
        std::tm tm = {};
        std::istringstream ss(input);
        ss >> std::get_time(&tm, format.c_str());
        return !ss.fail();
    }
    static time_type deserialize(const std::string& input, const std::string& format = default_datetime_format){
        std::tm tm = {};
        std::istringstream ss(input);
        ss >> std::get_time(&tm, format.c_str());
        if(ss.fail()){
            return time_type();
        }
        std::time_t tt = std::mktime(&tm);
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(tt);
        
        return std::chrono::time_point_cast<DurationT>(tp);
    }
};

template <typename DurationT>
struct deserializer<std::string, std::chrono::time_point<std::chrono::system_clock, DurationT>>{
    typedef std::chrono::time_point<std::chrono::system_clock, DurationT> time_type;
    
    static bool check(const std::chrono::time_point<std::chrono::system_clock, DurationT>& /*input*/, const std::string& format = default_datetime_format){
        return true;
    }
    static std::string deserialize(const std::chrono::time_point<std::chrono::system_clock, DurationT>& input, const std::string& format = default_datetime_format){
        auto tt = std::chrono::system_clock::to_time_t(input);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&tt), format.c_str());
        return ss.str();
    }
};
    
template <typename T>
struct parser{
    template <typename U, typename... ArgsT>
    static bool parsable(const U& input, const ArgsT&... args){
        return deserializer<T, U>::check(input, args...);
    }
    template <typename U, typename... ArgsT>
    static T parse(const U& input, const ArgsT&... args){
        return deserializer<T, U>::deserialize(input, args...);
    }
};

namespace drivers{
    
namespace detail{
    template <typename RequestT>
    struct extract_multipart_boundary{
        typedef RequestT request_type;
        
        request_type _request;
        std::string  _boundary;
        
        extract_multipart_boundary(const request_type& request): _request(request){}
        const std::string& boundary() const { return _boundary; }
        bool extract(const std::string& boundary_key = "boundary="){
            std::string content_type(_request[boost::beast::http::field::content_type]);
            std::string::size_type index = content_type.find(boundary_key);
            if(index == std::string::npos){
                return false;
            }else{
                _boundary = "--"+content_type.substr(index+boundary_key.size());
                return true;
            }
        }
    };
}
    
/**
 * Form driver for urlencoded forms
 */
template <typename Iterator = std::string::const_iterator>
struct urlencoded_{
    typedef Iterator iterator_type;
    typedef typename std::iterator_traits<iterator_type>::value_type value_type;
    typedef std::basic_string<value_type> string_type;
    typedef bounded_str<iterator_type> bounded_string_type;
    typedef std::map<string_type, bounded_string_type> header_map_type;
    typedef bounded_string_type bounded_string;
    
    header_map_type _fields;
    std::string _query;
            
    inline void parse(iterator_type begin, iterator_type end){
        iterator_type last  = begin;
        while(true){
            iterator_type it     = std::find(last, end, '&');
            iterator_type assign = std::find(last, it, '=');
            bounded_string_type key(last, assign);
            bounded_string_type value(assign+1, it);
            
            std::string key_str = key.template copied<std::string>();
            
//                 std::cout << "Key ^"   << key.  template copied<std::string>() << "$" << std::endl;
//                 std::cout << "value ^" << value.template copied<std::string>() << "$" << std::endl;
            
            _fields.insert(std::make_pair(key_str, value));
            if(it == end){
                break;
            }else{
                last = it+1;
            }
        }
    }
    /**
        * checks whether the value for the field is empty
        */
    inline bool empty(const std::string& name) const{
        auto it = _fields.find(name);
        if(it != _fields.end()){
            return it->second.size() == 0;
        }
        return true;
    }
    
    /**
    * checks whether there exists any field with the name provided
    */
    inline bool exists(const std::string& name) const{
        return _fields.find(name) != _fields.end();
    }
    
    template <typename T, typename ParserT = udho::forms::parser<T>, typename... ArgsT>
    bool parsable(const std::string& name, const ArgsT&... args) const{
        auto it = _fields.find(name);
        if(it != _fields.end()){
            std::string raw = it->second.template copied<std::string>();
            return ParserT::parsable(raw, args...);
        }
        return false;
    }
    
    /**
        * returns the value of the field with the name provided lexically casted to type T
        */
    template <typename T, typename ParserT = udho::forms::parser<T>, typename... ArgsT>
    const T parsed(const std::string& name, const ArgsT&... args) const{
        auto it = _fields.find(name);
        if(it != _fields.end()){
            std::string raw = it->second.template copied<std::string>();
            return ParserT::parse(raw, args...);
        }
        return T();
    }
};
typedef urlencoded_<std::string::const_iterator> urlencoded_raw;

/**
 * Form accessor for multipart forms
 */
template <typename Iterator = std::string::const_iterator>
struct multipart_{
    typedef Iterator iterator_type;
    typedef typename std::iterator_traits<iterator_type>::value_type value_type;
    typedef std::basic_string<value_type> string_type;
    typedef bounded_str<iterator_type> bounded_string_type;
    typedef std::map<string_type, bounded_string_type> header_map_type;
    typedef bounded_string_type bounded_string;
    
    /**
    * A part in the multipart form data
    */
    struct form_part{
        header_map_type _headers;
        bounded_string_type _body;
        
        form_part(iterator_type begin, iterator_type end): _body(begin, end){}
        
        void set_header(const header_map_type& headers){
            _headers = headers;
        }
        /**
        * returns the value associated with the key in the header of the part
        * \code
        * header("Content-Disposition").copied<std::string>();
        * \endcode
        */
        const bounded_string_type& header(const std::string& key) const{
            return _headers.at(key);
        }
        /**
        * returns the value associated with the key and sub key in the header of the part
        * \code
        * header("Content-Disposition", "name").copied<std::string>();
        * \endcode
        */
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
        /**
        * name of the part
        * \code
        * name().copied<std::string>();
        * \endcode
        */
        bounded_string_type name() const{
            return header("Content-Disposition", "name");
        }
        /**
        * filename of the part
        * \code
        * name().copied<std::string>();
        * \endcode
        */
        bounded_string_type filename() const{
            return header("Content-Disposition", "filename");
        }
        /**
        * body of the part returned as a pair of string iterators
        * \code
        * body().copied<std::string>();
        * \endcode
        */
        const bounded_string_type& body() const{
            return _body;
        }
        template <typename StrT>
        StrT copied() const{
            return body().template copied<StrT>();
        }
        /**
        * returns the body of the part as string
        */
        std::string str() const{
            return boost::trim_copy(copied<std::string>());
        }
        
        template <typename T, typename ParserT = udho::forms::parser<T>, typename... ArgsT>
        bool parsable(const ArgsT&... args) const{
            return ParserT::parsable(str(), args...);
        }
        bool empty() const{
            return _body.size() == 0;
        }
        /**
        * returns the value of the field with the name provided lexically casted to type T
        */
        template <typename T, typename ParserT = udho::forms::parser<T>, typename... ArgsT>
        const T parsed(const ArgsT&... args) const{
            if(parsable<T, ParserT>()){
                return ParserT::parse(str(), args...);
            }
            return T();
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
    /**
    * number of fields in the form
    */
    std::size_t count() const{
        return _parts.size();
    }
    /**
    * checks whether the form has any field with the given name
    */
    bool exists(const std::string name) const{
        return _parts.find(name) != _parts.end();
    }
    bool empty(const std::string name) const{
        if(exists(name)){
            return part(name).empty();
        }
        return true;
    }
    /**
    * returns the part associated with the given name
    */
    const form_part& part(const std::string& name) const{
        return _parts.at(name);
    }
    
    template <typename T, typename ParserT = udho::forms::parser<T>, typename... ArgsT>
    bool parsable(const std::string& name, const ArgsT&... args) const{
        if(exists(name)){
            return part(name).template parsable<T, ParserT>(args...);
        }
        return false;
    }
    
    /**
    * returns the value of the field with the name provided lexically casted to type T
    */
    template <typename T, typename ParserT = udho::forms::parser<T>, typename... ArgsT>
    const T parsed(const std::string& name, const ArgsT&... args) const{
        if(exists(name)){
            return part(name).template parsed<T, ParserT>(args...);
        }
        return T();
    }
};

typedef multipart_<std::string::const_iterator> multipart_raw;

template <typename RequestT>
struct urlencoded: urlencoded_<typename RequestT::body_type::value_type::const_iterator>{
    typedef RequestT request_type;
    typedef typename request_type::body_type::value_type body_type;
    typedef urlencoded_<typename request_type::body_type::value_type::const_iterator> urlencoded_type;
    
    const request_type& _request;
    
    urlencoded(const request_type& request): _request(request){
        urlencoded_type::parse(_request.body().begin(), _request.body().end());
    }
};

template <typename RequestT>
struct multipart: multipart_<typename RequestT::body_type::value_type::const_iterator>{
    typedef RequestT request_type;
    typedef typename request_type::body_type::value_type body_type;
    typedef multipart_<typename request_type::body_type::value_type::const_iterator> multipart_type;
    
    const request_type& _request;
    multipart(const request_type& request): _request(request){
        detail::extract_multipart_boundary<request_type> boundary_extractor(_request);
        if(!boundary_extractor.extract()){
            std::cout << "multipart boundary not found in Content-Type: " << _request[boost::beast::http::field::content_type] << std::endl;
            return;
        }else{
            std::string boundary = boundary_extractor.boundary();
            // std::cout << _request << std::endl << "#################### " << std::endl << "BOUNDARY =(" << boundary << ")" << std::endl;
            multipart_type::parse(boundary, _request.body().begin(), _request.body().end());
        }
    }
};

template <typename RequestT>
struct combo: private urlencoded_<typename RequestT::body_type::value_type::const_iterator>, private multipart_<typename RequestT::body_type::value_type::const_iterator>{
    enum class types{
        unparsed,
        urlencoded,
        multipart
    };
    
    typedef RequestT request_type;
    typedef urlencoded_<typename request_type::body_type::value_type::const_iterator> urlencoded_type;
    typedef multipart_<typename request_type::body_type::value_type::const_iterator> multipart_type;
    typedef typename request_type::body_type::value_type body_type;
    
    const request_type& _request;
    types _type;
    
    combo(const request_type& request): _request(request), _type(types::unparsed){
        if(_request[boost::beast::http::field::content_type].find("application/x-www-form-urlencoded") != std::string::npos){
            parse_urlencoded();
        }else if(_request[boost::beast::http::field::content_type].find("multipart/form-data") != std::string::npos){
            parse_multipart();
        }
    }
    /**
     * parse the beast request body as urlencoded form data
     */
    void parse_urlencoded(){
        urlencoded_type::parse(_request.body().begin(), _request.body().end());
        _type = types::urlencoded;
    }
    /**
     * parse the beast request body as multipart form data
     */
    void parse_multipart(){
        detail::extract_multipart_boundary<request_type> boundary_extractor(_request);
        if(!boundary_extractor.extract()){
            std::cout << "multipart boundary not found in Content-Type: " << _request[boost::beast::http::field::content_type] << std::endl;
            return;
        }else{
            std::string boundary = boundary_extractor.boundary();
            // std::cout << _request << std::endl << "#################### " << std::endl << "BOUNDARY =(" << boundary << ")" << std::endl;
            multipart_type::parse(boundary, _request.body().begin(), _request.body().end());
        }
    }
    /**
     * check whether the submitted form is urlencoded
     */
    bool is_urlencoded() const{
        return _type == types::urlencoded;
    }
    /**
     * check whether the submitted form is multipart
     */
    bool is_multipart() const{
        return _type == types::multipart;
    }
    const urlencoded_type& urlencoded() const{
        return static_cast<const urlencoded_type&>(*this);
    }
    /**
     * return the multipart specific form accessor
     */
    const multipart_type& multipart() const{
        return static_cast<const multipart_type&>(*this);
    }
    /**
        * checks whether the value for the field is empty
        */
    inline bool empty(const std::string name) const{
        if(_type == types::urlencoded){
            return urlencoded_type::empty(name);
        }else if(_type == types::multipart){
            return multipart_type::empty(name);
        }else{
            return true;
        }
    }
    
    /**
    * checks whether there exists any field with the name provided
    */
    inline bool exists(const std::string name) const{
        if(_type == types::urlencoded){
            return urlencoded_type::exists(name);
        }else if(_type == types::multipart){
            return multipart_type::exists(name);
        }else{
            return false;
        }
    }
    
    template <typename T, typename ParserT = udho::forms::parser<T>>
    bool parsable(const std::string& name) const{
        if(_type == types::urlencoded){
            return urlencoded_type::template parsable<T, ParserT>(name);
        }else if(_type == types::multipart){
            return multipart_type::template parsable<T, ParserT>(name);
        }else{
            return false;
        }
    }
    
    /**
        * returns the value of the field with the name provided lexically casted to type T
        */
    template <typename T, typename ParserT = udho::forms::parser<T>>
    const T parsed(const std::string& name) const{
        if(_type == types::urlencoded){
            return urlencoded_type::template parsed<T, ParserT>(name);
        }else if(_type == types::multipart){
            return multipart_type::template parsed<T, ParserT>(name);
        }else{
            return T();
        }
    }
};

    
}

template <typename DriverT>
struct form: public DriverT{
    
    using DriverT::DriverT;
    
    bool has(const std::string& name) const {
        return DriverT::exists(name);
    }
    template <typename T, typename ParserT = udho::forms::parser<T>>
    T field(const std::string& name, bool* ok = 0x0) const {
        if(!DriverT::template parsable<T, ParserT>(name) || DriverT::empty(name)){
            if(ok){
                *ok = false;
            }
            return T();
        }
        if(ok){
            *ok = true;
        }
        return DriverT::template parsed<T, ParserT>(name);
    }
    template <typename T, typename ParserT = udho::forms::parser<T>, typename... ArgsT>
    T field(const std::string& name, bool* ok, const ArgsT&... args) const {
        if(!DriverT::template parsable<T, ParserT>(name, args...) || DriverT::empty(name)){
            if(ok){
                *ok = false;
            }
            return T();
        }
        if(ok){
            *ok = true;
        }
        return DriverT::template parsed<T, ParserT>(name, args...);
    }
    template <typename T, typename ParserT = udho::forms::parser<T>, typename... ArgsT>
    T field(const std::string& name, const ArgsT&... args) const {
        bool okay;
        return field<T, ParserT, ArgsT...>(name, &okay, args...);
    }
};


using query_ = form<drivers::urlencoded_raw>;
template <typename RequestT>
using form_ = form<drivers::combo<RequestT>>;
template <typename RequestT>
using form_multipart_ = form<drivers::multipart<RequestT>>;

template <typename RequestT>
form_multipart_<RequestT> form_multipart(const RequestT& req){
    return form_multipart_<RequestT>(req);
}

template <typename T, bool Required = false>
struct field;

namespace detail{
    
    template <typename ConstrainedFieldT, unsigned Depth>
    struct constraint_visitor_{
        typedef typename constraint_visitor_<typename ConstrainedFieldT::head_type, Depth-1>::validator_type validator_type;

        static validator_type& validator(ConstrainedFieldT& cf){
            return constraint_visitor_<typename ConstrainedFieldT::head_type, Depth-1>::validator(cf._head);
        }
    };
    
    template <typename ConstrainedFieldT>
    struct constraint_visitor_<ConstrainedFieldT, 0>{
        typedef typename ConstrainedFieldT::validator_type validator_type;
        
        static validator_type& validator(ConstrainedFieldT& cf){
            return cf._validator;
        }
    };
    
    template <typename ConstrainedFieldT, unsigned Depth>
    using constraint_visitor = constraint_visitor_<ConstrainedFieldT, ConstrainedFieldT::depth - Depth>;
    
    template <typename ValidatorT, typename FieldT>
    struct constrained_field{
        typedef FieldT field_type;
        typedef typename field_type::value_type value_type;
        typedef ValidatorT validator_type;
        typedef constrained_field<validator_type, field_type> self_type;
        
        enum {depth = 1};
        
        validator_type _validator;
        FieldT _field;
        
        constrained_field(const validator_type& validator, const FieldT& field): _validator(validator), _field(field){}
        template <typename FormT>
        bool validate(const FormT& form){
            return _field.validate(form, _validator);
        }
        template <typename FormT>
        bool operator()(const FormT& form){
            return validate(form);
        }
        inline const field_type& field() const { return _field; }
        inline field_type& field() { return _field; }
        inline bool valid() const { return field().valid(); }
        inline value_type value() const { return field().value(); }
        inline std::string message() const { return field().message(); }
        inline value_type operator*() const { return value(); }
        inline bool operator!() const { return !valid(); }
        inline std::string name() const { return field().name(); }
        
        template <typename ValidatorU>
        detail::constrained_field<ValidatorU, self_type> constrain(const ValidatorU& validator){
            return detail::constrained_field<ValidatorU, self_type>(validator, *this);
        }
        
        template <template<typename> class ValidatorU, typename... ArgsT>
        detail::constrained_field<ValidatorU<value_type>, self_type> constrain(ArgsT&&... args){
            return constrain<ValidatorU<value_type>>(ValidatorU<value_type>(args...));
        }
        
        template <unsigned Depth>
        typename udho::forms::detail::constraint_visitor<self_type, Depth>::validator_type& constrain(){
            return udho::forms::detail::constraint_visitor<self_type, Depth>::validator(*this);
        }
    };
    
    template <typename ValidatorT, typename... T>
    struct constrained_field<ValidatorT, constrained_field<T...>>{
        typedef constrained_field<T...> head_type;
        typedef typename head_type::field_type field_type;
        typedef typename field_type::value_type value_type;
        typedef ValidatorT validator_type;
        typedef constrained_field<validator_type, head_type> self_type;
        
        enum {depth = head_type::depth +1};
        
        validator_type _validator;
        head_type _head;
        
        constrained_field(const validator_type& validator, const head_type& head): _validator(validator), _head(head){}
        template <typename FormT>
        bool validate(const FormT& form){
            bool valid = _head.validate(form);
            return valid && field().validate(form, _validator);
        }
        template <typename FormT>
        bool operator()(const FormT& form){
            return validate(form);
        }
        
        inline field_type& field() { return _head.field(); }
        inline const field_type& field() const { return _head.field(); }
        inline bool valid() const { return field().valid(); }
        inline value_type value() const { return field().value(); }
        inline std::string message() const { return field().message(); }
        inline value_type operator*() const { return value(); }
        inline bool operator!() const { return !valid(); }
        inline std::string name() const { return field().name(); }
        
        template <typename ValidatorU>
        detail::constrained_field<ValidatorU, self_type> constrain(const ValidatorU& validator){
            return detail::constrained_field<ValidatorU, self_type>(validator, *this);
        }
        
        template <template<typename> class ValidatorU, typename... ArgsT>
        detail::constrained_field<ValidatorU<value_type>, self_type> constrain(ArgsT&&... args){
            ValidatorU<value_type> validator(args...);
            return constrain<ValidatorU<value_type>>(validator);
        }
        
        template <unsigned Depth>
        typename udho::forms::detail::constraint_visitor<self_type, Depth>::validator_type& constrain(){
            return udho::forms::detail::constraint_visitor<self_type, Depth>::validator(*this);
        }
    };
}

namespace detail{

struct field_common: udho::prepare<field_common>{
    field_common(const std::string& name): _name(name), _absent(true), _unparsable(true), _fetched(false), _valid(true){}
    bool absent() const { return _absent; }
    bool unparsable() const { return _unparsable; }
    bool fetched() const { return _fetched; }
    const std::string& name() const { return _name; }
    std::string value_serialized() const { return _value_str; }
    bool valid() const{ return _valid; }
    const std::string& message() const { return _message; }
    const std::string& error() const { return _message; }
    inline bool operator!() const { return !valid(); }
    
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | fn("fetched", &field_common::fetched)
                     | fn("name",    &field_common::name)
                     | fn("value",   &field_common::value_serialized)
                     | fn("valid",   &field_common::valid)
                     | fn("error",   &field_common::error)
                     | fn("message", &field_common::error);
    }
    
    protected:
        std::string _name;
        std::string _message;
        bool _absent;
        bool _unparsable;
        bool _fetched;
        bool _valid;
        std::string _value_str;
};
    
template <typename ValueT>
struct field_data: field_common{
    typedef ValueT value_type;
    typedef field_data<ValueT> self_type;
    
    using field_common::field_common;
    
    inline const value_type& value() const { return _value; }
    inline const value_type& operator*() const { return value(); }
    template <typename... ArgsT>
    void value(const value_type& v, const ArgsT&... args) { _value = v; field_common::_value_str = parser<std::string>::parse(v, args...); }
    field_common& common() { return static_cast<field_common&>(*this); }
    const field_common& common() const { return static_cast<const field_common&>(*this); }
    private:    
        value_type  _value;
};
    
template <typename ValueT, typename DerivedT>
struct basic_field: field_data<ValueT>{
    typedef ValueT value_type;
    typedef DerivedT derived_type;
    typedef field_data<ValueT> data_type;
    
    basic_field(const std::string& name): data_type(name), _message_absent((boost::format("%1% absent") % name).str()), _message_unparsable((boost::format("%1% malformed") % name).str()) {}
    derived_type& absent(const std::string& message) { _message_absent = message; return self(); }
    derived_type& unparsable(const std::string& message) { _message_unparsable = message; return self(); }
    data_type& data() { return static_cast<data_type&>(*this);}
    const data_type& data() const { return static_cast<const data_type&>(*this);}
    
    template <typename ValidatorT>
    detail::constrained_field<ValidatorT, derived_type> constrain(const ValidatorT& validator){
        return detail::constrained_field<ValidatorT, derived_type>(validator, self());
    }
    
    template <template<typename> class ValidatorT, typename... ArgsT>
    detail::constrained_field<ValidatorT<value_type>, derived_type> constrain(ArgsT&&... args){
        return detail::constrained_field<ValidatorT<value_type>, derived_type>(ValidatorT<value_type>(args...), self());
    }
    
    protected:
        template <typename ValidatorT>
        bool validate(const ValidatorT& validator){
            if(data_type::valid() && !data_type::absent()){
                data_type::_valid = data_type::_valid && validator(data_type::value());
                if(!data_type::_valid){
                    data_type::_message = validator.message();
                }
                return data_type::_valid;
            }
            return false;
        }
        
        derived_type& self() { return static_cast<derived_type&>(*this); }
        const derived_type& self() const { return static_cast<const derived_type&>(*this); }
        
        std::string _message_absent;
        std::string _message_unparsable;
};

}

template <typename T>
struct field<T, true>: detail::basic_field<T, field<T, true>>{
    typedef T value_type;
    typedef field<T, true> self_type;
    typedef detail::basic_field<T, field<T, true>> base;
    enum { is_required = true };
    
    field(const std::string& name): base(name){}
    template <typename FormT>
    bool fetch(const FormT& form){
        if(base::_fetched) return base::valid();
        base::_absent  = !form.has(base::_name);
        if(!base::_absent){
            bool okay = true;
            base::value(form.template field<value_type>(base::_name, &okay));
            base::_unparsable = !okay;
            if(base::_unparsable){
                base::_message = base::_message_unparsable.empty() ? base::_message_absent : base::_message_unparsable;
            }
        }else{
            base::_message = base::_message_absent;
        }
        base::_valid = !base::_absent && !base::_unparsable;
        base::_fetched = true;
        return base::valid();
    }
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        return base::template validate<ValidatorT>(validator);
    }
};

template <typename T>
struct field<T, false>: detail::basic_field<T, field<T, false>>{
    typedef T value_type;
    typedef field<T, false> self_type;
    typedef detail::basic_field<T, field<T, false>> base;
    enum { is_required = false };
    
    field(const std::string& name, const value_type& def = value_type()): base(name), _def(def){}
    const value_type& def() const { return _def; }
    template <typename FormT>
    bool fetch(const FormT& form){
        if(base::_fetched) return base::valid();
        base::_absent  = !form.has(base::_name);
        if(!base::_absent){
            bool okay = true;
            base::value(form.template field<value_type>(base::_name, &okay));
            base::_unparsable = !okay;
            base::_valid = !base::_unparsable;
            if(base::_unparsable){
                base::_message = base::_message_unparsable.empty() ? base::_message_absent : base::_message_unparsable;
            }
        }else{
            base::_valid = true;
            base::value(_def);
        }
        base::_fetched = true;
        return base::valid();
    }
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        return base::template validate<ValidatorT>(validator);
    }
    
    protected:
        value_type _def;
};

template <typename DurationT>
struct field<std::chrono::time_point<std::chrono::system_clock, DurationT>, true>: detail::basic_field<std::chrono::time_point<std::chrono::system_clock, DurationT>, field<std::chrono::time_point<std::chrono::system_clock, DurationT>, true>>{
    typedef std::chrono::time_point<std::chrono::system_clock, DurationT> value_type;
    typedef field<std::chrono::time_point<std::chrono::system_clock, DurationT>, true> self_type;
    typedef detail::basic_field<std::chrono::time_point<std::chrono::system_clock, DurationT>, field<std::chrono::time_point<std::chrono::system_clock, DurationT>, true>> base;
    enum { is_required = true };
    
    std::string _format;
    
    field(const std::string& name, const std::string& format = default_datetime_format): base(name), _format(format){}
    template <typename FormT>
    bool fetch(const FormT& form){
        if(base::_fetched) return base::valid();
        base::_absent  = !form.has(base::_name);
        if(!base::_absent){
            bool okay = true;
            base::value(form.template field<value_type>(base::_name, &okay, _format), _format);
            base::_unparsable = !okay;
            if(base::_unparsable){
                base::_message = base::_message_unparsable.empty() ? base::_message_absent : base::_message_unparsable;
            }
        }else{
            base::_message = base::_message_absent;
        }
        base::_valid = !base::_absent && !base::_unparsable;
        base::_fetched = true;
        return base::valid();
    }
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        return base::template validate<ValidatorT>(validator);
    }
};

template <typename DurationT>
struct field<std::chrono::time_point<std::chrono::system_clock, DurationT>, false>: detail::basic_field<std::chrono::time_point<std::chrono::system_clock, DurationT>, field<std::chrono::time_point<std::chrono::system_clock, DurationT>, false>>{
    typedef std::chrono::time_point<std::chrono::system_clock, DurationT> value_type;
    typedef field<std::chrono::time_point<std::chrono::system_clock, DurationT>, false> self_type;
    typedef detail::basic_field<std::chrono::time_point<std::chrono::system_clock, DurationT>, field<std::chrono::time_point<std::chrono::system_clock, DurationT>, false>> base;
    enum { is_required = false };
    
    std::string _format;
    
    field(const std::string& name, const std::string& format = default_datetime_format, const value_type& def = value_type()): base(name), _format(format), _def(def){}
    const value_type& def() const { return _def; }
    template <typename FormT>
    bool fetch(const FormT& form){
        if(base::_fetched) return base::valid();
        base::_absent  = !form.has(base::_name);
        if(!base::_absent){
            bool okay = true;
            base::value(form.template field<value_type>(base::_name, &okay, _format), _format);
            base::_unparsable = !okay;
            base::_valid = !base::_unparsable;
            if(base::_unparsable){
                base::_message = base::_message_unparsable.empty() ? base::_message_absent : base::_message_unparsable;
            }
        }else{
            base::_valid = true;
            base::value(_def);
        }
        base::_fetched = true;
        return base::valid();
    }
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        return base::template validate<ValidatorT>(validator);
    }
    
    protected:
        value_type _def;
};
    
template <typename DriverT, typename ValidatorT, typename FieldT>
form<DriverT>& operator>>(form<DriverT>& form, detail::constrained_field<ValidatorT, FieldT>& field){
    field(form);
    return form;
}

template <typename DriverT, typename T, bool Required>
form<DriverT>& operator>>(form<DriverT>& form, field<T, Required>& field){
    field.fetch(form);
    return form;
}

template <typename T>
using required = field<T, true>;

template <typename T>
using optional = field<T, false>;


struct accumulated: udho::prepare<accumulated>{
    typedef udho::prepare<accumulated> base;
    typedef accumulated self_type;
    
    inline accumulated(): _valid(true), _submitted(false){}
    
    template <typename ValidatorT, typename TailT>
    self_type& add(const detail::constrained_field<ValidatorT, TailT>& cf){
        _submitted = _submitted || cf.field().fetched();
        return add(cf.field().common());
    }
    inline self_type& add(const detail::field_common& common){
        bool valid = common.valid();
        _valid = _valid && valid;
        if(!valid){
            _errors.push_back(common.message());
        }
        _fields.insert(std::make_pair(common.name(), common));
        return *this;
    }
    
    inline bool valid() const { return _valid; }
    
    inline void add_error(const std::string& err){
        _errors.push_back(err);
    }
    
    const std::vector<std::string>& errors() const{
        return _errors;
    }
    
    const detail::field_common& operator[](const std::string& name) const{
        return _fields.at(name);
    }
    
    template <typename DictT>
    auto dict(DictT assoc) const{
        return assoc | base::var("submitted", &self_type::_submitted)
                     | base::var("valid",     &self_type::_valid)
                     | base::var("errors",    &self_type::_errors)
                     | base::var("fields",    &self_type::_fields);
    }
    
    private:
        bool _valid;
        bool _submitted;
        std::vector<std::string> _errors;
        std::map<std::string, detail::field_common> _fields;
};

template <typename ValidatorT, typename FieldT>
accumulated& operator<<(accumulated& acc, detail::constrained_field<ValidatorT, FieldT>& field){
    return acc.add(field);
}

template <typename T, bool Required>
accumulated& operator<<(accumulated& acc, field<T, Required>& field){
    return acc.add(field);
}

template <typename FormT>
struct validated;

template <typename DriverT>
struct validated<form<DriverT>>: accumulated{
    typedef form<DriverT> form_type;
    typedef validated<form<DriverT>> self_type;
    
    validated(form_type& f): _form(f){}
    template <typename ValidatorT, typename TailT>
    self_type& add(detail::constrained_field<ValidatorT, TailT>& cf){
        _form >> cf;
        self() << cf;
        return *this;
    }
    template <typename T, bool Required>
    self_type& add(field<T, Required>& f){
        _form >> f;
        self() << f;
        return *this;
    }
    
    private:
        accumulated& self() { return static_cast<accumulated&>(*this); }
        form_type& _form;
};

template <typename DriverT>
validated<form<DriverT>> validate(form<DriverT>& f){
    return validated<form<DriverT>>(f);
}

template <typename DriverT, typename ValidatorT, typename FieldT>
validated<form<DriverT>>& operator>>(validated<form<DriverT>>& vf, detail::constrained_field<ValidatorT, FieldT>& field){
    return vf.add(field);
}

template <typename DriverT, typename T, bool Required>
validated<form<DriverT>>& operator>>(validated<form<DriverT>>& vf, field<T, Required>& field){
    return vf.add(field);
}

namespace constraints{
    
template <typename DerivedT>
struct basic_constrain{
    typedef DerivedT derived_type;
    
    basic_constrain(const std::string& m): _message(m){}
    derived_type& self() { return static_cast<derived_type&>(*this); }
    derived_type& error(const std::string& m) { _message = m; return self(); }
    derived_type& message(const std::string& m) { _message = m; return self(); }
    const std::string& message() const { return _message; }
    private:
        std::string _message;
};
    
template <typename T>
struct gte: basic_constrain<gte<T>>{
    typedef basic_constrain<gte<T>> base;
    
    T _value;
    
    gte(const T& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const T& input) const{ return input >= _value; }
};
template <typename T>
struct gt: basic_constrain<gt<T>>{
    typedef basic_constrain<gt<T>> base;
    
    T _value;
    
    gt(const T& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const T& input) const{ return input > _value; }
};
template <typename T>
struct lte: basic_constrain<lte<T>>{
    typedef basic_constrain<lte<T>> base;
    
    T _value;
    
    lte(const T& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const T& input) const{ return input <= _value; }
};
template <typename T>
struct lt: basic_constrain<lt<T>>{
    typedef basic_constrain<lt<T>> base;
    
    T _value;
    
    lt(const T& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const T& input) const{ return input < _value; }
};
template <typename T>
struct eq: basic_constrain<eq<T>>{
    typedef basic_constrain<eq<T>> base;
    
    T _value;
    
    eq(const T& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const T& input) const{ return input == _value; }
};

template <typename T>
struct neq: basic_constrain<neq<T>>{
    typedef basic_constrain<neq<T>> base;
    
    T _value;
    
    neq(const T& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const T& input) const{ return input != _value; }
};

template <typename T>
struct in: basic_constrain<in<T>>{
    typedef basic_constrain<in<T>> base;
    
    std::set<T> _values;
    
    template <typename It>
    in(const std::string& message, It begin, It end): base(message){
        std::copy(begin, end, std::inserter(_values, _values.begin()));
    }
    template <typename... U>
    in(const std::string& message, const U&... args): base(message){
        std::initializer_list<T> values{static_cast<T>(args)...};
        std::copy(values.begin(), values.end(), std::inserter(_values, _values.begin()));
    }
    template <typename... U>
    in(const U&... args): base(""){
        std::initializer_list<T> values{static_cast<T>(args)...};
        std::copy(values.begin(), values.end(), std::inserter(_values, _values.begin()));
    }
    bool operator()(const T& input) const{
        return _values.count(input);
    }
};

template <>
struct gte<std::string>: basic_constrain<gte<std::string>>{
    typedef basic_constrain<gte<std::string>> base;
    
    std::size_t _value;
    
    gte(const std::size_t& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const std::string& input) const{ return input.size() >= _value; }
};

template <>
struct gt<std::string>: basic_constrain<gt<std::string>>{
    typedef basic_constrain<gt<std::string>> base;
    
    std::size_t _value;
    
    gt(const std::size_t& value, const std::string& message = ""): base(message), _value(value) {}
    bool operator()(const std::string& input) const{ return input.size() > _value; }
};
template <>
struct lte<std::string>: basic_constrain<lte<std::string>>{
    typedef basic_constrain<lte<std::string>> base;
    
    std::size_t _value;
    
    lte(const std::size_t& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const std::string& input) const{ return input.size() <= _value; }
};
template <>
struct lt<std::string>: basic_constrain<lt<std::string>>{
    typedef basic_constrain<lt<std::string>> base;
    
    std::size_t _value;
    
    lt(const std::size_t& value, const std::string& message = ""): base(message), _value(value){}
    bool operator()(const std::string& input) const{ return input.size() < _value; }
};
    
struct all_digits: basic_constrain<all_digits>{
    typedef basic_constrain<all_digits> base;
    
    all_digits(std::string message = ""): base(message){}
    inline bool operator()(const std::string& value) const{
        for(auto it = value.begin(); it != value.end(); ++it){
            char c = *it;
            if(!std::isdigit(c)){
                return false;
            }
        }
        return true;
    }
};

struct no_space: basic_constrain<no_space>{
    typedef basic_constrain<no_space> base;
    
    no_space(std::string message = ""): base(message){}
    inline bool operator()(const std::string& value) const{
        for(auto it = value.begin(); it != value.end(); ++it){
            char c = *it;
            if(std::isspace(c)){
                return false;
            }
        }
        return true;
    }
};

template <typename T>
using min = gte<T>;
template <typename T>
using max = lte<T>;

typedef gte<std::string>    length_gte;
typedef gt<std::string>     length_gt;
typedef lte<std::string>    length_lte;
typedef lt<std::string>     length_lt;
typedef length_gte          length_min;
typedef length_lte          length_max;

struct length_eq: basic_constrain<length_eq>{
    typedef basic_constrain<length_eq> base;
    
    std::size_t _length;
    
    length_eq(std::size_t length, std::string message = ""): base(message), _length(length){}
    inline bool operator()(const std::string& value) const{
        return value.length() == _length;
    }
};

}

}
}

#endif // UDHO_FORMS_H
