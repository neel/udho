/*
 * Copyright 2019 <copyright holder> <email>
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include "util.h"
#include <string>
#include <functional>
#include <boost/regex.hpp>
#include <udho/router.h>
#include <udho/util.h>

#ifdef WITH_ICU
#include <boost/regex/icu.hpp>
#endif

namespace udho{
    
/**
 * @brief all applications derive from this class
 * @details
 * applications are a class having a set of methods bound with urls
 * @code
 * struct my_app: public udho::application<my_app>{
 *      my_app();
 *      int add(udho::request_type req, int a, int b);
 *      int mul(udho::request_type req, int a, int b);
 *      template <typename RouterT>
 *      auto route(RouterT& router){
 *          return router | (get(&my_app::add).plain() = "^/add/(\\d+)/(\\d+)$")
 *                        | (get(&my_app::mul).plain() = "^/mul/(\\d+)/(\\d+)$");
 *      }
 * };
 * @endcode
 * @tparam DerivedT The derived application class
 * \ingroup routing
 */
template <typename DerivedT>
class application{
  std::string _name;
  public:
    application(const std::string& name): _name(name){}
  public:
    std::string name() const{return _name;}
  public:
    /**
     * add a get method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto get(F f){
        return udho::get(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    /**
     * add a post method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto post(F f){
        return udho::post(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    /**
     * add a head method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto head(F f){
        return udho::head(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    /**
     * add a put method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto put(F f){
        return udho::put(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
    /**
     * add a delete method 
     * @tparam F callback type 
     * @param f callback
     */
    template <typename F>
    auto del(F f){
        return udho::del(internal::reduced(f, static_cast<DerivedT*>(this)));
    }
};

/**
 * \ingroup routing
 */
template <typename AppT, bool Ref=false>
struct app_{
    typedef app_<AppT, Ref> self_type;
    typedef self_type application_type;
    
    std::string _path;
    AppT        _app;
    
    template <typename... T>
    explicit app_(T... args): _app(args...){
        _path = "^/"+_app.name();
    }
    std::string name() const{
        return _app.name();
    }
    self_type& operator=(const std::string& path){
        _path = path;
        return *this;
    }
    template <typename ContextT, typename Lambda>
    int serve(ContextT& ctx, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        auto router = udho::router();
        return _app.route(router).serve(ctx, request_method, subject, send);
    }
    void summary(std::vector<module_info>& stack) const{
        auto router = udho::router();
        module_info info;
        info._pattern = _path;
        info._fptr = &_app;
        info._compositor = "APPLICATION";
        info._method = boost::beast::http::verb::unknown;
        const_cast<AppT&>(_app).route(router).summary(info._children);
        stack.push_back(info);
    }
    template <typename F>
    void eval(F& fnc){
        auto router = udho::router();
        auto routed = _app.route(router);
//         fnc(_app);
        routed.eval(fnc);
        fnc();
    }
};

/**
 * \ingroup routing
 */
template <typename AppT>
struct app_<AppT, true>{
    typedef app_<AppT, true> self_type;
    typedef self_type application_type;
    
    std::string _path;
    AppT&       _app;
    
    explicit app_(AppT& app): _app(app){
        _path = "^/"+_app.name();
    }
    std::string name() const{
        return _app.name();
    }
    self_type& operator=(const std::string& path){
        _path = path;
        return *this;
    }
    template <typename ContextT, typename Lambda>
    int serve(ContextT& ctx, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        auto router = udho::router();
        return _app.route(router).serve(ctx, request_method, subject, send);
    }
    void summary(std::vector<module_info>& stack) const{
        auto router = udho::router();
        module_info info;
        info._pattern = _path;
        info._fptr = &_app;
        info._compositor = "APPLICATION";
        info._method = boost::beast::http::verb::unknown;
        const_cast<AppT&>(_app).route(router).summary(info._children);
        stack.push_back(info);
    }
    template <typename F>
    void eval(F& fnc){
        auto router = udho::router();
        auto routed = _app.route(router);
//         fnc(_app);
        routed.eval(fnc);
        fnc();
    }
};

/**
 * \ingroup routing.overload
 */
template <typename U, typename V, bool Ref>
struct overload_group<U, app_<V, Ref>>{
    typedef overload_group<U, app_<V, Ref>>  self_type;
    typedef U                           parent_type;
    typedef app_<V, Ref>                     overload_type;
    typedef typename parent_type::terminal_type terminal_type;
    
    parent_type   _parent;
    overload_type _overload;
    
    overload_group(const parent_type& parent, const overload_type& overload): _parent(parent), _overload(overload){}
    template <typename ContextT, typename Lambda>
    int serve(ContextT& ctx, boost::beast::http::verb request_method, const std::string& subject, Lambda send){
        std::string subject_decoded = udho::util::urldecode(subject);
        boost::smatch match;
#ifdef WITH_ICU
        bool result = boost::u32regex_search(subject_decoded, match, boost::make_u32regex(_overload._path));
#else
        bool result = boost::regex_search(subject_decoded, match, boost::regex(_overload._path));
#endif
        if(result){
            std::string rest = result ? subject.substr(match.length()) : subject;
            return _overload.serve(ctx, request_method, rest, send);
        }else{
            return _parent.template serve<ContextT, Lambda>(ctx, request_method, subject, send);
        }
    }
    
    void summary(std::vector<module_info>& stack) const{
        _overload.summary(stack);
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
        _parent.eval(fnc);
        fnc(_overload);
        _overload.eval(fnc);
    }
    const terminal_type& terminal() const{
        return _parent.terminal();
    }
};

/**
 * \ingroup routing
 */
template <typename AppT, typename... T>
app_<AppT, false> app(T... args){
    return app_<AppT, false>(args...);
}

/**
 * \ingroup routing
 */
template <typename AppT>
app_<AppT, true> app(AppT& a){
    return app_<AppT, true>(a);
}

}

#endif // APPLICATION_H
