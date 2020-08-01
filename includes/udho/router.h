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
#include <boost/locale.hpp>
#include <iomanip>
#include <udho/contexts.h>
#include <udho/compositors.h>
#include <udho/listener.h>
#include <udho/connection.h>
#include "util.h"

#ifdef WITH_ICU
#include <boost/regex/icu.hpp>
#endif

#define ROUTING_DEFERRED -102

namespace udho{
    
class resolver;

namespace internal{
    /**
     * extract the function signature
     */
    template <typename T>
    struct function_signature: function_signature<typename T::function_type>{};
    
    template <typename R, typename... Args>
    struct function_signature<R (*)(Args...)>{
        typedef R                               return_type;
        typedef boost::tuple<std::decay_t<Args>...>           tuple_type;
        typedef boost::fusion::tuple<std::decay_t<Args>...>   arguments_type;
    };
    
    template <typename R, typename... Args>
    struct function_signature<boost::function<R (Args...)> >{
        typedef R                               return_type;
        typedef boost::tuple<std::decay_t<Args>...>           tuple_type;
        typedef boost::fusion::tuple<std::decay_t<Args>...>   arguments_type;
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
        static void convert(TupleT& /*tuple*/, const std::vector<std::string>& /*args*/){
            // boost::get<0>(tuple) = cast_optionally<typename boost::tuples::element<0, TupleT>::type, 0>::cast(args);
        }
    };
    
    template <typename TupleT>
    void arguments_to_tuple(TupleT& tuple, const std::vector<std::string>& args){
        arg_to_tuple<TupleT>::convert(tuple, args);
    }
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
    module_overload(const self_type& other): _request_method(other._request_method), _pattern(other._pattern), _function(other._function), _compositor(other._compositor){}

    std::string pattern() const{
        return _pattern;
    }
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
        // std::cout << "_pattern " << _pattern << " " << " subject " << subject_decoded << std::endl;
#ifdef WITH_ICU
        return (request_method == _request_method) && boost::u32regex_search(subject_decoded, boost::make_u32regex(_pattern));
#else
        return (request_method == _request_method) && boost::regex_search(subject_decoded, boost::regex(_pattern));
#endif
    }
    template <typename T>
    return_type call(T& value, const std::vector<std::string>& args){
        std::deque<std::string> argsq;
        std::copy(args.begin(), args.end(), std::back_inserter(argsq));
        tuple_type tuple(value);
        if(::boost::tuples::length<tuple_type>::value > args.size()){
            argsq.push_front("");
        }
        std::vector<std::string> args_str;
        std::copy(argsq.begin(), argsq.end(), std::back_inserter(args_str));
        internal::arguments_to_tuple(tuple, args_str);
//         boost::get<0>(tuple) = value;
        arguments_type arguments = internal::to_fusion(tuple);
        // https://www.boost.org/doc/libs/1_68_0/libs/fusion/doc/html/fusion/functional/invocation/functions/invoke.html
        return boost::fusion::invoke(_function, arguments);
    }
    template <typename T>
    response_type operator()(T& value, const std::vector<std::string>& args){
        return_type ret = call(value, args);
        return _compositor(value, std::move(ret));
    }
    template <typename T>
    response_type operator()(T& value, const std::string& subject){
        std::vector<std::string> args;
        boost::smatch caps;
        try{
            std::string subject_decoded = udho::util::urldecode(subject);
            // std::cout << "subject_decoded: " << subject_decoded << " _pattern: " << _pattern << std::endl;
#ifdef WITH_ICU
            if(boost::u32regex_search(subject_decoded, caps, boost::make_u32regex(_pattern))){
#else
            if(boost::regex_search(subject_decoded, caps, boost::regex(_pattern))){
#endif
                std::copy(caps.begin()+1, caps.end(), std::back_inserter(args));
            }
            // std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, ", "));
            // std::cout << std::endl;
        }catch(const udho::exceptions::reroute&){
            throw;
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
    private:
        self_type& operator=(const self_type& other){
            _request_method = other._request_method;
            _function = other._function;
            _pattern = other._pattern;
            _compositor = other._compositor;
            return *this;
        }
};

template <typename Function>
struct module_overload<Function, compositors::deferred>{    
    typedef Function                                                           function_type;
    typedef module_overload<Function, compositors::deferred>                   self_type;
    typedef typename internal::function_signature<Function>::return_type       return_type;
    typedef typename internal::function_signature<Function>::tuple_type        tuple_type;
    typedef typename internal::function_signature<Function>::arguments_type    arguments_type;
    typedef compositors::deferred<return_type>                                 compositor_type;
    typedef typename compositor_type::response_type                            response_type;
    
    boost::beast::http::verb _request_method;
    std::string              _pattern;
    function_type            _function;
    compositor_type          _compositor;
    
    module_overload(boost::beast::http::verb request_method, function_type f, compositor_type compositor=compositor_type()): _request_method(request_method), _function(f), _compositor(compositor){}
    module_overload(const self_type& other): _request_method(other._request_method), _pattern(other._pattern), _function(other._function), _compositor(other._compositor){}

    std::string pattern() const{
        return _pattern;
    }
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
        // std::cout << "_pattern " << _pattern << " " << " subject " << subject_decoded << std::endl;
#ifdef WITH_ICU
        return (request_method == _request_method) && boost::u32regex_search(subject_decoded, boost::make_u32regex(_pattern));
#else
        return (request_method == _request_method) && boost::regex_search(subject_decoded, boost::regex(_pattern));
#endif
    }
    template <typename T>
    void call(T& value, const std::vector<std::string>& args){
        std::deque<std::string> argsq;
        std::copy(args.begin(), args.end(), std::back_inserter(argsq));
        tuple_type tuple(value);
        if(::boost::tuples::length<tuple_type>::value > args.size()){
            argsq.push_front("");
        }
        std::vector<std::string> args_str;
        std::copy(argsq.begin(), argsq.end(), std::back_inserter(args_str));
        internal::arguments_to_tuple(tuple, args_str);
//         boost::get<0>(tuple) = value;
        arguments_type arguments = internal::to_fusion(tuple);
        // https://www.boost.org/doc/libs/1_68_0/libs/fusion/doc/html/fusion/functional/invocation/functions/invoke.html
        boost::fusion::invoke(_function, arguments);
    }
    template <typename T>
    void operator()(T& value, const std::vector<std::string>& args){
        call(value, args);
        _compositor();
    }
    template <typename T>
    void operator()(T& value, const std::string& subject){
        std::vector<std::string> args;
        boost::smatch caps;
        try{
            std::string subject_decoded = udho::util::urldecode(subject);
            // std::cout << "subject_decoded: " << subject_decoded << " _pattern: " << _pattern << std::endl;
#ifdef WITH_ICU
            if(boost::u32regex_search(subject_decoded, caps, boost::make_u32regex(_pattern))){
#else
            if(boost::regex_search(subject_decoded, caps, boost::regex(_pattern))){
#endif
                std::copy(caps.begin()+1, caps.end(), std::back_inserter(args));
            }
            // std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, ", "));
            // std::cout << std::endl;
        }catch(const udho::exceptions::reroute&){
            throw;
        }catch(std::exception& ex){
            std::cout << "ex: " << ex.what() << std::endl;
        }
        operator()(value, args);
    }
    module_info info() const{
        module_info inf;
        inf._pattern = _pattern;
        inf._method = _request_method;
        inf._compositor = _compositor.name();
        inf._fptr = &_function;
        return inf;
    }
    private:
        self_type& operator=(const self_type& other){
            _request_method = other._request_method;
            _function = other._function;
            _pattern = other._pattern;
            _compositor = other._compositor;
            return *this;
        }
};

template <typename Function, template <typename> class CompositorT, int N>
module_overload<Function, CompositorT> operator<<(module_overload<Function, CompositorT> overload, const char (&expr)[N]){
    overload = expr;
    return overload;
}

template <typename Function, template <typename> class CompositorT, int N>
module_overload<Function, CompositorT> operator>>(const char (&expr)[N], module_overload<Function, CompositorT> overload){
    overload = expr;
    return overload;
}

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
 * \defgroup routing
 * conjugating the routing table with multiple url mapped callbacks
 */

/**
 * \defgroup content content
 * content wrappers attach HTTP request feasibility criteria with a callback
 * @ingroup routing
 */

/**
 * @brief mapping of an url with a http request defined by http method and the url pattern
 * @details
 * A content wrapper is defined by a HTTP verb, a callback and a url pattern. 
 * A content wrapper uses a compositor that prepares a HTTP response based on the callbacks return
 * @note binds a variable by reference with the mapping, which might be a database connection or some persistent stateful object
 * @tparam F callback type
 * @tparam A1 type of the passed value which will be passed by reference
 * @ingroup content
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
        // typedef CompositorT<typename internal::function_signature<F>::return_type> compositor_type;
        
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
 * @ingroup content
 */
template <typename F>
struct content_wrapper0{
    typedef content_wrapper0<F> self_type;
    
    F  _ftor;
    boost::beast::http::verb _method;
    
    content_wrapper0(boost::beast::http::verb method, F ftor): _ftor(ftor), _method(method){}    
    template <template <typename> class CompositorT=compositors::transparent>
    auto unwrap(CompositorT<typename internal::function_signature<F>::return_type> compositor = CompositorT<typename internal::function_signature<F>::return_type>()){
        return overload<F, CompositorT>(_method, _ftor, compositor);
    }
    auto deferred(){
        compositors::deferred<typename internal::function_signature<F>::return_type> compositor = compositors::deferred<typename internal::function_signature<F>::return_type>();
        return overload<F, compositors::deferred>(_method, _ftor, compositor);
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
 * \defgroup shorthand shorthands
 * conveniance functions to instanciate a content wrapper
 * @ingroup content
 */

/**
 * creates a get mapping
 * @ingroup shorthand
 * @see content_wrapper0
 */
template <typename F>
auto get(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::get, ftor);
}

/**
 * creates a get mapping
 * @ingroup shorthand
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto get(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::get, ftor, a1);
}

/**
 * creates a post mapping
 * @ingroup shorthand
 * @see content_wrapper0
 */
template <typename F>
auto post(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::post, ftor);
}

/**
 * creates a post mapping
 * @ingroup shorthand
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto post(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::post, ftor, a1);
}

/**
 * creates a put mapping
 * @ingroup shorthand
 * @see content_wrapper0
 */
template <typename F>
auto put(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::put, ftor);
}

/**
 * creates a put mapping
 * @ingroup shorthand
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto put(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::put, ftor, a1);
}

/**
 * creates a head mapping
 * @ingroup shorthand
 * @see content_wrapper0
 */
template <typename F>
auto head(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::head, ftor);
}

/**
 * creates a head mapping
 * @ingroup shorthand
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto head(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::head, ftor, a1);
}

/**
 * creates a delete mapping
 * @ingroup shorthand
 * @see content_wrapper0
 */
template <typename F>
auto del(F ftor){
    return content_wrapper0<F>(boost::beast::http::verb::delete_, ftor);
}

/**
 * creates a delete mapping
 * @ingroup shorthand
 * @see content_wrapper1
 */
template <typename F, typename A1>
auto del(F ftor, A1& a1){
    return content_wrapper1<F, A1>(boost::beast::http::verb::delete_, ftor, a1);
}

template <typename AuxT = void>
struct overload_terminal: AuxT{
    typedef AuxT aux_type;
        
    using aux_type::aux_type;
};

template <>
struct overload_terminal<void>{
    typedef void aux_type;
    
    template <typename... Args>
    overload_terminal(Args...){}
};

template <typename OverloadT, typename ResponseT = typename OverloadT::response_type>
struct overload_group_helper{
    typedef OverloadT overload_type;
    typedef typename overload_type::response_type response_type;
    
    OverloadT& _overload;
    
    overload_group_helper(OverloadT& overload): _overload(overload){}
    template <typename ContextT, typename Lambda>
    int resolve(ContextT& ctx, Lambda send, const std::string& subject){
        response_type res = _overload(ctx, subject);
        http::status status = res.result();
        ctx.patch(res);
        if(ctx.alt_path().empty()){
            send(std::move(res));
            return static_cast<int>(status);
        }else{
            return ROUTING_DEFERRED;
        }
    }
};

template <typename OverloadT>
struct overload_group_helper<OverloadT, void>{
    typedef OverloadT overload_type;
    typedef void response_type;
    
    OverloadT& _overload;
    
    overload_group_helper(OverloadT& overload): _overload(overload){}
    template <typename ContextT, typename Lambda>
    int resolve(ContextT& ctx, Lambda send, const std::string& subject){
        _overload(ctx, subject);
        return ROUTING_DEFERRED;
    }
};

/**
 * compile time chain of url mappings 
 * @see content_wrapper0
 * @see content_wrapper1
 * @ingroup routing
 */
template <typename U, typename V>
struct overload_group{
    typedef overload_group<U, V> self_type; ///< type of this overload
    typedef U            parent_type;       ///< type of the parient in the meta-chain of overloads
    typedef V            overload_type;     ///< type of the next child in the overload chain
    typedef typename parent_type::terminal_type terminal_type;
    
    parent_type   _parent;
    overload_type _overload;
    
    overload_group(const parent_type& parent, const overload_type& overload): _parent(parent), _overload(overload){}

    /**
     * serves the content if the http request matches with the content's request method and path.
     * Calls the callback in `_overload` if it is feasible for the current request. 
     * Otherwise bubbles the request meta-recursively if some other overload group is feasible.
     * 
     * @param req the http request to serve
     * @param request_method http vmethod get post put head etc ...
     * @param subject http resource path 
     * @param send the write callback
     */
    template <typename ContextT, typename Lambda>
    int serve(ContextT& ctx, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        int status = 0;
        if(_overload.feasible(request_method, subject)){
            try{
                ctx.push(udho::detail::route{ctx.path(), subject, _overload.pattern()});
                overload_group_helper<overload_type> helper(_overload);
                status = helper.resolve(ctx, send, subject);
            }catch(const udho::exceptions::http_error& error){
                send(std::move(error.response(ctx.request())));
                return static_cast<int>(error.result());
            }catch(const udho::exceptions::reroute&){
                throw;
            }catch(const std::exception& ex){
                std::cout << ex.what() << std::endl;
                ctx << udho::logging::messages::formatted::error("router", "unhandled exception %1% while serving %2% using method %3%") % ex.what() % subject % request_method;
                udho::exceptions::http_error error(boost::beast::http::status::internal_server_error, (boost::format("unhandled exception %1% while serving %2% using method %3%") % ex.what() % subject % request_method).str());
                send(std::move(error.response(ctx.request())));
                return static_cast<int>(error.result());
            }catch(...){
                udho::exceptions::http_error error(boost::beast::http::status::internal_server_error);
                send(std::move(error.response(ctx.request())));
                return static_cast<int>(error.result());
            }
            return status;
        }else{
            return _parent.template serve<ContextT, Lambda>(ctx, request_method, subject, send);
        }
    }
    void summary(std::vector<module_info>& stack) const{
        stack.push_back(_overload.info());
        _parent.summary(stack);
    }
    template <typename AttachmentT>
    self_type& listen(boost::asio::io_service& io, AttachmentT& attachment, int port=9198){
        typedef udho::listener<self_type, AttachmentT> listener_type;
        std::make_shared<listener_type>(*this, io, attachment, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("0.0.0.0"), port))->run();
        return *this;
    }
    template <typename F>
    void eval(F& fnc){
        fnc(_overload);
        _parent.eval(fnc);
    }
    const terminal_type& terminal() const{
        return _parent.terminal();
    }
};

/**
 * terminal node of the compile time chain of url mappings 
 * @see content_wrapper0
 * @see content_wrapper1
 * @ingroup routing
 */
template <typename U, typename V>
struct overload_group<U, overload_terminal<V>>{
    typedef overload_group<U, overload_terminal<V>> self_type;        ///< type of this overload
    typedef V                 overload_type;      ///< type of parent in the overoad meta-chain
    typedef U                 parent_type;    ///< type of the next child in the overload chain
    typedef overload_terminal<V> terminal_type;
    
    terminal_type _terminal;
    
    template <typename... Args>
    overload_group(Args... args): _terminal(args...){}
    template <typename ContextT, typename Lambda>
    int serve(ContextT& ctx, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        int status = 0;
        if(_terminal.feasible(request_method, subject)){
            try{
                ctx.push(udho::detail::route{ctx.path(), subject, _terminal.pattern()});
                overload_group_helper<terminal_type> helper(_terminal);
                status = helper.resolve(ctx, send, subject);
            }catch(const udho::exceptions::http_error& error){
                send(std::move(error.response(ctx.request())));
                return static_cast<int>(error.result());
            }catch(const udho::exceptions::reroute&){
                throw;
            }catch(const std::exception& ex){
                std::cout << ex.what() << std::endl;
                ctx << udho::logging::messages::formatted::error("router", "unhandled exception %1% while serving %2% using method %3%") % ex.what() % subject % request_method;
                udho::exceptions::http_error error(boost::beast::http::status::internal_server_error, (boost::format("unhandled exception %1% while serving %2% using method %3%") % ex.what() % subject % request_method).str());
                send(std::move(error.response(ctx.request())));
                return static_cast<int>(error.result());
            }catch(...){
                udho::exceptions::http_error error(boost::beast::http::status::internal_server_error, "Server encountered an unknown error");
                send(std::move(error.response(ctx.request())));
                return static_cast<int>(error.result());
            }
            return status;
        }else{
            return 0;
        }
    }
    void summary(std::vector<module_info>& /*stack*/) const{}
    template <typename F>
    void eval(F& fnc){
        fnc(_terminal);
    }
    const terminal_type& terminal() const{
        return _terminal;
    }
};

template <typename U>
struct overload_group<U, overload_terminal<void>>{
    typedef overload_group<U, overload_terminal<void>> self_type;        ///< type of this overload
    typedef void              overload_type;      ///< type of parent in the overoad meta-chain
    typedef U                 parent_type;    ///< type of the next child in the overload chain
    typedef overload_terminal<void> terminal_type;
    
    terminal_type _terminal;
    
    template <typename... Args>
    overload_group(Args...){}
    template <typename ContextT, typename Lambda>
    int serve(ContextT& /*ctx*/, boost::beast::http::verb /*request_method*/, const std::string& /*subject*/, Lambda /*send*/){
        return 0;
    }
    void summary(std::vector<module_info>& /*stack*/) const{}
    template <typename F>
    void eval(F& /*fnc*/){}
    const terminal_type& terminal() const{
        return _terminal;
    }
};

/**
 * router maps HTTP requests with the callbacks
 * @code
 * auto router = udho::router()
 *      | (udho::get(add).plain()   = "^/add/(\\d+)/(\\d+)$")
 *      | (udho::get(hello).plain() = "^/hello$");
 * @endcode
 * @ingroup routing
 */
template <typename AuxT=void>
struct basic_router: public overload_group<void, overload_terminal<AuxT>>{
    typedef overload_group<void, overload_terminal<AuxT>> base_type;
    /**
     * cnstructs a router
     */
    template <typename... Args>
    basic_router(Args... args): base_type(args...){}
    
    template <typename U, typename V, typename F>
    friend overload_group<overload_group<U, V>, F> operator|(const overload_group<U, V>& group, const F& method);
};

template <typename U, typename V, typename F>
udho::overload_group<U, V>& operator/=(udho::overload_group<U, V>& group, F fixture){
    group.eval(fixture);
    return group;
}

typedef basic_router<> router;

/**
 * adds a callback url mapping to the router by conjugating an overload in the overload meta-chain.
 * The url mapping is provided by content wrappers like \ref content_wrapper0 \ref content_wrapper1 
 * which attaches feasibility criteria like http resource path, http verb with a callback function. 
 * The url mapping can be built using \ref get \ref post \ref put \ref head etc.. See \ref router for
 * an example
 * 
 * @param group the overload group (which is actually the router or router attached with some url mappings)
 * @param method url mapping
 * @ingroup routing
 */
template <typename U, typename V, typename F>
overload_group<overload_group<U, V>, F> operator|(const overload_group<U, V>& group, const F& method){
    return overload_group<overload_group<U, V>, F>(group, method);
}

template <typename U, typename V, typename F>
overload_group<overload_group<U, V>, F> operator<<(const overload_group<U, V>& group, const F& method){
    return overload_group<overload_group<U, V>, F>(group, method);
}

struct reroute{
    typedef boost::function<boost::beast::http::response<boost::beast::http::string_body> (udho::contexts::stateless)> function_type;
    
    std::string _alt_path;
    
    explicit inline reroute(const std::string& alt_path): _alt_path(alt_path){}
    inline std::string alt_path() const{
        return _alt_path;
    }
    inline boost::beast::http::response<boost::beast::http::string_body> operator()(udho::contexts::stateless ctx){
        ctx.reroute(_alt_path);
        
        boost::beast::http::response<boost::beast::http::string_body> response{boost::beast::http::status::ok, ctx.request().version()};
        return response;
    }
};

template <typename StreamT, typename U, typename V>
StreamT& operator<<(StreamT& stream, const overload_group<U, V>& router){
    std::vector<udho::module_info> summary;
    router.summary(summary);
    int indent = 0;
    
    std::function<void (const std::vector<udho::module_info>&)> printer;
    printer = [&stream, &indent, &printer](const std::vector<udho::module_info>& infos){
        for(auto i = infos.rbegin(); i != infos.rend(); ++i){
            auto info = *i;
            std::string method_str;
            
            if(info._compositor == "UNSPECIFIED"){
                continue;
            }
            
            for(int j = 0; j < indent; ++j){
                std::cout << "\t";
            }
            stream << boost::format("%1% %2% -> %3% (%4%)") % info._method % info._pattern % info._fptr % info._compositor << std::endl;
            if(info._children.size() > 0){
                ++indent;
                printer(info._children);
                --indent;
            }
        }
    };
    printer(summary);
    return stream;
}

}

#endif // ROUTER_H

