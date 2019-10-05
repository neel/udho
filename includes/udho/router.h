#ifndef ROUTER_H
#define ROUTER_H

#include <deque>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/function.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/mpl/int.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/fusion/functional/invocation/invoke.hpp>
#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>
#include <boost/locale.hpp>
#include "listener.h"
#include "session.h"
#include "util.h"

namespace udho{
    
typedef boost::beast::http::request<boost::beast::http::string_body>    request_type;
typedef boost::beast::http::response<boost::beast::http::string_body>   response_type;
    
class resolver;

namespace internal{
    /**
     * extract the function signature
     */
    template <typename T>
    struct function_signature{};
    
    template <typename R, typename... Args>
    struct function_signature<R (*)(Args...)>{
        typedef R                               return_type;
        typedef boost::tuple<Args...>           tuple_type;
        typedef boost::fusion::tuple<Args...>   arguments_type;
    };
    
    template <typename R, typename... Args>
    struct function_signature<boost::function<R (Args...)> >{
        typedef R                               return_type;
        typedef boost::tuple<Args...>           tuple_type;
        typedef boost::fusion::tuple<Args...>   arguments_type;
    };

    // convert boost::tuple to boost::fusion::tuple
    // https://stackoverflow.com/a/52667660/256007
    template<std::size_t...Is, class T>
    auto to_fusion(std::index_sequence<Is...>, T&& in ) {
        using boost::get;
        using std::get;
        return boost::fusion::make_tuple(get<Is>(std::forward<T>(in))... );
    }
    template<class...Ts>
    auto to_fusion(boost::tuple<Ts...> in ) {
        return to_fusion(std::make_index_sequence<::boost::tuples::length< boost::tuple<Ts...>>::value>{}, std::move(in) );
    }
    template<class...Ts>
    boost::fusion::tuple<Ts...> to_fusion(std::tuple<Ts...> in ) {
        return to_fusion(std::make_index_sequence<sizeof...(Ts)>{}, std::move(in) );
    }
    
    template <typename T, int Index>
    struct cast_optionally{
        static T cast(const std::vector<std::string>& args){
            try{
                return boost::lexical_cast<T>(args[Index]);
            }catch(...){
                return T();
            }
        }
    };
    
    template <typename TupleT, int Index=boost::tuples::length<TupleT>::value-1>
    struct arg_to_tuple: arg_to_tuple<TupleT, Index-1>{
        typedef arg_to_tuple<TupleT, Index-1> base_type;
        
        static void convert(TupleT& tuple, const std::vector<std::string>& args){
            boost::get<Index>(tuple) = cast_optionally<typename boost::tuples::element<Index, TupleT>::type, Index>::cast(args);
            base_type::convert(tuple, args);
        }
    };
    
    template <typename TupleT>
    struct arg_to_tuple<TupleT, 0>{
        static void convert(TupleT& tuple, const std::vector<std::string>& args){
            // boost::get<0>(tuple) = cast_optionally<typename boost::tuples::element<0, TupleT>::type, 0>::cast(args);
        }
    };
    
    template <typename TupleT>
    void arguments_to_tuple(TupleT& tuple, const std::vector<std::string>& args){
        arg_to_tuple<TupleT>::convert(tuple, args);
    }
}

namespace compositors{
    template <typename OutputT>
    struct transparent{
        typedef OutputT response_type;
        
        template <typename ReqT>
        response_type operator()(const ReqT& /*req*/, const OutputT& out){
            return out;
        }
        std::string name() const{
            return "UNSPECIFIED";
        }
    };

    template <typename OutputT>
    struct mimed{
        typedef boost::beast::http::response<boost::beast::http::string_body> response_type;
        std::string _mime;
        
        mimed(const std::string& mime): _mime(mime){}
        template <typename ReqT>
        response_type operator()(const ReqT& req, const OutputT& out){
            std::string content = boost::lexical_cast<std::string>(out);
            response_type res{boost::beast::http::status::ok, req.version()};
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(boost::beast::http::field::content_type,   _mime);
            res.set(boost::beast::http::field::content_length, content.size());
            res.keep_alive(req.keep_alive());
            res.body() = content;
            res.prepare_payload();
            return res;
        }
        std::string name() const{
            return (boost::format("MIMED %1%") % _mime).str();
        }
    };
}

template <typename Function, template <typename> class CompositorT=compositors::transparent>
struct module_overload{    
    typedef Function                                                           function_type;
    typedef module_overload<Function, CompositorT>                             self_type;
    typedef typename internal::function_signature<Function>::return_type       return_type;
    typedef typename internal::function_signature<Function>::tuple_type        tuple_type;
    typedef typename internal::function_signature<Function>::arguments_type    arguments_type;
    typedef CompositorT<return_type>                                           compositor_type;
    typedef typename compositor_type::response_type                            response_type;
    
    boost::beast::http::verb _request_method;
    std::string              _pattern;
    function_type            _function;
    compositor_type          _compositor;
    
    module_overload(boost::beast::http::verb request_method, function_type f, compositor_type compositor=compositor_type()): _request_method(request_method), _function(f), _compositor(compositor){}
    module_overload(const self_type& other): _request_method(other._request_method), _function(other._function), _pattern(other._pattern), _compositor(other._compositor){}

    self_type& operator=(const std::string& pattern){
        _pattern = pattern;
        return *this;
    }
    /**
     * check number of arguments supplied on runtime and number of arguments with which this overload has been prepared at compile time.
     */
    bool feasible(boost::beast::http::verb request_method, const std::string& subject) const{
        if(_pattern.empty()){
            return false;
        }
        std::string subject_decoded = udho::util::urldecode(subject);
        std::cout << "_pattern " << _pattern << " " << " subject " << subject_decoded << std::endl;
        return (request_method == _request_method) && boost::u32regex_search(subject_decoded, boost::make_u32regex(_pattern));
    }
    template <typename T>
    return_type call(T value, const std::vector<std::string>& args){
        std::deque<std::string> argsq;
        std::copy(args.begin(), args.end(), std::back_inserter(argsq));
        tuple_type tuple;
        if(::boost::tuples::length<tuple_type>::value > args.size()){
            argsq.push_front("");
        }
        std::vector<std::string> args_str;
        std::copy(argsq.begin(), argsq.end(), std::back_inserter(args_str));
        internal::arguments_to_tuple(tuple, args_str);
        boost::get<0>(tuple) = value;
        arguments_type arguments = internal::to_fusion(tuple);
        // https://www.boost.org/doc/libs/1_68_0/libs/fusion/doc/html/fusion/functional/invocation/functions/invoke.html
        return boost::fusion::invoke(_function, arguments);
    }
    template <typename T>
    response_type operator()(T value, const std::vector<std::string>& args){
        return_type ret = call(value, args);
        return _compositor(value, ret);
    }
    template <typename T>
    response_type operator()(T value, const std::string& subject){
        std::vector<std::string> args;
        boost::smatch caps;
        try{
            std::string subject_decoded = udho::util::urldecode(subject);
            // std::cout << "subject_decoded: " << subject_decoded << " _pattern: " << _pattern << std::endl;
            if(boost::u32regex_search(subject_decoded, caps, boost::make_u32regex(_pattern))){
                std::copy(caps.begin()+1, caps.end(), std::back_inserter(args));
            }
            // std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, ", "));
            // std::cout << std::endl;
        }catch(std::exception& ex){
            std::cout << "ex: " << ex.what() << std::endl;
        }
        return operator()(value, args);
    }
    module_info info() const{
        module_info inf;
        inf._pattern = _pattern;
        inf._method = _request_method;
        inf._compositor = _compositor.name();
        inf._fptr = &_function;
        return inf;
    }
};

namespace internal{
template <typename C>
struct actual_callback_type{
    typedef C callback_type;
};
template <typename A1, typename R, typename... V>
struct callable1;
template <typename A1, typename R, typename... V>
struct callable1<A1, R (*)(A1&, V...)>{
    typedef typename actual_callback_type<R (*)(A1&, V...)>::callback_type  callback_type;
    typedef boost::function<R (V...)>               function_type;
    typedef R                                       return_type;
    
    callback_type _callback;
    A1& _a1;
    
    callable1(callback_type cb, A1& a1): _callback(cb), _a1(a1){}
    return_type operator()(V... args){
        return _callback(_a1, args...);
    }
};
}

template <typename F, template <typename> class CompositorT=compositors::transparent>
module_overload<F, CompositorT> overload(boost::beast::http::verb request_method, F ftor, CompositorT<typename internal::function_signature<F>::return_type> compositor=CompositorT<typename internal::function_signature<F>::return_type>()){
    return module_overload<F, CompositorT>(request_method, ftor, compositor);
}

template <typename F, typename A1, template <typename> class CompositorT=compositors::transparent>
auto overload(boost::beast::http::verb request_method, F ftor, A1& a1, CompositorT<typename internal::function_signature<F>::return_type> compositor=CompositorT<typename internal::function_signature<F>::return_type>()){
    typedef internal::callable1<A1, F> callable_type;
    typedef typename callable_type::function_type function_type;
    function_type function = callable_type(ftor, a1);
    return overload<function_type>(request_method, function, compositor);
}

/**
 * @brief mapping of an url with a http request defined by http method and the url pattern
 * @details
 * A content wrapper is defined by a HTTP verb, a callback and a url pattern. 
 * A content wrapper uses a compositor that prepares a HTTP response based on the callbacks return
 * @note binds a variable by reference with the mapping, which might be a database connection or some persistent stateful object
 * @tparam F callback type
 * @tparam A1 type of the passed value which will be passed by reference
 */
template <typename F, typename A1>
struct content_wrapper1{
    typedef content_wrapper1<F, A1> self_type;
    
    F  _ftor;
    A1& _a1;
    boost::beast::http::verb _method;
    
    content_wrapper1(boost::beast::http::verb method, F ftor, A1& a1): _method(method), _ftor(ftor), _a1(a1){}    
    template <template <typename> class CompositorT=compositors::transparent>
    auto unwrap(CompositorT<typename internal::function_signature<F>::return_type> compositor = CompositorT<typename internal::function_signature<F>::return_type>()){
        typedef CompositorT<typename internal::function_signature<F>::return_type> compositor_type;
        
        return overload<F, A1, CompositorT>(_method, _ftor, _a1, compositor);
    }
    /**
     * raw content delivery using transparent compositor
     */
    auto raw(){
        return unwrap();
    }
    /**
     * attach an url pattern
     */
    auto operator=(const std::string& pattern){
        auto overloaded = raw();
        overloaded = pattern;
        return overloaded;
    }
    /**
     * applies a mimed compositor on the return
     * @param mime returned mime type
     */
    auto mimed(std::string mime){
        return unwrap(compositors::mimed<typename internal::function_signature<F>::return_type>(mime));
    }
    /**
     * shorthand for html mime type
     */
    auto html(){
        return mimed("text/html");
    }
    /**
     * shorthand for plain text mime type
     */
    auto plain(){
        return mimed("text/plain");
    }
    /**
     * shorthand for json mime type
     */
    auto json(){
        return mimed("application/json");
    }
};

/**
 * @brief mapping of an url with a http request defined by http method and the url pattern
 * @details
 * A content wrapper is defined by a HTTP verb, a callback and a url pattern. 
 * A content wrapper uses a compositor that prepares a HTTP response based on the callbacks return
 * @tparam F callback type
 */
template <typename F>
struct content_wrapper0{
    typedef content_wrapper0<F> self_type;
    
    F  _ftor;
    boost::beast::http::verb _method;
    
    content_wrapper0(boost::beast::http::verb method, F ftor): _method(method), _ftor(ftor){}    
    template <template <typename> class CompositorT=compositors::transparent>
    auto unwrap(CompositorT<typename internal::function_signature<F>::return_type> compositor = CompositorT<typename internal::function_signature<F>::return_type>()){
        typedef CompositorT<typename internal::function_signature<F>::return_type> compositor_type;
        
        return overload<F, CompositorT>(_method, _ftor, compositor);
    }
    /**
     * raw content delivery using transparent compositor
     */
    auto raw(){
        return unwrap();
    }
    /**
     * attach an url pattern
     */
    auto operator=(const std::string& pattern){
        auto overloaded = raw();
        overloaded = pattern;
        return overloaded;
    }
    /**
     * applies a mimed compositor on the return
     * @param mime returned mime type
     */
    auto mimed(std::string mime){
        return unwrap(compositors::mimed<typename internal::function_signature<F>::return_type>(mime));
    }
    /**
     * shorthand for html mime type
     */
    auto html(){
        return mimed("text/html");
    }
    /**
     * shorthand for plain text mime type
     */
    auto plain(){
        return mimed("text/plain");
    }
    /**
     * shorthand for json mime type
     */
    auto json(){
        return mimed("application/json");
    }
};

/**
 * creates a get mapping
 * @see content_wrapper0
 */
template <typename F>
auto get(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::get, ftor);
}

/**
 * creates a get mapping
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto get(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::get, ftor, a1);
}

/**
 * creates a post mapping
 * @see content_wrapper0
 */
template <typename F>
auto post(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::post, ftor);
}

/**
 * creates a post mapping
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto post(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::post, ftor, a1);
}

/**
 * creates a put mapping
 * @see content_wrapper0
 */
template <typename F>
auto put(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::put, ftor);
}

/**
 * creates a put mapping
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto put(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::put, ftor, a1);
}

/**
 * creates a head mapping
 * @see content_wrapper0
 */
template <typename F>
auto head(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::head, ftor);
}

/**
 * creates a head mapping
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto head(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::head, ftor, a1);
}

/**
 * creates a delete mapping
 * @see content_wrapper0
 */
template <typename F>
auto del(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::delete_, ftor);
}

/**
 * creates a delete mapping
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto del(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::delete_, ftor, a1);
}

/**
 * compile time chain of url mappings 
 * @see content_wrapper0
 * @see content_wrapper1
 */
template <typename U, typename V = void>
struct overload_group{
    typedef overload_group<U, V> self_type;
    typedef U            parent_type;
    typedef V            overload_type;
    
    parent_type   _parent;
    overload_type _overload;
    
    overload_group(const parent_type& parent, const overload_type& overload): _parent(parent), _overload(overload){}
    template <typename ReqT, typename Lambda>
    http::status serve(ReqT req, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        std::cout << "serve: " << subject << std::endl;
        if(_overload.feasible(request_method, subject)){
            typename overload_type::response_type res;
            try{
                res = _overload(req, subject);
                send(std::move(res));
            }catch(const udho::exceptions::http_error& error){
                send(std::move(error.response(req)));
                return error.result();
            }catch(const std::exception& ex){
                std::cout << ex.what() << std::endl;
            }
            return res.result();
        }else{
            return _parent.template serve<ReqT, Lambda>(req, request_method, subject, send);
        }
    }
    void summary(std::vector<module_info>& stack){
        stack.push_back(_overload.info());
        _parent.summary(stack);
    }
    self_type& listen(boost::asio::io_service& io, int port=9198, std::string doc_root=""){
        typedef udho::listener<self_type> listener_type;
        std::make_shared<listener_type>(*this, io, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), port), std::make_shared<std::string>(doc_root))->run();
        return *this;
    }
};

/**
 * terminal node of the compile time chain of url mappings 
 * @see content_wrapper0
 * @see content_wrapper1
 */
template <typename U>
struct overload_group<U, void>{
    typedef overload_group<U> self_type;
    typedef void              parent_type;
    typedef U                 overload_type;
    
    overload_type _overload;
    
    overload_group(const overload_type& overload): _overload(overload){}
    template <typename ReqT, typename Lambda>
    http::status serve(ReqT req, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        std::cout << "serve <void>: " << subject << std::endl;
        if(_overload.feasible(request_method, subject)){
            typename overload_type::response_type res;
            try{
                res = _overload(req, subject);
                send(std::move(res));
            }catch(const udho::exceptions::http_error& error){
                send(std::move(error.response(req)));
                return error.result();
            }catch(const std::exception& ex){
                std::cout << ex.what() << std::endl;
            }
            return res.result();
        }
        return http::status::unknown;
    }
    void summary(std::vector<module_info>& stack){
        stack.push_back(_overload.info());
    }
};

udho::response_type failure_callback(udho::request_type req);

/**
 * router maps HTTP requests with the callbacks
 * @code
 * auto router = udho::router()
 *      | (udho::get(add).plain()   = "^/add/(\\d+)/(\\d+)$")
 *      | (udho::get(hello).plain() = "^/hello$");
 * @endcode
 * @example example/simple.cpp
 */
struct router: public overload_group<module_overload<udho::response_type (*)(udho::request_type)>, void>{
    router(): overload_group<module_overload<udho::response_type (*)(udho::request_type)>, void>(udho::overload(boost::beast::http::verb::unknown, &failure_callback)){}
};

/**
 * adds a callback url mapping to the router
 * @param group the overload group (which is actually the router or router attached with some url mappings)
 * @param method url mapping
 */
template <typename U, typename V, typename F>
overload_group<overload_group<U, V>, F> operator|(const overload_group<U, V>& group, const F& method){
    return overload_group<overload_group<U, V>, F>(group, method);
}


}

#endif // ROUTER_H
