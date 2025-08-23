#ifndef UDHO_MANIFOLD_EVALUATOR_helper_H
#define UDHO_MANIFOLD_EVALUATOR_helper_H

#include <boost/asio/ip/address.hpp>
#include <udho/net/common.h>
#include <udho/manifold/fwd.h>
#include <udho/manifold/features.h>
#include <udho/manifold/journal.h>

namespace udho {
namespace manifold {


namespace detail {

template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT, bool YieldsResult=udho::manifold::has_result<FacetT>::value>
class next_evaluator_helper_internal{
    using facet_type      = FacetT;
    using handler_type    = HandlerT;
    using args_tuple_type = ArgsTupleT;
    using result_type     = typename udho::manifold::facet_traits<facet_type>::result_type;
    using journal_type    = typename HandlerT::journal_type;

    handler_type _handler;
    args_tuple_type _args;
public:
    inline explicit next_evaluator_helper_internal(handler_type&& handler, args_tuple_type&& args): _handler(std::move(handler)), _args(std::move(args)) {}
    next_evaluator_helper_internal(const next_evaluator_helper_internal&) = delete;
    next_evaluator_helper_internal(next_evaluator_helper_internal&& other): _handler(std::move(other._handler)), _args(std::move(other._args)) {}

    void pass(result_type&& result) {
        _handler.journal().template get<facet_type>() = std::move(result);
        proceed();
    }

    void skip(){
        proceed();
    }

    void fail(result_type&& result){
        _handler.journal().template get<facet_type>() = std::move(result);
        _handler.completion()(false);
    }

    void proceed(){
        std::apply([&](auto&&... args){
            _handler.template operator()<Idx>(std::forward<decltype(args)>(args)...);
        }, _args);
    }

    void operator()(result_type&& result, bool success = true){
        if(success) pass(std::forward<result_type>(result));
        else        fail(std::forward<result_type>(result));
    }
};

template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT>
class next_evaluator_helper_internal<Idx, ArgsTupleT, FacetT, HandlerT, false>{
    using facet_type      = FacetT;
    using handler_type    = HandlerT;
    using args_tuple_type = ArgsTupleT;
    using result_type     = typename udho::manifold::facet_traits<facet_type>::result_type;
    using journal_type    = typename HandlerT::journal_type;

    handler_type _handler;
    args_tuple_type _args;
public:
    inline explicit next_evaluator_helper_internal(handler_type&& handler, args_tuple_type&& args): _handler(std::move(handler)), _args(std::move(args)) {}
    next_evaluator_helper_internal(const next_evaluator_helper_internal&) = delete;
    next_evaluator_helper_internal(next_evaluator_helper_internal&& other): _handler(std::move(other._handler)), _args(std::move(other._args)) {}

    void pass() {
        std::apply([&](auto&&... args){
            _handler.template operator()<Idx>(std::forward<decltype(args)>(args)...);
        }, _args);
    }

    void skip(){
        pass();
    }

    void fail(){
        _handler.completion()(false);
    }

    void operator()(bool success = true){
        if(success) pass();
        else        fail();
    }
};

template <std::size_t Idx, typename ArgsTupleT, typename FacetT, typename HandlerT>
struct next_evaluator_helper: next_evaluator_helper_internal<Idx, ArgsTupleT, FacetT, HandlerT>{
    using base_type = next_evaluator_helper_internal<Idx, ArgsTupleT, FacetT, HandlerT>;

    using base_type::base_type;
};



template <std::size_t, typename...>
struct evaluator_helper;

template <std::size_t Stage, typename FeatureX, typename... Features>
struct evaluator_helper<Stage, FeatureX, Features...>{

    template <typename... Facets>
    struct handler{
        using handler_type = handler<Facets...>;
        using fabric_type  = udho::manifold::fabric<Stage, Facets...>;
        using journal_type = typename udho::manifold::detail::journal_for_facets<Facets...>::type;
        using safe_success_type = std::variant<bool, std::exception_ptr>;
        using async_callback_type = std::function<void (safe_success_type)>;

        handler(fabric_type& fabric, journal_type& journal, async_callback_type& callback): _fabric(fabric), _journal(journal), _callback(callback) {}
        handler(handler&& other): _fabric(other._fabric), _journal(other._journal), _callback(other._callback) {}
        handler(const handler&) = delete;

        journal_type& journal() { return _journal; }

        async_callback_type& completion() { return _callback; }

        template <std::size_t Idx, typename... Args, std::enable_if_t<std::is_void_v<typename fabric_type::template facet_type<FeatureX, Idx>>, bool> = true>
        void eval(Args&&... args){
            using args_tuple_type   = std::tuple<Args&&...>;
            using rest_handler_type = typename evaluator_helper<Stage, Features...>::template handler<Facets...>;

            rest_handler_type rest_handler{_fabric, _journal, _callback};
            rest_handler.template operator()<0, Args...>(std::forward<Args>(args)...);
        }

        template <std::size_t Idx, typename... Args, std::enable_if_t<(fabric_type::template count<FeatureX>() > 0 && fabric_type::template count<FeatureX>()-1 > Idx), bool> = true>
        void eval(Args&&... args){
            using facet_type      = typename fabric_type::template facet_type<FeatureX, Idx>;
            using args_tuple_type = std::tuple<Args&&...>;
            using next_type       = next_evaluator_helper<Idx+1, args_tuple_type, facet_type, handler_type>;

            auto& wrapper = _fabric.template at<FeatureX, Idx>();
            auto& facet   = wrapper.facet();

            try{
                facet(_journal, next_type{std::move(*this), std::forward_as_tuple(args...)}, std::forward<Args>(args)...); // facet will call the pass or fail method of the next_evaluator_helper
            } catch(...) {
                _callback(std::current_exception());
            }
        }

        template <std::size_t Idx, typename... Args, std::enable_if_t<(fabric_type::template count<FeatureX>() > 0 && fabric_type::template count<FeatureX>()-1 == Idx), bool> = true>
        void eval(Args&&... args){
            using facet_type        = typename fabric_type::template facet_type<FeatureX, Idx>;
            using args_tuple_type   = std::tuple<Args&&...>;
            using rest_handler_type = typename evaluator_helper<Stage, Features...>::template handler<Facets...>;
            using next_type         = next_evaluator_helper<0, args_tuple_type, facet_type, rest_handler_type>;

            auto& wrapper = _fabric.template at<FeatureX, Idx>();
            auto& facet   = wrapper.facet();

            try{
                facet(_journal, next_type{rest_handler_type{_fabric, _journal, _callback}, std::forward_as_tuple(args...)}, std::forward<Args>(args)...); // facet will call the pass or fail method of the next_evaluator_helper
            } catch(...) {
                _callback(std::current_exception());
            }
        }

        template <std::size_t Idx, typename... Args>
        void operator()(Args&&... args){
            eval<Idx>(std::forward<Args>(args)...);
        }

        private:
            fabric_type&  _fabric;
            journal_type& _journal;
            async_callback_type& _callback;
    };


};

template <std::size_t Stage>
struct evaluator_helper<Stage>{
    template <typename... Facets>
    struct handler{
        using handler_type = handler<Facets...>;
        using fabric_type  = udho::manifold::fabric<Stage, Facets...>;
        using journal_type = typename udho::manifold::detail::journal_for_facets<Facets...>::type;
        using safe_success_type = std::variant<bool, std::exception_ptr>;
        using async_callback_type = std::function<void (safe_success_type)>;

        handler(fabric_type& fabric, journal_type& journal, async_callback_type& callback): _fabric(fabric), _journal(journal), _callback(callback) {}
        handler(handler&& other): _fabric(other._fabric), _journal(other._journal), _callback(other._callback) {}
        handler(const handler&) = delete;

        journal_type& journal() { return _journal; }

        async_callback_type& completion() { return _callback; }

        template <std::size_t Idx, typename... Args>
        void operator()(Args&&... args){
            _callback(true);
        }

        private:
            fabric_type&  _fabric;
            journal_type& _journal;
            async_callback_type& _callback;
    };
};

}

// template <std::size_t Stage, typename... Features>
// struct evaluator{
//     using safe_success_type = std::variant<bool, std::exception_ptr>;
//     using async_callback_type = std::function<void (safe_success_type)>;


//     template <typename... Facets, typename... Args>
//     void eval(udho::manifold::fabric<Stage, Facets...>& fabric, typename udho::manifold::detail::journal_for_facets<Facets...>::type& journal, Args&&... args) {
//         using journal_type = typename udho::manifold::detail::journal_for_facets<Facets...>::type;
//         using helper_type  = detail::evaluator_helper<Stage, Features...>;
//         using handler_type = typename helper_type::template helper_type<Facets...>;

//         handler_type handler{fabric, journal, _callback};
//         handler(std::forward<Args>(args)...);
//     }

//     template <typename... Facets, typename... Args>
//     std::future<bool> operator()(udho::manifold::fabric<Stage, Facets...>& fabric, typename udho::manifold::detail::journal_for_facets<Facets...>::type& journal, Args&&... args){
//         eval(fabric, journal, std::forward<Args>(args)...);
//         return _promise.get_future();
//     }

// private:
//     async_callback_type _callback;
// };

}
}

#endif // UDHO_MANIFOLD_EVALUATOR_helper_H
