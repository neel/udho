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
#include <udho/util.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>

namespace udho{
namespace forms{
    
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
    
template <typename T>
struct parser{
    template <typename U>
    static bool parsable(const U& input){
        return deserializer<T, U>::check(input);
    }
    template <typename U>
    static T parse(const U& input){
        return deserializer<T, U>::deserialize(input);
    }
};
    
namespace drivers{    
    /**
     * Form driver for urlencoded forms
     */
    struct urlencoded{
        typedef std::string::const_iterator iterator_type;
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
        inline bool empty(const std::string name) const{
            auto it = _fields.find(name);
            if(it != _fields.end()){
                return it->second.size() == 0;
            }
            return true;
        }
        
        /**
        * checks whether there exists any field with the name provided
        */
        inline bool exists(const std::string name) const{
            return _fields.find(name) != _fields.end();
        }
        
        template <typename T, typename ParserT = udho::forms::parser<T>>
        bool parsable(const std::string& name) const{
            auto it = _fields.find(name);
            if(it != _fields.end()){
                std::string raw = it->second.copied<std::string>();
                return ParserT::parsable(raw);
            }
            return false;
        }
        
        /**
         * returns the value of the field with the name provided lexically casted to type T
         */
        template <typename T, typename ParserT = udho::forms::parser<T>>
        const T parsed(const std::string& name) const{
            auto it = _fields.find(name);
            if(it != _fields.end()){
                std::string raw = it->second.copied<std::string>();
                return ParserT::parse(raw);
            }
            return T();
        }
    };
    
    /**
    * Form accessor for multipart forms
    */
    struct multipart_form{
        typedef std::string::const_iterator iterator_type;
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
            
            template <typename T, typename ParserT = udho::forms::parser<T>>
            bool parsable() const{
                return ParserT::parsable(str());
            }
            
            /**
            * returns the value of the field with the name provided lexically casted to type T
            */
            template <typename T, typename ParserT = udho::forms::parser<T>>
            const T parsed() const{
                if(parsable<T, ParserT>()){
                    return ParserT::parse(str());
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
        /**
        * returns the part associated with the given name
        */
        const form_part& part(const std::string& name) const{
            return _parts.at(name);
        }
        
        template <typename T, typename ParserT = udho::forms::parser<T>>
        bool parsable(const std::string& name) const{
            if(exists(name)){
                return part(name).template parsable<T, ParserT>();
            }
            return false;
        }
        
        /**
        * returns the value of the field with the name provided lexically casted to type T
        */
        template <typename T, typename ParserT = udho::forms::parser<T>>
        const T parsed(const std::string& name) const{
            if(exists(name)){
                return part(name).template parsed<T, ParserT>();
            }
            return T();
        }
    };

};

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
};

template <typename T, bool Required = false>
struct field;

namespace detail{
    template <typename ValidatorT, typename FieldT>
    struct constrained_field{
        typedef FieldT field_type;
        typedef typename field_type::value_type value_type;
        typedef ValidatorT validator_type;
        typedef constrained_field<validator_type, field_type> self_type;
        
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
        
        template <typename ValidatorU>
        detail::constrained_field<ValidatorU, self_type> constrain(const ValidatorU& validator){
            return detail::constrained_field<ValidatorU, self_type>(validator, *this);
        }
        
        template <template<typename> class ValidatorU, typename... ArgsT>
        detail::constrained_field<ValidatorU<value_type>, self_type> constrain(ArgsT&&... args){
            return detail::constrained_field<ValidatorU<value_type>, self_type>(ValidatorU<value_type>(args...), *this);
        }
    };
    
    template <typename ValidatorT, typename... T>
    struct constrained_field<ValidatorT, constrained_field<T...>>{
        typedef constrained_field<T...> head_type;
        typedef typename head_type::field_type field_type;
        typedef typename field_type::value_type value_type;
        typedef ValidatorT validator_type;
        typedef constrained_field<validator_type, field_type> self_type;
        
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
        
        template <typename ValidatorU>
        detail::constrained_field<ValidatorU, self_type> constrain(const ValidatorU& validator){
            return detail::constrained_field<ValidatorU, self_type>(validator, *this);
        }
        
        template <template<typename> class ValidatorU, typename... ArgsT>
        detail::constrained_field<ValidatorU<value_type>, self_type> constrain(ArgsT&&... args){
            return detail::constrained_field<ValidatorU<value_type>, self_type>(ValidatorU<value_type>(args...), *this);
        }
    };
}

template <typename T>
struct field<T, true>{
    typedef T value_type;
    typedef field<T, true> self_type;
    enum { is_required = true };
    
    field(const std::string& name): _name(name), _absent(true), _unparsable(true), _fetched(false), _valid(true){}
    self_type& absent(const std::string& message) { _message_absent = message; return *this; }
    self_type& unparsable(const std::string& message) { _message_unparsable = message; return *this; }
    
    template <typename FormT>
    bool fetch(const FormT& form){
        if(_fetched) return valid();
        _absent  = !form.has(_name);
        if(!_absent){
            bool okay = true;
            _value = form.template field<value_type>(_name, &okay);
            _unparsable = !okay;
            if(_unparsable){
                _message = _message_unparsable.empty() ? _message_absent : _message_unparsable;
            }
        }else{
            _message = _message_absent;
        }
        _valid = !_absent && !_unparsable;
        _fetched = true;
        return valid();
    }
    value_type value() const {
        return _value;
    }
    inline value_type operator*() const { return value(); }
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        if(valid()){
            _valid = _valid && validator(value());
            if(!_valid){
                _message = validator.message();
            }
            return _valid;
        }
        return false;
    }
    bool valid() const{
        return _valid;
    }
    std::string message() const {
        return _message;
    }
    
    template <typename ValidatorT>
    detail::constrained_field<ValidatorT, self_type> constrain(const ValidatorT& validator){
        return detail::constrained_field<ValidatorT, self_type>(validator, *this);
    }
    
    template <template<typename> class ValidatorT, typename... ArgsT>
    detail::constrained_field<ValidatorT<value_type>, self_type> constrain(ArgsT&&... args){
        return detail::constrained_field<ValidatorT<value_type>, self_type>(ValidatorT<value_type>(args...), *this);
    }
    
    std::string _name;
    std::string _message_absent;
    std::string _message_unparsable;
    value_type  _value;
    bool _absent;
    bool _unparsable;
    bool _fetched;
    bool _valid;
    std::string _message;
};

template <typename T>
struct field<T, false>{
    typedef T value_type;
    typedef field<T, false> self_type;
    enum { is_required = false };
    
    field(const std::string& name): _name(name), _absent(true), _unparsable(true), _valid(true), _fetched(false){}
    self_type& absent(const std::string& message) { _message_absent = message; return *this; }
    self_type& unparsable(const std::string& message) { _message_unparsable = message; return *this; }
    
    template <typename FormT>
    bool fetch(const FormT& form){
        if(!_fetched) return valid();
        _absent  = !form.has(_name);
        if(!_absent){
            bool okay = true;
            _value = form.template field<value_type>(_name, &okay);
            _unparsable = !okay;
            _valid = _unparsable;
            if(_unparsable){
                _message = _message_unparsable.empty() ? _message_absent : _message_unparsable;
            }
        }else{
            _valid = true;
        }
        _fetched = true;
        return valid();
    }
    value_type value() const {
        return _value;
    }
    template <typename FormT, typename ValidatorT>
    bool validate(const FormT& form, const ValidatorT& validator){
        fetch(form);
        if(valid()){
            bool valid = validator(value());
            if(!valid){
                _message = validator.message();
            }
            return valid;
        }
        return false;
    }
    bool valid() const{
        return _valid;
    }
    std::string message() const {
        return _message;
    }
    
    template <typename ValidatorT>
    detail::constrained_field<ValidatorT, self_type> constrain(const ValidatorT& validator){
        return detail::constrained_field<ValidatorT, self_type>(validator, *this);
    }
    
    template <template<typename> class ValidatorT, typename... ArgsT>
    detail::constrained_field<ValidatorT<value_type>, self_type> constrain(ArgsT&&... args){
        return detail::constrained_field<ValidatorT<value_type>, self_type>(ValidatorT<value_type>(args...), *this);
    }
    
    std::string _name;
    std::string _message_absent;
    std::string _message_unparsable;
    value_type  _value;
    bool _fetched;
    bool _absent;
    bool _unparsable;
    bool _valid;
    std::string _message;
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

namespace constraints{
    
template <typename T>
struct gte{
    T _value;
    std::string _message;
    
    gte(const T& value, const std::string& message): _value(value), _message(message){}
    const std::string& message() const { return _message; }
    bool operator()(const T& input) const{
        return input >= _value;
    }
};
template <typename T>
struct gt{
    T _value;
    std::string _message;
    
    gt(const T& value, const std::string& message): _value(value), _message(message){}
    const std::string& message() const { return _message; }
    bool operator()(const T& input) const{
        return input > _value;
    }
};
template <typename T>
struct lte{
    T _value;
    std::string _message;
    
    lte(const T& value, const std::string& message): _value(value), _message(message){}
    const std::string& message() const { return _message; }
    bool operator()(const T& input) const{
        return input <= _value;
    }
};
template <typename T>
struct lt{
    T _value;
    std::string _message;
    
    lt(const T& value, const std::string& message): _value(value), _message(message){}
    const std::string& message() const { return _message; }
    bool operator()(const T& input) const{
        return input < _value;
    }
};

template <>
struct gte<std::string>{
    std::size_t _value;
    std::string _message;
    
    gte(const std::size_t& value, const std::string& message): _value(value), _message(message){}
    const std::string& message() const { return _message; }
    bool operator()(const std::string& input) const{
        return input.size() >= _value;
    }
};
template <>
struct gt<std::string>{
    std::size_t _value;
    std::string _message;
    
    gt(const std::size_t& value, const std::string& message): _value(value), _message(message){}
    const std::string& message() const { return _message; }
    bool operator()(const std::string& input) const{
        return input.size() > _value;
    }
};
template <>
struct lte<std::string>{
    std::size_t _value;
    std::string _message;
    
    lte(const std::size_t& value, const std::string& message): _value(value), _message(message){}
    const std::string& message() const { return _message; }
    bool operator()(const std::string& input) const{
        return input.size() <= _value;
    }
};
template <>
struct lt<std::string>{
    std::size_t _value;
    std::string _message;
    
    lt(const std::size_t& value, const std::string& message): _value(value), _message(message){}
    const std::string& message() const { return _message; }
    bool operator()(const std::string& input) const{
        return input.size() < _value;
    }
};
    
}

}
}

#endif // UDHO_FORMS_H
